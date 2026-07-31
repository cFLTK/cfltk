# cfltk

An ANSI C (C99) reimplementation of the FLTK 1.3 API, architecture,
widget hierarchy, rendering model and event semantics — no C++ compiler,
no libstdc++, no RTTI, no exceptions. Built so existing FLTK 1.3
applications (Dillo in particular) can be ported with mechanical API
adjustments rather than an architectural rewrite, and so the toolkit can
eventually target embedded systems such as NuttX.

Status: early phase 1. The core object model (widgets, groups, windows,
the global event context) and a Linux/X11 reference backend are working;
`examples/hello` renders correctly. See `docs/DESIGN.md` for exactly
what's implemented, what's simplified, and what's next.

## Building

```sh
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make
./hello
```

or, without CMake:

```sh
make        # builds build/libcfltk.a and build/hello
```

Requires a C99 compiler and the X11/Xft development headers
(`libx11-dev`, `libxft-dev` on Debian/Ubuntu) for the Linux backend.

## Layout

```
include/cfltk/   public headers, one per upstream FL/*.H
src/core/        Fl_Widget, Fl_Group, the Fl global context
src/widgets/     concrete widgets (Fl_Window, Fl_Box, ...)
src/draw/        platform-independent drawing (box types, labels, colormap)
src/backend/x11/ Linux/X11 platform backend
examples/        ports of the official FLTK example suite
docs/            design notes and (later) a porting guide
```

## Design principles

- Inheritance becomes struct embedding: every derived widget embeds its
  parent as its first member, so a plain pointer cast is always a valid
  upcast.
- Virtual methods become a shared `Fl_WidgetOps` vtable
  (`draw`/`handle`/`resize`/`show`/`hide`/`destroy`/`as_group`/`as_window`).
  No switch-on-type polymorphism anywhere.
- Enum values and the default color palette are numerically identical
  to upstream FLTK 1.3.
- See `docs/DESIGN.md` for the full set of conversion rules and the
  current list of known differences from upstream.
