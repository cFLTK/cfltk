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
| `Fl_Button` | `include/cfltk/Fl_Button.h`, `src/widgets/Fl_Button.c` |
| `Fl_Toggle_Button` | `include/cfltk/Fl_Toggle_Button.h`, `src/widgets/Fl_Toggle_Button.c` |
| `Fl_Radio_Button` | `include/cfltk/Fl_Radio_Button.h`, `src/widgets/Fl_Radio_Button.c` |
| `Fl_Light_Button` + `Fl_Radio_Light_Button` | `include/cfltk/Fl_Light_Button.h`, `src/widgets/Fl_Light_Button.c` |
| `Fl_Round_Button` + `Fl_Radio_Round_Button` | `include/cfltk/Fl_Round_Button.h`, `src/widgets/Fl_Round_Button.c` |
| `Fl_Check_Button` | `include/cfltk/Fl_Check_Button.h`, `src/widgets/Fl_Check_Button.c` |
| `Fl_Return_Button` | `include/cfltk/Fl_Return_Button.h`, `src/widgets/Fl_Return_Button.c` |
| `Fl_Repeat_Button` | `include/cfltk/Fl_Repeat_Button.h`, `src/widgets/Fl_Repeat_Button.c` |
| `Fl_Input_` + `Fl_Input` | `include/cfltk/Fl_Input.h`, `src/widgets/Fl_Input.c` (merged, see header) |
| `Fl_Float_Input` | `include/cfltk/Fl_Float_Input.h`, `src/widgets/Fl_Float_Input.c` |
| `Fl_Int_Input` | `include/cfltk/Fl_Int_Input.h`, `src/widgets/Fl_Int_Input.c` |
| `Fl_Multiline_Input` | `include/cfltk/Fl_Multiline_Input.h`, `src/widgets/Fl_Multiline_Input.c` |
| `Fl_Secret_Input` | `include/cfltk/Fl_Secret_Input.h`, `src/widgets/Fl_Secret_Input.c` |
| `Fl_Output` | `include/cfltk/Fl_Output.h`, `src/widgets/Fl_Output.c` |
| `Fl_Multiline_Output` | `include/cfltk/Fl_Multiline_Output.h`, `src/widgets/Fl_Multiline_Output.c` |
| `Fl_Menu_Item` | `include/cfltk/Fl_Menu_Item.h`, `src/menu/Fl_Menu_Item.c` |
| `Fl_Menu_` | `include/cfltk/Fl_Menu_.h`, `src/menu/Fl_Menu_.c` |
| Menu popup engine (`Fl_Menu_Item::popup()`/`pulldown()`) | `src/menu/fl_menu_popup.c` (own design, see Known differences) |
| `Fl_Menu_Button` | `include/cfltk/Fl_Menu_Button.h`, `src/menu/Fl_Menu_Button.c` |
| `Fl_Choice` | `include/cfltk/Fl_Choice.h`, `src/menu/Fl_Choice.c` |
| `Fl_Menu_Bar` | `include/cfltk/Fl_Menu_Bar.h`, `src/menu/Fl_Menu_Bar.c` |
| `Fl` (static class) | `include/cfltk/Fl.h`, `src/core/Fl.c` |
| Timers (`Fl::add_timeout` etc.) | `include/cfltk/Fl.h`, `src/core/Fl.c` (fixed 32-slot pool, no per-timer heap allocation) |
| Clipboard (`Fl::copy`/`Fl::paste`) | `include/cfltk/Fl.h`, `src/core/Fl.c` (in-process only, see Known differences) |
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
- **Subclasses that add no fields reuse the parent's struct outright.**
  Several upstream classes exist purely to run different constructor
  logic (and, for the `_Light_Button` family, different `draw()`/
  `handle()`) over the *same* private fields as their base class --
  `Fl_Toggle_Button`/`Fl_Radio_Button` add nothing to `Fl_Button`;
  `Fl_Round_Button`/`Fl_Check_Button` add nothing to `Fl_Light_Button`.
  cfltk does not generate a new struct for these: `Fl_Toggle_Button_new()`
  is a factory function returning a plain `Fl_Button*` with `type()`
  pre-set, not a distinct type. This mirrors upstream's actual memory
  layout (a `Fl_Toggle_Button` *is* an `Fl_Button` with one field
  different) more directly than inventing an empty wrapper struct would,
  and avoids the "no duplicated code" the contract asks to avoid. Only
  classes that change *behavior* (a different `Fl_WidgetOps`, i.e.
  `Fl_Light_Button` and `Fl_Return_Button`) get their own vtable; classes
  that change neither fields nor behavior, only constructor defaults,
  don't even need that.
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
- **No drag-and-drop, no tooltips.** Event codes exist (`FL_DND_*`,
  `FL_SELECTIONCLEAR`) but nothing produces or consumes them yet. Match
  the contract's "optional compile-time configuration to disable" list
  — these will be CMake options, off by default for embedded targets,
  on by default for the Linux backend. Clipboard (`FL_PASTE`) exists as
  of `Fl_Input`, but only in-process (see below) — it does not claim
  X11 selection ownership or answer other applications' paste requests.
- **No images** (`Fl_Image` is an opaque forward declaration only).
  `Fl_Label`'s `image`/`deimage` fields exist but are never read.
- **Timers exist (`Fl_add_timeout`/`Fl_repeat_timeout`/`Fl_remove_timeout`),
  but `Fl::add_fd()` does not.** `Fl_wait_for()` clamps its backend wait
  to the earliest pending deadline and fires expired timeouts before
  flushing; there is no way yet to also wake on an arbitrary file
  descriptor becoming readable (would need `select()`'s fd_set extended
  past the X11 connection in `fl_x11_event.c`).
- **`Fl_Input_`/`Fl_Input` are merged into one struct/file** (`Fl_Input.h`/
  `.c`), unlike the button-family pattern of reusing a struct across
  *separate* files. Upstream splits them so people can subclass
  `Fl_Input_` with different key bindings while reusing all the field/
  buffer/selection machinery; cfltk has no such subclass yet, so the
  split has no payoff today. If one is needed later, `fl_input_ops`'s
  `handle` is the only piece that would need to move to a new file --
  the fields already model exactly what `Fl_Input_` held.
- **`Fl_Input` has no undo/redo, no word-wrap, no `^X` control-character
  display expansion, and no IME/composition support.** Cursor motion and
  `replace()` are byte-oriented, not UTF-8-character-oriented (multi-byte
  characters are treated as atomic units for *click positioning* via a
  lightweight UTF-8 length check, but not elsewhere). `FL_SECRET_INPUT`
  masks with ASCII `*` instead of upstream's Unicode bullet. See
  `Fl_Input.h` for the full list.
- **The menu popup engine is a from-scratch design, not a translation of
  `src/Fl_Menu.cxx`.** Upstream opens one native window per visible
  submenu level and relies on FLTK's own `Fl::grab()`/modal stack to
  route events across them; cfltk doesn't have that plumbing (see the
  "No grab()/modal() stack" note above). Instead, `src/menu/fl_menu_popup.c`
  uses a single borderless popup `Fl_Window` sized to the union of every
  currently-open level, with a real X11 pointer+keyboard grab
  (`fl_backend_grab()`, new in `fl_backend.h` alongside
  `fl_backend_screen_size()`) so every event is delivered to it
  regardless of which physical window is under the cursor; layout,
  hit-testing and drawing across levels are done by hand instead of
  relying on window-manager stacking. Functionally equivalent (validated
  interactively: menu bar with a 3-level-deep nested submenu, a
  right-click context menu, `Fl_Choice`, radio groups, inactive items,
  dividers, and `'&x'` mnemonic underlines all work), but it's a
  different mechanism a future maintainer should know about before
  trying to diff it against `Fl_Menu.cxx` line by line. A NuttX/NX
  backend needs `fl_backend_grab()`/`_ungrab()` implemented (NX has no
  native concept of a pointer grab, so this will likely become a
  software redirect inside the backend rather than a single ioctl/call)
  and `fl_backend_screen_size()`.
- **`Fl_Menu_::add()`/`insert()` take a flat label, no "File/Open"
  hierarchical path parsing** (see `Fl_Menu_.h`); build submenu arrays
  explicitly with `FL_SUBMENU`/`FL_SUBMENU_POINTER` instead, which is
  what upstream's path parser produces internally anyway. No
  `item_pathname()`/`find_index(pathname)` either, for the same reason.
- **`'&x'` mnemonic display** (the underline under a label's shortcut
  letter) is implemented (`fl_draw_shortcut` in `fl_draw.h`, honored by
  `fl_label_draw()`), but assumes the marked character is a single ASCII
  byte -- consistent with the ASCII-only shortcut limitation noted
  elsewhere (Fl_Widget.h, Fl_Button.h).
- **No color schemes** (upstream's `Fl::scheme("gtk+"/"plastic"/"gleam")`).
  `Fl_Light_Button`'s indicator always renders via the plain
  `FL_THIN_DOWN_BOX` + `fl_pie()` path; the box-type families those
  schemes need (`_FL_GTK_*`, `_FL_PLASTIC_*`, `_FL_GLEAM_*`) already fall
  back to `fl_up_box`/`fl_border_frame` per the box-type note above.
- **Shortcuts are ASCII-only.** `Fl_Widget_label_shortcut()` /
  `Fl_Widget_test_shortcut()` (the `'&x'` label shortcut) and
  `Fl_test_shortcut()` (the explicit `Fl_Button_shortcut()` value) both
  decode a single byte instead of a full UTF-8 code point, since
  `fl_utf8decode()` hasn't been ported. Only matters for non-ASCII
  shortcut letters.
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

1. More widgets: `Fl_Valuator` family (sliders, dials, counters),
   `Fl_Browser_`, `Fl_Tabs`, `Fl_Scroll`, `Fl_Text_Buffer`/`Fl_Text_Editor`.
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
