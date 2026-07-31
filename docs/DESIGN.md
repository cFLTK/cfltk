# cfltk Design Document

cfltk is an ANSI C (C99) reimplementation of the FLTK 1.3 API surface,
architecture, widget hierarchy, rendering model and event semantics,
built so that FLTK 1.3 client code (Dillo's `dw::fltk` layer in
particular) can be ported with mechanical, not architectural, changes.
This document tracks, class by class, how each FLTK C++ construct was
translated to C, and records what is intentionally not implemented yet.

See also each header's top-of-file comment (`include/cfltk/*.h`), which
carries the same Original class / New C structure / Inheritance / Vtbl /
Ownership / Known differences breakdown for that specific class — this
document is the index and cross-cutting notes; the headers are the
source of truth for any one class.

## Status: Phase 1 (Linux/X11 core pipeline)

Implemented and validated end-to-end (`examples/hello` renders correctly
under an isolated X11 display):

| Upstream class | cfltk translation unit(s) |
|---|---|
| `Fl_Widget` | `include/cfltk/Fl_Widget.h`, `src/core/Fl_Widget.c` |
| `Fl_Group` | `include/cfltk/Fl_Group.h`, `src/core/Fl_Group.c` |
| `Fl_Window` | `include/cfltk/Fl_Window.h`, `src/widgets/Fl_Window.c` |
| `Fl_Box` | `include/cfltk/Fl_Box.h`, `src/widgets/Fl_Box.c` |
| `Fl` (static class) | `include/cfltk/Fl.h`, `src/core/Fl.c` |
| `fl_draw.H` free functions | `include/cfltk/fl_draw.h`, `src/draw/fl_draw.c` |
| `Enumerations.H` | `include/cfltk/Enumerations.h` |
| default color map | `include/cfltk/fl_colormap.h`, `src/draw/fl_colormap.c` |
| Linux/X11 platform layer | `src/backend/x11/*`, seam defined in `src/backend/fl_backend.h` |

## Cross-cutting translation rules

- **Inheritance → embedding.** Every derived widget struct embeds its
  parent struct as its first member (`Fl_Group.widget`,
  `Fl_Window.group`, ...). This makes `(Fl_Widget*)ptr` always valid for
  any widget type without a cast table, which is what makes
  `FL_WIDGET()` a no-op macro.
- **Virtual methods → `Fl_WidgetOps`.** One vtable struct
  (`draw`/`handle`/`resize`/`show`/`hide`/`destroy`/`as_group`/
  `as_window`) shared by every widget type; a concrete widget either
  points at a shared `const Fl_WidgetOps` instance (`fl_box_ops`,
  `fl_group_ops`, `fl_window_ops`) or, if it has no per-instance state
  beyond what `Fl_Widget` already carries, reuses another type's table
  outright. There is no dynamic dispatch beyond following `widget->ops`.
- **RTTI avoidance.** Upstream already avoids `dynamic_cast` via
  `as_group()`/`as_window()`/`as_gl_window()` virtuals returning `this`
  or `0`. cfltk keeps exactly that mechanism (`Fl_Widget_as_group()`,
  `Fl_Widget_as_window()`) instead of inventing a separate tag/kind
  enum, and builds the checked-cast macros `FL_GROUP()`/`FL_WINDOW()` on
  top of it (assert in debug builds, plain reinterpret in release).
- **Constructors/destructors.** `Fl_<Type>_init()` (in-place, caller
  owns storage) and `Fl_<Type>_new()` (heap-allocates then calls
  `_init()`). Destruction is split the same way C++ splits it into
  "run every destructor body root-to-leaf then free": `ops->destroy()`
  tears down subclass-owned state and, for containers, recursively
  destroys+frees children; `Fl_Widget_base_destroy()` is the shared
  "base class destructor" step every `destroy()` must call last
  (removes the widget from its parent, frees a copied label/tooltip,
  clears any dangling focus/pushed/belowmouse/tracker references);
  `Fl_Widget_delete()` = dispatch `destroy()` then `free()`.
- **Ownership.** A widget is owned by its parent `Fl_Group` once added
  (`Fl_Group_add`); deleting a group recursively deletes its children,
  matching `Fl_Group::~Fl_Group()`/`clear()`. Not-yet-added widgets and
  top-level windows are owned by whoever called `Fl_*_new()`. The
  global context (`Fl.c`) never owns widgets, only observes them
  (`focus`/`pushed`/`belowmouse` are cleared automatically on deletion
  via `Fl_context_widget_deleted()`).
- **The widget tree auto-build mechanism.** Upstream's `Fl_Widget`
  constructor does `if (Fl_Group::current()) Fl_Group::current()->add(this);`
  and `Fl_Group`'s constructor calls `begin()` (but never auto-`end()`s).
  cfltk keeps this exactly: `Fl_Widget_init()` auto-adds to
  `Fl_Group_current()`, and `Fl_Group_init()`/`Fl_Window_init()` call
  `Fl_Group_begin()` but leave closing the group (`Fl_Group_end()`) to
  the caller. This is what lets `examples/hello` build its tree by
  simply constructing widgets in order, exactly like the C++ original.
- **Colors/box types/events are numerically unchanged.** `Enumerations.h`
  preserves every enumerator's integer value from `FL/Enumerations.H`,
  and `fl_colormap.c` reproduces upstream's 256-entry default palette
  verbatim, so widget code and saved numeric values stay compatible.

## Known differences from upstream (tracked for later phases)

- **Box types**: only `FL_NO_BOX` through `FL_BORDER_FRAME` have real
  drawing implementations (`src/draw/fl_draw.c`); rounded/oval/plastic/
  gtk/gleam box types fall back to `fl_up_box`/`fl_border_frame`.
  Upstream keeps these in separate translation units for the same
  reason cfltk will: don't force a build to link box art it never uses.
  Each family becomes its own `src/draw/fl_box_<family>.c`, gated by a
  CMake option, in a later phase.
- **No grab()/modal() stack.** Popup menus and modal dialogs need this;
  `Fl_context_handle()` in `Fl.c` is the seam where it will be added.
- **No drag-and-drop, no clipboard, no tooltips.** Event codes exist
  (`FL_DND_*`, `FL_PASTE`, `FL_SELECTIONCLEAR`) but nothing produces or
  consumes them yet. Match the contract's "optional compile-time
  configuration to disable" list — these will be CMake options, off by
  default for embedded targets, on by default for the Linux backend.
- **No images** (`Fl_Image` is an opaque forward declaration only).
  `Fl_Label`'s `image`/`deimage` fields exist but are never read.
- **Labels**: only `FL_NORMAL_LABEL` actually draws differently from
  `FL_NO_LABEL`; shadow/engraved/embossed/icon/image label types parse
  but render as plain text. `fl_label_draw()` in `src/draw/fl_draw.c`
  is the place to extend.
- **Clipping** is a single intersected rectangle stack
  (`src/backend/x11/fl_x11_driver.c`), not an arbitrary X `Region`.
  Sufficient for rectangular widget damage; revisit if a widget needs
  non-rectangular clipping (e.g. round buttons).
- **`Fl_Group_resize()`** recomputes proportional child geometry using
  upstream's algorithm from `sizes_`, but `sizes_` is allocated lazily
  on first resize exactly like upstream — this one has no behavioral
  difference, noted here only because it's the single most intricate
  piece of arithmetic ported so far and worth double-checking against
  `src/Fl_Group.cxx` if resize bugs show up.

## Next phases (not started)

1. More widgets: the `Fl_Button` family, `Fl_Input`/text fields, menus,
   `Fl_Valuator` family, `Fl_Browser_`, `Fl_Tabs`, `Fl_Scroll`,
   `Fl_Text_Buffer`/`Fl_Text_Editor`.
2. `Fl_Image` + image loaders (behind a compile-time switch).
3. The rest of the official FLTK example suite (see the contract's
   "Required Validation Programs" list), each one both a port target
   and a regression check on the core.
4. Automated regression tests (widget lifecycle, event propagation,
   layout/resize, parent/child bookkeeping) — none exist yet; phase 1
   was validated by visual inspection of `examples/hello` only.
5. Dillo integration once the widget set Dillo's `dw::fltk` needs is
   covered.
6. A NuttX/NX backend implementing the exact `src/backend/fl_backend.h`
   seam the X11 backend implements now.

## Reference source

`../reference/fltk-1.3.11-reference/` holds the official FLTK 1.3.11
source (downloaded from
`https://github.com/fltk/fltk/releases/download/release-1.3.11/fltk-1.3.11-source.tar.gz`),
kept read-only as the ground truth for behavior. It is not built and is
not part of cfltk's own source tree.
