#include "scribble.hpp"

#include <panes/panes.hpp>

#include <filesystem>
#include <exception>
#include <iostream>

namespace fs = std::filesystem;

const char *kCss = R"css(
#main    { background-color: #180b0e; color: #f7e6e8; padding: 16px; gap: 12px; }
#screen     { flex: 1; flex-direction: row; gap: 12px; }
canvas   { flex: 1; border: 2px solid #5c242e; border-radius: 14px; }
#bar     { flex-direction: row; gap: 10px; align-items: center; }

.btn       { padding: 8px 16px; border-radius: 8px; background-color: #b91c1c; color: #ffffff; }
.btn:hover { background-color: #dc2626; }

#open, #save             { background-color: #7f1d1d; border: 1px solid #991b1b; }
#open:hover, #save:hover { background-color: #991b1b; }

#color_input {
    width: 110px;
    height: 34px;
    padding: 6px 10px;
    border: 1px solid #5c242e;
    border-radius: 8px;
    background-color: #261116;
    color: #f7e6e8;
}

#color_input:focus {
    border-color: #ef4444;
}

#readout { color: #d8a8b0; }
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

    // optional CSS external theme:
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