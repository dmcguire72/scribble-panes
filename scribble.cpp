#include "scribble.hpp"

#include <panes/panes.hpp>
#include <SDL3/SDL.h>

#include <fstream>
#include <sstream>
#include <string>
#include <utility>

#define BIND_HOVER(c_app, element, description) \
    c_app.on(element ":hover", [](panes::Event& e) { e.app().set_text("readout", description); e.stop_propagation(); })
#define BIND_HOVER_BG(c_app, element) \
    BIND_HOVER(c_app, element, "")

namespace {

bool parse_hex_color(const std::string& text, StrokeColor& out) {
    size_t start = (!text.empty() && text[0] == '#') ? 1 : 0;
    const size_t len = text.size() - start;
    if (len != 3 && len != 4 && len != 6 && len != 8) return false;

    auto hex = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    for (size_t i = start; i < text.size(); ++i) {
        if (hex(text[i]) < 0) return false;
    }

    if (len == 3 || len == 4) {
        out.r = uint8_t(hex(text[start + 0]) * 17);
        out.g = uint8_t(hex(text[start + 1]) * 17);
        out.b = uint8_t(hex(text[start + 2]) * 17);
        out.a = len == 4 ? uint8_t(hex(text[start + 3]) * 17) : 255;
    } else {
        out.r = uint8_t(hex(text[start + 0]) * 16 + hex(text[start + 1]));
        out.g = uint8_t(hex(text[start + 2]) * 16 + hex(text[start + 3]));
        out.b = uint8_t(hex(text[start + 4]) * 16 + hex(text[start + 5]));
        out.a = len == 8 ? uint8_t(hex(text[start + 6]) * 16 + hex(text[start + 7])) : 255;
    }
    return true;
}

bool save_segments(const std::string& path, const std::vector<Segment>& segments) {
    std::ofstream out(path);
    if (!out) return false;
    out << "SCRIBBLE 1\n";
    for (const Segment& s : segments) {
        out << s.x0 << ' ' << s.y0 << ' ' << s.x1 << ' ' << s.y1 << ' '
            << int(s.color.r) << ' ' << int(s.color.g) << ' '
            << int(s.color.b) << ' ' << int(s.color.a) << '\n';
    }
    return out.good();
}

bool load_segments(const std::string& path, std::vector<Segment>& out_segments) {
    std::ifstream in(path);
    if (!in) return false;

    std::string header;
    if (!std::getline(in, header) || header.rfind("SCRIBBLE 1", 0) != 0) return false;

    std::vector<Segment> loaded;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::istringstream ss(line);
        Segment s;
        int r = 0, g = 0, b = 0, a = 255;
        if (!(ss >> s.x0 >> s.y0 >> s.x1 >> s.y1 >> r >> g >> b >> a)) return false;
        s.color = { uint8_t(r), uint8_t(g), uint8_t(b), uint8_t(a) };
        loaded.push_back(s);
    }
    out_segments = std::move(loaded);
    return true;
}

}  // namespace

void ScribbleEngine::bind(panes::App& app) {
    bind_canvas(app);
    bind_toolbar(app);
    bind_shortcuts(app);
}

void ScribbleEngine::clear_surface(panes::App& app) {
    if (panes::Canvas* c = app.canvas("scribble")) {
        c->clear(30, 14, 18, 255);
    }
}

bool ScribbleEngine::canvas_point(panes::Event& e, float& x, float& y) {
    panes::Canvas* c = e.app().canvas("scribble");
    if (!c) return false;
    x = e.local_x() - c->offset_x();
    y = e.local_y() - c->offset_y();
    return true;
}

void ScribbleEngine::redraw_all(panes::App& app) {
    painted_ = 0;
    have_last_ = false;
    clear_surface(app);
    app.invalidate("scribble");
}

void ScribbleEngine::commit_stroke() {
    if (!have_last_) return;
    have_last_ = false;
    if (segments_.size() > stroke_start_) {
        DrawOp op;
        op.kind = DrawOp::Kind::Stroke;
        op.count = segments_.size() - stroke_start_;
        undo_stack_.push_back(std::move(op));
        redo_stack_.clear();
    }
}

void ScribbleEngine::begin_stroke(float x, float y, panes::App& app) {
    commit_stroke();
    stroke_start_ = segments_.size();
    have_last_ = true;
    last_x_ = x;
    last_y_ = y;
    if (segments_.empty()) clear_surface(app);
    segments_.push_back({ x, y, x, y, current_color_ });
    app.invalidate("scribble");
}

void ScribbleEngine::extend_stroke(float x, float y, panes::App& app) {
    if (!have_last_) return;
    segments_.push_back({ last_x_, last_y_, x, y, current_color_ });
    last_x_ = x;
    last_y_ = y;
    app.invalidate("scribble");
}

void ScribbleEngine::clear(panes::App& app) {
    commit_stroke();
    if (!segments_.empty()) {
        DrawOp op;
        op.kind = DrawOp::Kind::Clear;
        op.segments = std::move(segments_);
        undo_stack_.push_back(std::move(op));
        redo_stack_.clear();
    }
    segments_.clear();
    redraw_all(app);
    app.set_text("readout", "Cleared");
}

void ScribbleEngine::replace_all(std::vector<Segment> loaded, panes::App& app) {
    segments_ = std::move(loaded);
    undo_stack_.clear();
    redo_stack_.clear();
    redraw_all(app);
}

void ScribbleEngine::undo(panes::App& app) {
    commit_stroke();
    if (undo_stack_.empty()) {
        app.set_text("readout", "nothing to undo");
        return;
    }
    DrawOp op = std::move(undo_stack_.back());
    undo_stack_.pop_back();

    if (op.kind == DrawOp::Kind::Stroke) {
        const size_t keep = segments_.size() - op.count;
        op.segments.assign(segments_.begin() + long(keep), segments_.end());
        segments_.resize(keep);
        redo_stack_.push_back(std::move(op));
    } else {
        segments_ = std::move(op.segments);
        redo_stack_.push_back({ DrawOp::Kind::Clear, 0, {} });
    }
    redraw_all(app);
    app.set_text("readout", "undo");
}

void ScribbleEngine::redo(panes::App& app) {
    commit_stroke();
    if (redo_stack_.empty()) {
        app.set_text("readout", "nothing to redo");
        return;
    }
    DrawOp op = std::move(redo_stack_.back());
    redo_stack_.pop_back();

    if (op.kind == DrawOp::Kind::Stroke) {
        if (segments_.empty()) clear_surface(app);
        segments_.insert(segments_.end(), op.segments.begin(), op.segments.end());
        op.count = op.segments.size();
        op.segments.clear();
        op.segments.shrink_to_fit();
        undo_stack_.push_back(std::move(op));
        app.invalidate("scribble");
    } else {
        op.segments = std::move(segments_);
        segments_.clear();
        undo_stack_.push_back(std::move(op));
        redraw_all(app);
    }
    app.set_text("readout", "redo");
}

void ScribbleEngine::paint(panes::Canvas& c) {
    SDL_Renderer* r = c.renderer();
    if (c.resized()) {
        SDL_SetRenderDrawColor(r, 30, 14, 18, 255);
        SDL_RenderClear(r);
        painted_ = 0;
    }
    for (; painted_ < segments_.size(); ++painted_) {
        const Segment& s = segments_[painted_];
        SDL_SetRenderDrawColor(r, s.color.r, s.color.g, s.color.b, s.color.a);
        SDL_RenderLine(r, s.x0, s.y0, s.x1, s.y1);
        SDL_RenderLine(r, s.x0 + 1, s.y0, s.x1 + 1, s.y1);
        SDL_RenderLine(r, s.x0, s.y0 + 1, s.x1, s.y1 + 1);
    }
    if (segments_.empty()) {
        SDL_SetRenderDrawColor(r, 150, 90, 100, 255);
        SDL_RenderDebugText(r, 12, 12, "draw here");
    }
}

void ScribbleEngine::bind_canvas(panes::App& app) {
    app.on_paint("scribble", [this](panes::Canvas& c) { paint(c); });

    app.on("scribble:mousedown", [this](panes::Event& e) {
        float x, y;
        if (canvas_point(e, x, y)) begin_stroke(x, y, e.app());
    });

    app.on("scribble:mousemove", [this](panes::Event& e) {
        float x, y;
        if (!have_last_ || !canvas_point(e, x, y)) return;
        if (!(SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_LMASK)) {
            commit_stroke();
            return;
        }
        extend_stroke(x, y, e.app());
    });

    app.on("scribble:mouseup", [this](panes::Event&) { commit_stroke(); });
}

void ScribbleEngine::bind_toolbar(panes::App& app) {
    app.on("color_input:change", [this](panes::Event& e) {
        StrokeColor parsed;
        if (parse_hex_color(e.text(), parsed)) current_color_ = parsed;
    });

    app.on("clear:click", [this](panes::Event& e) { clear(e.app()); });

    app.on("save:click", [this, &app](panes::Event&) {
        commit_stroke();
        panes::FileDialogOptions opts;
        opts.title = "Save Scribble Drawing";
        opts.filters = { { "Scribble Drawing (.scribble)", "scribble" }, { "All Files", "*" } };
        app.save_file(opts, [this, &app](const std::vector<std::string>& paths) {
            if (paths.empty()) return;
            if (save_segments(paths.front(), segments_)) {
                app.set_text("readout", "Saved " + std::to_string(segments_.size()) + " segments");
            } else {
                app.set_text("readout", "Failed to save file");
            }
        });
    });

    app.on("open:click", [this, &app](panes::Event&) {
        commit_stroke();
        panes::FileDialogOptions opts;
        opts.title = "Open Scribble Drawing";
        opts.filters = { { "Scribble Drawing (.scribble)", "scribble" }, { "All Files", "*" } };
        app.open_file(opts, [this, &app](const std::vector<std::string>& paths) {
            if (paths.empty()) return;
            std::vector<Segment> loaded;
            if (load_segments(paths.front(), loaded)) {
                const size_t count = loaded.size();
                replace_all(std::move(loaded), app);
                app.set_text("readout", "loaded " + std::to_string(count) + " segments");
            } else {
                app.set_text("readout", "failed to load file!");
            }
        });
    });

    BIND_HOVER(app, "scribble", "drag cursor/finger to scribble");
    BIND_HOVER(app, "clear", "clear current scribble");
    BIND_HOVER(app, "color_input", "customize color");
    BIND_HOVER(app, "open", "load scribble from file");
    BIND_HOVER(app, "save", "save scribble to file");
    BIND_HOVER_BG(app, "main");
}

void ScribbleEngine::bind_shortcuts(panes::App& app) {
    app.on("main:keydown", [this](panes::Event& e) {
        if (!e.mods().command()) return;
        if (e.key() == panes::Key::Z) {
            if (e.mods().shift) redo(e.app());
            else undo(e.app());
            e.stop_propagation();
        } else if (e.key() == panes::Key::Y) {
            redo(e.app());
            e.stop_propagation();
        }
    });
}