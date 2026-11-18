#include "scribble.hpp"

#include <panes/panes.hpp>

#include <filesystem>
#include <exception>
#include <iostream>

namespace fs = std::filesystem;

const char *kCss = R"css(
:root {
    --bg:          light-dark(#fff5f6, #180b0e);
    --fg:          light-dark(#3b1219, #f7e6e8);
    --muted:       light-dark(#8a4a54, #d8a8b0);
    --border:      light-dark(#e3b5bc, #5c242e);
    --field-bg:    light-dark(#ffffff, #261116);
    --focus:       light-dark(#dc2626, #ef4444);
    --btn-bg:      light-dark(#dc2626, #b91c1c);
    --btn-hover:   light-dark(#b91c1c, #dc2626);
    --btn2-bg:     light-dark(#fee2e2, #7f1d1d);
    --btn2-fg:     light-dark(#7f1d1d, #ffffff);
    --btn2-border: light-dark(#fca5a5, #991b1b);
    --btn2-hover:  light-dark(#fecaca, #991b1b);
}

#main    { background-color: var(--bg); color: var(--fg); padding: 16px; gap: 12px; }
#screen     { flex: 1; flex-direction: row; gap: 12px; }
canvas   { flex: 1; border: 2px solid var(--border); border-radius: 14px; }
#bar     { flex-direction: row; gap: 10px; align-items: center; }

.btn       { padding: 8px 16px; border-radius: 8px; background-color: var(--btn-bg); color: #ffffff; }
.btn:hover { background-color: var(--btn-hover); }

#open, #save             { background-color: var(--btn2-bg); color: var(--btn2-fg); border: 1px solid var(--btn2-border); }
#open:hover, #save:hover { background-color: var(--btn2-hover); }

#color_input {
    width: 110px;
    height: 34px;
    padding: 6px 10px;
    border: 1px solid var(--border);
    border-radius: 8px;
    background-color: var(--field-bg);
    color: var(--fg);
}

#color_input:focus {
    border-color: var(--focus);
}

#readout { color: var(--muted); }
)css";

const char* kUI = R"yaml(
panes:
  - id: main
    children:
      - id: bar
        type: container
        children:
          - id: open
            type: button
            classes: [btn]
            children:
              - { type: text, content: "Open" }
          - id: save
            type: button
            classes: [btn]
            children:
              - { type: text, content: "Save" }
          - id: clear
            type: button
            classes: [btn]
            children:
              - { type: text, content: "Clear" }
          - { id: color_input, type: input, placeholder: "#ff4d4d", value: "#ff4d4d" }
          - { id: readout, type: text, content: "welcome to scribble!" }
      - id: screen
        type: container
        children:
          - { id: scribble, type: canvas }
)yaml";

int main(int, char**) try {
    panes::App app;
    app.title("Scribble").add_panes(kUI);
#if defined(PANES_FAMILY_DESKTOP)
    app.window_size(900, 560);
#endif

    if (fs::exists("assets/style.css")) {
        app.stylesheet("assets/style.css");
    } else {
        app.stylesheet_string(kCss);
    }

    ScribbleEngine engine;
    engine.bind(app);

    return app.run();
} catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
}