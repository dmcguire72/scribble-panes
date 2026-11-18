#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace panes {
class App;
class Canvas;
class Event;
}  // namespace panes

struct StrokeColor {
    uint8_t r = 255, g = 77, b = 77, a = 255;
};

struct Segment {
    float x0, y0, x1, y1;
    StrokeColor color;
};

struct DrawOp {
    enum class Kind { Stroke, Clear };
    Kind kind = Kind::Stroke;
    size_t count = 0;
    std::vector<Segment> segments;
};

class ScribbleEngine {
public:
    ScribbleEngine() = default;
    ~ScribbleEngine() = default;

    ScribbleEngine(const ScribbleEngine&)            = delete;
    ScribbleEngine& operator=(const ScribbleEngine&) = delete;
    ScribbleEngine(ScribbleEngine&&)                 = delete;
    ScribbleEngine& operator=(ScribbleEngine&&)      = delete;

    void bind(panes::App& app);

    void begin_stroke(float x, float y, panes::App& app);
    void extend_stroke(float x, float y, panes::App& app);
    void commit_stroke();

    void clear(panes::App& app);
    void undo(panes::App& app);
    void redo(panes::App& app);
    void replace_all(std::vector<Segment> loaded, panes::App& app);

    void paint(panes::Canvas& c);

    const std::vector<Segment>& segments() const { return segments_; }
    StrokeColor current_color() const { return current_color_; }
    void set_color(StrokeColor color) { current_color_ = color; }

private:
    static void clear_surface(panes::App& app);
    static bool canvas_point(panes::Event& e, float& x, float& y);

    void redraw_all(panes::App& app);
    void bind_canvas(panes::App& app);
    void bind_toolbar(panes::App& app);
    void bind_shortcuts(panes::App& app);

    std::vector<Segment> segments_;
    std::vector<DrawOp>  undo_stack_;
    std::vector<DrawOp>  redo_stack_;
    panes::App*          app_          = nullptr;
    StrokeColor          current_color_{ 255, 77, 77, 255 };
    size_t               painted_      = 0;
    size_t               stroke_start_ = 0;
    bool                 have_last_    = false;
    float                last_x_       = 0.f;
    float                last_y_       = 0.f;
};