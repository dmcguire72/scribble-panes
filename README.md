# Scribble

A drawing application written in Panes (using Panes SDL3 Software Canvas for the drawing area)

Supports undo/redo, save/load, color, and partial theming (via CSS)

<img width="451" height="296" alt="image" src="https://github.com/user-attachments/assets/f83624ac-c094-4c17-a6ad-9d3713c7e058" />
<img width="451" height="296" alt="image" src="https://github.com/user-attachments/assets/4cf0c069-8386-4074-bd36-f50f7fbb67ad" />

A scribble file looks like:

```scribble
SCRIBBLE 1
90 45 90 45 255 77 77 255
90 45 90 47 255 77 77 255
90 47 90 48 255 77 77 255
90 48 90 50 255 77 77 255
90 50 90 54 255 77 77 255
90 54 89 61 255 77 77 255
89 61 87 66 255 77 77 255
87 66 86 70 255 77 77 255
86 70 85 73 255 77 77 255
85 73 84 75 255 77 77 255
84 75 83 76 255 77 77 255
```

And is laid out as:

```
MAGIC
x1 y1 x2 y2 r g b a
x1 y1 x2 y2 r g b a
x1 y1 x2 y2 r g b a
...
```

Panes is an unstable in development UI framework, as with all unsafe software do not trust unknown files and DO NOT just load them into any Panes application!
