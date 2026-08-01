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
| `Fl_Double_Window` | `include/cfltk/Fl_Double_Window.h`, `src/widgets/Fl_Double_Window.c` (+ offscreen-buffer support in `src/backend/x11/fl_x11_window.c`) |
| `Fl_Single_Window` | `include/cfltk/Fl_Single_Window.h`, `src/widgets/Fl_Single_Window.c` |
| `Fl_Menu_Window` | `include/cfltk/Fl_Menu_Window.h`, `src/widgets/Fl_Menu_Window.c` |
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
| Portable vertex/matrix drawing (`src/fl_vertex.cxx`: `fl_push_matrix`/`fl_begin_polygon`/`fl_vertex`/...) | `include/cfltk/fl_draw.h`, `src/draw/fl_draw.c` (file-static matrix stack/point buffer, see Known differences) |
| `'@'`-symbol label glyphs (`src/fl_symbols.cxx`: `fl_add_symbol`/`fl_draw_symbol`, ~35 built-in shapes) | `include/cfltk/fl_draw.h`, `src/draw/fl_symbols.c` (linear-scan table, no `"returnarrow"`, see Known differences) |
| `Enumerations.H` | `include/cfltk/Enumerations.h` |
| default color map | `include/cfltk/fl_colormap.h`, `src/draw/fl_colormap.c` |
| Linux/X11 platform layer | `src/backend/x11/*`, seam defined in `src/backend/fl_backend.h` |
| `Fl_Valuator` | `include/cfltk/Fl_Valuator.h`, `src/valuators/Fl_Valuator.c` |
| `Fl_Slider` (+ `Fl_Fill_Slider`/`Fl_Hor_Slider`/`Fl_Hor_Fill_Slider`/`Fl_Nice_Slider`/`Fl_Hor_Nice_Slider`) | `include/cfltk/Fl_Slider.h` and siblings, `src/valuators/Fl_Slider.c` and siblings |
| `Fl_Value_Slider` (+ `Fl_Hor_Value_Slider`) | `include/cfltk/Fl_Value_Slider.h`, `src/valuators/Fl_Value_Slider.c` and siblings |
| `Fl_Scrollbar` | `include/cfltk/Fl_Scrollbar.h`, `src/valuators/Fl_Scrollbar.c` |
| `Fl_Dial` (+ `Fl_Fill_Dial`/`Fl_Line_Dial`) | `include/cfltk/Fl_Dial.h` and siblings, `src/valuators/Fl_Dial.c` and siblings |
| `Fl_Counter` (+ `Fl_Simple_Counter`) | `include/cfltk/Fl_Counter.h`, `src/valuators/Fl_Counter.c` and siblings |
| `Fl_Roller` | `include/cfltk/Fl_Roller.h`, `src/valuators/Fl_Roller.c` |
| `Fl_Value_Input` | `include/cfltk/Fl_Value_Input.h`, `src/valuators/Fl_Value_Input.c` |
| `Fl_Value_Output` | `include/cfltk/Fl_Value_Output.h`, `src/valuators/Fl_Value_Output.c` |
| `Fl_Adjuster` | `include/cfltk/Fl_Adjuster.h`, `src/valuators/Fl_Adjuster.c` |
| `Fl_Progress` | `include/cfltk/Fl_Progress.h`, `src/widgets/Fl_Progress.c` |
| `Fl_Spinner` | `include/cfltk/Fl_Spinner.h`, `src/widgets/Fl_Spinner.c` |
| `Fl_Clock_Output`/`Fl_Clock`/`Fl_Round_Clock` | `include/cfltk/Fl_Clock.h`, `src/widgets/Fl_Clock.c` |
| `Fl_Tabs` | `include/cfltk/Fl_Tabs.h`, `src/widgets/Fl_Tabs.c` |
| `Fl_Scroll` | `include/cfltk/Fl_Scroll.h`, `src/widgets/Fl_Scroll.c` |
| `Fl_Pack` | `include/cfltk/Fl_Pack.h`, `src/widgets/Fl_Pack.c` |
| `Fl_Tile` | `include/cfltk/Fl_Tile.h`, `src/widgets/Fl_Tile.c` |
| `Fl_Wizard` | `include/cfltk/Fl_Wizard.h`, `src/widgets/Fl_Wizard.c` |
| `Fl_Browser_` | `include/cfltk/Fl_Browser_.h`, `src/widgets/Fl_Browser_.c` |
| `Fl_Browser` | `include/cfltk/Fl_Browser.h`, `src/widgets/Fl_Browser.c` |
| `Fl_Select_Browser` / `Fl_Hold_Browser` / `Fl_Multi_Browser` | `include/cfltk/Fl_{Select,Hold,Multi}_Browser.h`, `src/widgets/Fl_{Select,Hold,Multi}_Browser.c` |
| `Fl_Check_Browser` | `include/cfltk/Fl_Check_Browser.h`, `src/widgets/Fl_Check_Browser.c` |
| UTF-8 primitives (new infrastructure, not an upstream header) | `include/cfltk/fl_utf8.h`, `src/core/fl_utf8.c` |
| `Fl_Text_Buffer` (+ `Fl_Text_Selection`) | `include/cfltk/Fl_Text_Buffer.h`, `src/text/Fl_Text_Buffer.c` |
| `Fl_Text_Display` | `include/cfltk/Fl_Text_Display.h`, `src/text/Fl_Text_Display.c` |
| `Fl_Text_Editor` | `include/cfltk/Fl_Text_Editor.h`, `src/text/Fl_Text_Editor.c` |
| `Fl_Image` (base) | `include/cfltk/Fl_Image.h`, `src/image/Fl_Image.c` |
| `Fl_RGB_Image` | `include/cfltk/Fl_RGB_Image.h`, `src/image/Fl_RGB_Image.c` |
| `Fl_Pixmap` (+ XPM parsing engine from `fl_draw_pixmap.cxx`) | `include/cfltk/Fl_Pixmap.h`, `src/image/Fl_Pixmap.c` |
| `Fl_Bitmap` | `include/cfltk/Fl_Bitmap.h`, `src/image/Fl_Bitmap.c` |
| Raw image blit/read-back (`fl_draw_image`/`fl_read_image`/`fl_draw_bitmask`, new `Fl_Graphics_Driver` methods) | `include/cfltk/fl_draw.h`, `src/backend/x11/fl_x11_driver.c` |
| `fl_parse_color` (new; XPM color-table parsing) | `include/cfltk/fl_colormap.h`, `src/draw/fl_colormap.c` |
| `Fl_BMP_Image` | `include/cfltk/Fl_BMP_Image.h`, `src/image/Fl_BMP_Image.c` |
| `Fl_GIF_Image` | `include/cfltk/Fl_GIF_Image.h`, `src/image/Fl_GIF_Image.c` |
| `Fl_PNG_Image` (built only when libpng is found, see `CFLTK_ENABLE_PNG`) | `include/cfltk/Fl_PNG_Image.h`, `src/image/Fl_PNG_Image.c` |
| `Fl_JPEG_Image` (built only when libjpeg is found, see `CFLTK_ENABLE_JPEG`) | `include/cfltk/Fl_JPEG_Image.h`, `src/image/Fl_JPEG_Image.c` |
| `Fl_Shared_Image` (+ `fl_register_images`/`fl_check_images` from `fl_images_core.cxx`) | `include/cfltk/Fl_Shared_Image.h`, `src/image/Fl_Shared_Image.c` |
| `Fl_XPM_Image` (loads a `.xpm` file, reusing `Fl_Pixmap`) | `include/cfltk/Fl_XPM_Image.h`, `src/image/Fl_XPM_Image.c` |
| `Fl_XBM_Image` (loads a `.xbm` file, reusing `Fl_Bitmap`) | `include/cfltk/Fl_XBM_Image.h`, `src/image/Fl_XBM_Image.c` |
| `filename.H` utilities (`fl_filename_list`/`fl_numericsort`/`fl_filename_match`/`fl_filename_isdir`/`fl_filename_name`/`fl_filename_ext`/`fl_filename_setext`/`fl_filename_expand`/`fl_filename_absolute`/`fl_filename_relative`, collected from several small upstream files) | `include/cfltk/fl_filename.h`, `src/core/fl_filename.c` |
| `Fl_File_Browser` (no `Fl_File_Icon`, see Known differences) | `include/cfltk/Fl_File_Browser.h`, `src/widgets/Fl_File_Browser.c` |
| `Fl_Tooltip` | `include/cfltk/Fl_Tooltip.h`, `src/core/Fl_Tooltip.c` |
| `fl_ask.H` common dialogs (`fl_message`/`fl_alert`/`fl_ask`/`fl_choice`/`fl_choice_n`/`fl_input`/`fl_password`) | `include/cfltk/fl_ask.h`, `src/dialogs/fl_ask.c` |
| `Fl_Input_Choice` | `include/cfltk/Fl_Input_Choice.h`, `src/widgets/Fl_Input_Choice.c` |
| `Fl_File_Input` | `include/cfltk/Fl_File_Input.h`, `src/widgets/Fl_File_Input.c` |

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
- **The plain Makefile tracks header dependencies (`-MMD -MP`).** Editing
  a widely-`#include`d header like `Fl_Valuator.h` (e.g. adding a field)
  must trigger a rebuild of every `.c` that includes it, not just `.c`
  files whose own mtime changed -- otherwise `make` links stale `.o`s
  compiled against the old struct layout into the same library as fresh
  ones compiled against the new layout. That mismatch doesn't fail the
  build; it segfaults at runtime through a garbage function pointer or
  corrupted field, which is what happened here (adding `Fl_Valuator`'s
  `value_damage` field). CMake's generator already tracks this
  correctly; the plain Makefile didn't until this was found the hard
  way -- worth remembering if a future header edit causes a build that
  succeeds but a binary that crashes.

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
- **`'@'`-symbol labels** (`fl_draw_symbol()`, `src/draw/fl_symbols.c`)
  are recognized by `fl_label_draw()`/`fl_label_measure()` only at the
  very start and/or very end of the whole (single-line) label text --
  e.g. `"@< Prev"` or `"Next @>"` -- matching the two cases upstream's
  `fl_draw()` detects, but not upstream's per-line detection inside a
  wrapped multi-line label (cfltk's label engine doesn't wrap at all,
  see above). A detected symbol is always drawn at `fl_height()` square
  (the font's line height), laid out immediately beside the remaining
  text with no gap, exactly matching upstream's single-line spacing.
  `"@@"` at the very start correctly suppresses leading-symbol
  detection (matching upstream's escape), but an embedded `"@@"`
  elsewhere in running text is not collapsed to one literal `'@'`
  (upstream's full multi-line `expand_text_()` engine does this;
  out of scope here). The vertex/matrix layer's own matrix stack and
  point buffer (`src/draw/fl_draw.c`) are file-static rather than
  per-driver-instance state, since cfltk's `Fl_Graphics_Driver` is a
  stateless shared vtable; not thread-safe, matching upstream (FLTK
  drawing is single-threaded by design). `fl_begin_complex_polygon()`/
  `fl_gap()` don't actually support multiple contours/holes --
  `fl_end_complex_polygon()` fills the same way `fl_end_polygon()`
  does -- unexercised rather than narrowed, since none of the ~35
  symbols in `fl_symbols.c` (nor upstream's own set) ever call
  `fl_gap()`. `"returnarrow"` is not registered in the symbol table:
  `Fl_Return_Button` already has its own private, working
  `fl_return_arrow()` static helper for its own decoration; exposing
  it under this name too (for the rare case of typing `@returnarrow`
  manually in an arbitrary label) is out of scope for this pass.
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
- **Bug fix: a window's own background box was drawn at its screen
  position instead of its local origin.** `Fl_Group_draw_children()`
  (`Fl_Group.c`) draws its own box at `(self_w->x, self_w->y)`, which is
  correct for a non-window widget (whose `x()`/`y()` are already local
  to the drawable it's painted into) but was also being used, unfixed,
  for widgets that are themselves windows -- whose `x()`/`y()` are their
  *screen* position (used to place the native X window), not an offset
  within their own drawable. Any top-level window not created at
  `(0,0)` had its background box drawn shifted by its own screen
  position and clipped at the drawable's edges (child widgets were
  unaffected, since their coordinates are already window-local).
  Reproduced and confirmed fixed via a throwaway window at `(150,100)`
  before/after screenshots. Found while implementing `Fl_Tooltip`'s
  popup window, which is never at `(0,0)`; also fixed the same latent
  (previously masked) instance in `fl_menu_popup.c`'s `popup_draw()`.
- **`Fl_Tooltip`**: no automatic word-wrap to `wrap_width()` -- a
  tooltip line wider than `wrap_width()` is capped/clipped rather than
  reflowed onto more lines; explicit `'\n'` still starts a new line.
  `enabled()`/`enable()` use their own dedicated static flag rather than
  a general `Fl::option()` mechanism, matching how `Fl_visible_focus()`/
  `Fl_scrollbar_size()` already do this in `Fl.h`. The popup window
  reuses the same border-0/override-redirect trick as the menu popup
  engine (`fl_menu_popup.c`) instead of a real `Fl_Menu_Window`.
- **Bug fix: moving a window with a `resizable_widget` also shifted its
  children.** `Fl_Group_resize()` (`Fl_Group.c`) was missing upstream's
  `type() >= FL_WINDOW` guard (`src/Fl_Group.cxx`): a widget's children
  store window-local coordinates that must NOT change just because the
  enclosing top-level window is repositioned on screen (only the
  window's own `x()`/`y()`, used to place the native X window, should
  move) -- but cfltk's port unconditionally shifted every child by the
  window's move delta in both the plain-translate and the proportional-
  cascade branches. Harmless for a window with no `resizable_widget`
  (the common case), but any window that both sets one *and* gets
  moved after layout -- e.g. `fl_ask.c`'s dialog, positioned by
  `Fl_Window_hotspot()` right after `resizeform()` lays out 1-3 buttons
  -- had every child silently offset by the window's own hotspot delta,
  pushing buttons below the visible drawable (invisible, not just
  misdrawn). Fixed by zeroing the translation applied to children when
  `Fl_Widget_as_window(self_w)` is non-NULL, matching upstream's
  `dx = dy = 0` exactly. Found and reproduced while implementing
  `fl_ask.c`'s `fl_choice()` (3 visible buttons; `fl_message()`'s single
  button happened not to trigger the proportional-cascade branch the
  same way, which is why this stayed latent until now).
- **`Fl_Group_resize()` also gained upstream's `dw==0 && dh==0`
  shortcut** (a pure move, no size change): previously ANY resize call
  with a `resizable_widget` set took the full proportional-cascade-
  from-cached-`sizes_` path even for a pure translation, discarding any
  layout applied to children after the group's last real (size-
  changing) resize and replacing it with a stale recompute. Found
  alongside the bug above, in the same `Fl_Window_hotspot()` call path.
- **`fl_ask.h` common dialogs** (`fl_message`/`fl_alert`/`fl_ask`/
  `fl_choice`/`fl_choice_n`/`fl_input`/`fl_password`): a single reused
  dialog window (`makeform()`/`innards()`/`resizeform()`, translated
  near-verbatim from `src/fl_ask.cxx`), positioned via the new
  `Fl_Window_hotspot()`/`Fl_Window_hotspot_widget()` (`Fl_Window.c`,
  ported from `src/Fl_Window_hotspot.cxx`; the widget-overload skips
  upstream's `o->window()` accumulation loop across nested window
  boundaries, not a supported/tested configuration here) and a new
  `fl_backend_query_pointer()`/`Fl_get_mouse()` (live `XQueryPointer`,
  independent of the last dispatched event) plus `Fl_screen_xywh()`/
  `Fl_screen_work_area()` (single-monitor-at-origin, matching the
  existing convention in `fl_menu_popup.c`). No `Fl::grab()` save/
  restore around showing the dialog (cfltk has no tracked "current
  grab" at the `Fl::` level) and the dialog is not truly modal (no
  `Fl::modal()` event-redirection stack) -- other windows stay
  independently clickable while a dialog is open, but the call still
  blocks the caller until the dialog closes, which is what real callers
  depend on. `size_range()` isn't called (not implemented for
  `Fl_Window`, and moot without an interactive WM-driven resize path).
  `fl_message_hotspot()` is split into a setter and
  `fl_message_hotspot_get()` since C has no overloading. `fl_measure()`
  (declared in `fl_draw.h` since the vertex/matrix work but never
  implemented until now) backs `resizeform()`'s layout math: `'\n'`-
  split line metrics plus the same leading/trailing `@symbol` detection
  as `fl_label_draw()`, no mnemonic/`'&'` handling (not needed for raw
  text metrics).
- **Bug fix: windows never dispatched `FL_SHOW` through their widget
  tree on first display.** `Fl_Widget_default_show()`'s `FL_SHOW`
  dispatch is guarded by `if (!Fl_Widget_visible(self))` (matching
  upstream's own `Fl_Widget::show()`) -- but a freshly constructed
  widget already starts visible (never explicitly hidden), so that
  guard always skipped the dispatch for a window's very first
  `show()`. Upstream avoids this because `Fl_Window::show()` doesn't
  go through the base `Fl_Widget::show()` at all: `Fl_X::make_xid()`
  (`src/Fl_x.cxx`) calls `win->handle(FL_SHOW)` *unconditionally* right
  after mapping, explicitly commented "get child windows to appear".
  `Fl_Group::handle()`'s own `FL_SHOW`/`FL_HIDE` case (already ported
  correctly, see `Fl_Group_handle()` in `Fl_Group.c`) then cascades
  the event to every visible child. Without the unconditional dispatch,
  anything relying on `FL_SHOW` to start itself -- so far, only
  `Fl_Clock`'s 1-second ticker -- silently never started. Fixed in
  `Fl_Window_show()` (`Fl_Window.c`): dispatch `FL_SHOW` unconditionally
  whenever the native window is actually (re)created, which in cfltk
  is every real show() (`Fl_Window_hide()` destroys, not just unmaps,
  the native window, so there's no "already-created, just remap" case
  to distinguish from upstream's rarer one). Reproduced and confirmed
  fixed by cropping the clock face and diffing two screenshots 8
  seconds apart (second hand visibly advances only after the fix).
- **`Fl_Progress`**: a direct, small translation (draw() only); no
  behavioral differences from upstream.
- **`Fl_Spinner`**: `input_`/`up_button_`/`down_button_` are heap-
  allocated and added as ordinary children instead of embedded by
  value in the struct (see `Fl_Spinner.h`'s "Ownership" note) --
  required because cfltk's group teardown always `free()`s every
  child, which would corrupt the heap for embedded-by-value storage;
  invisible to callers.
- **`Fl_Clock`**: a direct translation, including the vertex/matrix-
  drawn hands/tick-marks (`fl_push_matrix`/`fl_rotate`/`fl_vertex`/
  `fl_circle`, from the `fl_draw_symbol()` work above) -- no
  behavioral differences from upstream. `Fl_Digital_Clock` was never
  implemented upstream either ("not yet implemented", see
  `FL_DIGITAL_CLOCK` in `Fl_Clock.h`).
- **`Fl_Input_Choice`**: `inp_`/`menu_` are heap-allocated and added as
  ordinary children instead of embedded by value (same reasoning as
  `Fl_Spinner.h`'s "Ownership" note). The private `InputMenuButton`
  subclass's tiny-triangle `draw()` override lives directly in
  `Fl_Input_Choice.c` (a private `Fl_WidgetOps` table, not exported),
  matching upstream's own private-nested-class scoping.
- **`Fl_File_Input`**: a direct translation of the directory-breadcrumb
  bar and its click-to-truncate-path behavior. `Fl_Input_draw()`
  (`Fl_Input.c`) was refactored to take the box rectangle and the text-
  clip rectangle as two separate parameters (`Fl_Input_draw_text_region()`,
  now exported) instead of always deriving both from the widget's own
  bounds, specifically so `Fl_File_Input_draw()` can draw its box
  shifted down below the breadcrumb bar while still clipping text to
  that same shifted region -- upstream's `Fl_Input_::drawtext()` needed
  no such split since it already took an explicit rectangle throughout.
  No `window()->cursor(FL_CURSOR_INSERT/_DEFAULT)` hinting on
  `FL_MOVE`/`FL_ENTER` (cfltk has no cursor-shape API yet, same
  omission as `Fl_Tile`). `handle_button()`'s press/release visual
  feedback uses `Fl_Widget_redraw()` (deferred to the next flush)
  instead of upstream's synchronous mid-handler `window()->make_current();
  draw_buttons();` -- behaviorally equivalent in cfltk's model, where
  each dispatched event is already followed by exactly one flush
  before the next event is read (see `Fl_wait_for()` in `Fl.c`).
- **`Fl_Dial`'s dot/line indicator** is drawn as a plain trig-computed
  dot/needle from the hub instead of upstream's small rotated polygon
  shapes. This predates `fl_draw.h`'s push/translate/scale/rotate
  transform matrix stack (added for `fl_draw_symbol()`, see above) --
  `Fl_Dial` hasn't been revisited to use it since. The value↔angle
  mapping and drag interaction are translated exactly (verified by
  tracing upstream's `atan2(-my,mx)` mouse-handling formula for the
  cardinal directions); only the indicator's pixel art differs.
  `FL_FILL_DIAL` (the pie meter) has no such difference — it's a
  near-direct translation using `fl_pie()`, which cfltk already has.
- **`Fl_Counter`'s step-button arrows** are drawn as plain triangles via
  `fl_polygon3()` instead of upstream's `"@-4<"`/`"@-4>"` `fl_draw_symbol()`
  glyphs — cfltk hasn't ported the `'@'`-string symbol mini-language
  `fl_draw_symbol()` depends on. Same arrow count/direction/position,
  different pixel art.
- **`Fl_Scrollbar`** has no `"gtk+"` scheme arrow-glyph variant (see the
  "no color schemes" note above) — always draws the plain-scheme
  triangle arrows.
- **`Fl_Valuator` has an optional `value_damage` function-pointer field**
  standing in for upstream's protected virtual `value_damage()` (run
  whenever `value_` changes, so a subclass can resync dependent state --
  `Fl_Value_Input` uses it to refresh its embedded `Fl_Input`'s text,
  `Fl_Adjuster` uses it as a no-op since its appearance never depends on
  the numeric value). `Fl_Valuator` has no vtable of its own to hang a
  real virtual off of (see its own "none of its own" vtable note above),
  so this one field is the exception; every other valuator leaves it
  NULL and gets the default "mark `FL_DAMAGE_EXPOSE`" behavior.
- **`Fl_Value_Input` embeds a real `Fl_Input` as a plain struct member,
  not a normal `Fl_Group` child** -- reproducing upstream's own
  self-described "kludge": `Fl_Value_Input` is an `Fl_Valuator`, not an
  `Fl_Group`, so the embedded input's `parent` pointer is force-set to
  `(Fl_Group *)self` after undoing the automatic add `Fl_Input_init()`
  performs. This is safe in cfltk for the same structural reason
  `FL_WIDGET()` is safe everywhere else: `Fl_Group` and `Fl_Valuator`
  both start with `Fl_Widget widget` as their first member, so any code
  that only ever dereferences `parent->widget` (window()-lookup and
  visible_r()/active_r() parent-chain walks, redraw bubbling) sees the
  right bytes regardless of which one the pointer *actually* points to.
  The one operation that would NOT be safe -- `Fl_Group_add()`/`_remove()`
  reading/writing real `Fl_Group` fields (the children array) through
  that fake pointer -- never runs against it: `Fl_Value_Input_destroy()`
  clears `input.widget.parent` back to NULL before calling
  `Fl_Input_destroy()`, mirroring upstream's destructor un-kludge
  exactly. See `Fl_Value_Input.h` for the full writeup.
- **`Fl_Adjuster`'s three speed buttons** are drawn with 1/2/3 plain
  triangles via `fl_polygon3()` instead of upstream's three distinct
  embedded XBM bitmap glyphs (fastarrow/mediumarrow/slowarrow) -- the
  same kind of image-art substitution already used for `Fl_Counter`'s
  step buttons, for the same reason (no image support yet).
- **`Fl_Tabs`** drops upstream's `Fl_Tooltip::current()`/`::enter()` calls
  in its `FL_MOVE` handler -- cfltk has no tooltip subsystem (see above);
  `FL_MOVE` falls through to plain `Fl_Group_handle()`, losing only the
  per-tab-hover tooltip switch, not any layout/value/click behavior.
- **`Fl_Scroll` has no accelerated "shift already-drawn pixels, redraw
  only the newly exposed strip" blit** (upstream's `fl_scroll()`, backed
  by a platform copy-area primitive cfltk hasn't ported).
  `FL_DAMAGE_SCROLL` just redraws the whole visible content area instead
  -- correct pixels, less efficient. Its `ScrollInfo`/`recalc_scrollbars()`
  are also kept file-private (upstream exposes them `protected` for
  subclasses); promote them to the header if a future cfltk subclass
  needs them. See `Fl_Scroll.h` for the embedded-scrollbar-ownership
  writeup (how it stays safe to `free()` only the real, heap-allocated
  children and never the two always-embedded `Fl_Scrollbar` members).
- **Fixed while building `Fl_Scroll`: Xft text ignored the clip stack.**
  `fl_x11_driver.c`'s `apply_clip()` only called `XSetClipRectangles()`
  on the plain X11 GC; text is drawn separately through an `XftDraw`
  object (`d_draw_text()`, via `XftDrawStringUtf8()`) that has its own,
  independent clip state and was never being told about it. Every prior
  widget happened not to have labels positioned outside their own clip
  region, so this was invisible until `Fl_Scroll` clipped a grid of
  labeled boxes to a small viewport -- child labels past the visible
  edge rendered anyway, right through box borders that *were* correctly
  clipped (box fills/borders go through the GC, not Xft). Fixed by also
  calling `XftDrawSetClipRectangles()`/`XftDrawSetClip(...,None)` in
  lockstep with the GC clip. Any future backend (NuttX/NX) implementing
  its own text renderer needs the equivalent: whatever draws text must
  honor the same clip state as whatever draws shapes.
- **`Fl_Browser_`'s pure-virtual `item_*()` methods become a second,
  item-level vtable** (`Fl_Browser_ItemOps`) alongside the normal
  widget-level `Fl_WidgetOps` -- this class genuinely has two orthogonal
  axes of virtual behavior (how it draws/handles as a *widget*, shared
  by every concrete browser and implemented once in `Fl_Browser_.c`;
  and how its *items* are stored/measured/drawn, which is what
  `Fl_Browser`, `Fl_Check_Browser`, and any future `Fl_File_Browser` --
  actually varies). Optional item-ops entries left NULL fall back to
  the same default behavior the corresponding upstream virtual's
  default body provides. See `Fl_Browser_.h`.
- **`Fl_Browser` has no icon support** (`Fl_Browser::icon()`, and the
  `Fl_Image *icon` field every upstream `FL_BLINE` carries) -- cfltk
  has no `Fl_Image` yet (see "No images" above). `FL_BLINE` has no icon
  field; `item_height()`/`item_width()`/`item_draw()` skip the
  icon-measurement/drawing branches entirely. The '@'-prefixed inline
  text-format codes (font/size/color/alignment/underline/strikethrough/
  background-fill) are fully ported, though -- that mini-language only
  needed `fl_font`/`fl_color`/`fl_line`/`fl_rectf`/`fl_label_draw`,
  which cfltk already has, unlike the separate `fl_draw_symbol()`
  '@'-glyph language used for e.g. `Fl_Counter`'s arrows, which is not
  ported.
- **`Fl_Check_Browser`'s checkbox outline uses `fl_rect()`** in place of
  upstream's `fl_loop(x,y, x,y2, x2,y2, x2,y)` (a general unfilled
  closed-polygon primitive cfltk doesn't have) -- the four points form a
  plain axis-aligned square in this one call site, so `fl_rect()`
  produces the identical outline. Every other line is a direct port,
  including the quirky-but-intentional detail that a line's
  `selected()` state (the normal browser highlight bar) is never
  actually set true by this class -- clicking a line only toggles its
  checkbox, it never shows a selection bar, matching upstream exactly
  (see the header's class-conversion note for how that falls out of
  `item_select()`'s "no-op on val==0" trick combined with `deselect()`
  always being called first on push).
- **`Fl_Browser::load(filename)` is not ported** -- a thin wrapper over
  `fopen()`/`fgets()`/`add()` a caller can trivially write against the
  public `add()` API; no client needs it yet.
- **Fixed while building `Fl_Browser`: text measurement crashed before
  any window was shown.** `fl_graphics_driver()` returned NULL until
  `Fl_Widget_show()` on some window ran `fl_backend_init()` as a side
  effect -- fine for widgets that only measure/draw text from their own
  `draw()`, but `Fl_Browser_add()` calls `item_height()`/`item_width()`
  (hence `fl_font()`) immediately, at insertion time, which is normal
  FLTK usage: populate widgets, `show()` the window, then `Fl_run()`.
  Building the browser example crashed on the very first `add()` call
  before any window was on screen. Fixed by making `fl_graphics_driver()`
  itself lazily call `fl_backend_init()` on first use (`src/draw/fl_draw.c`),
  matching upstream's implicit `fl_open_display()`-on-first-use behavior --
  the display connection and font system have no real dependency on any
  particular window existing.
- **Upstream's multi-segment `fl_xyline`/`fl_yxline` overloads** (the
  4/5-argument forms that draw a connected two- or three-segment path —
  e.g. vertical-then-horizontal — in one call) have no cfltk equivalent;
  `fl_draw.h` only provides the single-segment 3-argument forms. Every
  translated `.c` file that used a multi-arg overload (`Fl_Return_Button`,
  `Fl_Roller`) manually decomposes it into two or three single-segment
  calls. Watch for this when porting any new upstream file that draws
  multi-segment outlines this way.
- **New UTF-8 infrastructure (`fl_utf8.h`/`fl_utf8.c`)** was added
  specifically for `Fl_Text_Buffer`, whose position/indexing model is
  UTF-8-native by design (`char_at()` decodes a codepoint,
  `next_char()`/`prev_char()` step by codepoint not byte). Nothing
  before this needed it -- every other cfltk widget so far treats text
  as opaque bytes. Scope is deliberately narrow: byte-level decode/
  encode/sequence-length (`fl_utf8decode`/`fl_utf8encode`/`fl_utf8len1`,
  ported from `src/fl_utf8.cxx`'s `ERRORS_TO_CP1252` default behavior)
  plus ASCII-only `fl_tolower`/`fl_toupper` for case-insensitive search
  -- full Unicode case-folding would require porting a large per-script
  table upstream itself generates from `XUtf8Tolower()`, out of
  proportion to what any current client needs.
- **`Fl_Text_Buffer`'s undo is a single global slot, not a per-buffer
  stack** -- this is upstream's own actual design (file-static
  `undobuffer`/`undowidget`/etc. in `Fl_Text_Buffer.cxx`, shared by
  whichever buffer was modified most recently), ported verbatim
  including the surprising cross-buffer-invalidation behavior it
  implies (undoing in buffer B after editing buffer A does nothing
  useful once A's edit has overwritten the slot). Not a cfltk
  simplification -- a faithful port of a real upstream quirk.
- **`Fl_Text_Buffer::insertfile()`/`outputfile()` and loadfile
  derivatives are not ported** -- thin `fopen`/`fread`/`fwrite`
  wrappers a caller can write against the public `insert()`/`text()`
  API; no client needs them yet.
- **`Fl_Text_Display`/`Fl_Text_Editor` known differences** (see the
  header comments in `Fl_Text_Display.h`/`Fl_Text_Editor.h` for full
  detail): no drag-and-drop (the `DRAG_START_DND` path and `FL_DND_*`
  event cases are dropped -- cfltk has no DND on any backend); no
  custom mouse cursor shapes over the text area (cfltk's `Fl_Window`
  has no cursor-shape API yet); no printing-surface awareness; no
  system beep. `Fl_Text_Display`'s two scrollbars are real
  heap-allocated `Fl_Scrollbar*` children (`Fl_Scrollbar_new()`,
  auto-added like any other widget) rather than the embedded-struct
  pattern used for `Fl_Scroll`/`Fl_Browser_`'s scrollbars -- upstream's
  own `Fl_Text_Display` genuinely allocates them with `new` too, so
  there's no destroy-ordering kludge to replicate here.
- **Fixed while building `Fl_Text_Editor`: dropping `Fl::compose()`
  entirely broke typing any Shift-produced character.** Upstream's
  `handle_key()` checks `Fl::compose()` *first*, before ever consulting
  the key-binding table; for ordinary printable text (including
  Shift-produced capitals and symbols) it short-circuits straight to
  inserting `Fl::event_text()`. The separate `kf_default()`/
  `default_key_function_` fallback path -- which an early draft
  wrongly treated as the *only* text-insertion mechanism -- is gated by
  `if (default_key_function_ && !state)`, so it never fires when Shift
  is held; it exists to catch unmodified control-ish keys like Tab, not
  to be the general typing path. The fix ports `Fl::compose()`'s actual
  non-Apple classification logic (`text_key_state()` in
  `Fl_Text_Editor.c`: true for plain printable/high-bit text, false for
  control characters and for Alt/Meta/Ctrl-modified non-high-bit keys)
  ahead of the key-binding dispatch. What's genuinely not ported is
  only the persistent XIM dead-key/CJK composition state machine
  (`Fl::compose_state`, marked-text underlining) -- true multi-keystroke
  IME composition -- since cfltk has no XIM/IME layer on any backend.
  Caught by interactive Xephyr testing: typing lowercase letters worked
  immediately, but capitals and shifted symbols (`*`, `/`) silently
  failed to insert until this was fixed.
- **`Fl_Image`/`Fl_RGB_Image`/`Fl_Pixmap`/`Fl_Bitmap` have no cached
  offscreen drawing surface.** Upstream caches a platform image handle
  per object (`id_`/`mask_`, an `Fl_Offscreen`/server-side Pixmap built
  lazily on first `draw()` and reused after) built on a general
  offscreen-surface subsystem (`fl_create_offscreen`/
  `fl_begin_offscreen`/`fl_end_offscreen`) that nothing else in cfltk
  needs yet. Rather than build that whole subsystem prematurely (see
  cfltk's own "don't add abstractions beyond what's needed" rule),
  every image type here re-blits straight from its source pixel array
  on every `draw()` call, through two new backend-neutral primitives
  added to `Fl_Graphics_Driver` (see `fl_draw.h`): `draw_image()`/
  `read_image()` (raw RGB(A) blit/read-back, X11 impl: fresh `XImage` +
  `XPutImage`/`XGetImage`, no caching) and `draw_bitmask()` (1-bpp
  stipple fill in the current color, X11 impl: fresh `XCreateBitmapFromData`
  + `XSetStipple`/`XFillRectangle`, no caching). This is
  correctness-identical to upstream, just without the caching
  optimization; `uncache()` is consequently a no-op on every image
  type. 2- and 4-channel (alpha-bearing) `Fl_RGB_Image`s and
  `Fl_Pixmap`'s inherently-binary-alpha XPM output are composited in
  software against `read_image()`'s screen readback -- the same
  "manual composite, no accelerated alpha" fallback upstream's own
  Xlib backend uses when accelerated alpha isn't available (cfltk
  always takes that path).
- **`fl_parse_color()` (new; needed by `Fl_Pixmap`'s XPM color-table
  parsing) only recognizes hex color specs plus a small fixed table of
  common X11 color names** (black/white/red/green/blue/...), not the
  full X11 `rgb.txt` name database upstream's `XParseColor()` resolves
  on Linux. Virtually all real-world XPM files use hex colors, so this
  covers the common case; an unrecognized name (including XPM's own
  "None" transparency marker) falls back to fully transparent, matching
  upstream's own catch-all behavior for unparseable colors.
- **No `Fl_Menu_Item` image-label support.** Upstream's
  `Fl_Image::label(Fl_Menu_Item*)` plugs into `Fl::set_labeltype()`'s
  pluggable labeltype-callback registry, which cfltk's menu items don't
  have (they draw plain text labels only). Image labels on ordinary
  `Fl_Widget`s -- the far more common case, and what toolbar/icon
  buttons actually use -- work fully via `Fl_Widget_set_image()`.
- **Combined text+image label drawing (`fl_label_draw()`) supports
  vertical stacking only** (image above or below text, chosen by
  `FL_ALIGN_TEXT_OVER_IMAGE`, or image-only/text-only when the other is
  absent) -- covering icon-only buttons and the common
  text-with-icon-above/below-it case. Not ported: `FL_ALIGN_IMAGE_NEXT_TO_TEXT`
  side-by-side layout, and `@`-prefixed inline symbol glyphs in labels
  (`fl_draw_symbol()`'s mini-language -- distinct from `Fl_Browser`'s
  own `@`-format codes, which *are* ported).
- **`Fl_PNG_Image`/`Fl_JPEG_Image` are only compiled at all when their
  library is found** (`CFLTK_ENABLE_PNG`/`CFLTK_ENABLE_JPEG` in
  CMakeLists.txt, `CFLTK_ENABLE_PNG`/`CFLTK_ENABLE_JPEG` make variables
  in the Makefile, both auto-detected via `pkg-config` and defaulting
  to on when found) -- this is a build-time source-inclusion decision,
  not an internal `#ifdef HAVE_LIBPNG` the way upstream does it, since
  a NuttX/embedded target may want the whole library dependency gone,
  not just stubbed to a no-op. When built, `CFLTK_HAVE_PNG`/
  `CFLTK_HAVE_JPEG` are defined on the `cfltk` target (and propagate to
  anything linking it) so client code can `#ifdef` around their use,
  as `examples/loaders/loaders.c` does.
- **`Fl_PNG_Image`/`Fl_JPEG_Image`'s in-memory-buffer constructors
  don't auto-register with `Fl_Shared_Image`** the way upstream's do
  (`Fl_Shared_Image` is implemented, see above, but nothing wires these
  two loaders' in-memory-buffer constructors into its registry). The
  decode itself is fully ported.
- **Fixed while porting `Fl_JPEG_Image`: upstream's in-memory JPEG
  reader has no real bounds checking.** Upstream hand-rolls its own
  `jpeg_mem_src()` whose `fill_input_buffer()` always claims exactly
  4096 more bytes are available regardless of the actual buffer size,
  silently reading past the end for any input under ~4KB or not
  block-aligned. This translation uses libjpeg's own standard
  `jpeg_mem_src(cinfo, buffer, size)` (an explicit-length, bounds-safe
  API present in libjpeg-turbo and all IJG releases >= 8) instead of
  reimplementing the unsafe version -- consistent with this project's
  practice of fixing real bugs found while translating rather than
  faithfully reproducing them (compare the `Fl_Text_Editor` `compose()`
  fix and the `Fl_GIF_Image` EOF-check fix below).
- **Fixed while porting `Fl_GIF_Image`: upstream's `if (i<0)` EOF check
  is dead code.** `NEXTBYTE` casts `getc()`'s result to `uchar` before
  it can ever be compared as negative, so `EOF` (-1) silently becomes
  255 and is never detected there -- on a truncated file, upstream
  loops reading synthetic 255 bytes rather than reporting
  `ERR_FORMAT`. This translation checks `feof()` directly instead, a
  minor robustness fix with no behavior change for any well-formed GIF.
- **`Fl_BMP_Image`/`Fl_GIF_Image`/`Fl_PNG_Image`/`Fl_JPEG_Image` write
  to stderr instead of calling `Fl::warning()`/`Fl::error()` on
  decode failure** -- cfltk has neither yet. `Fl_Image_fail()` still
  reports the correct `ERR_FILE_ACCESS`/`ERR_FORMAT` code either way,
  so this only affects the diagnostic message, not error handling.
- **`Fl_Shared_Image`'s `.xbm`/`.xpm` *text-file* auto-detection is not
  ported** (upstream dispatches these via `Fl_XBM_Image`/
  `Fl_XPM_Image`, neither of which exists in cfltk -- `Fl_Bitmap`/
  `Fl_Pixmap` already load from in-memory `char**`/XBM byte data,
  covering the common compiled-in-icon case). `fl_register_images()`
  covers BMP/GIF/PNG/JPEG via magic-byte sniffing (ported from
  upstream's own `fl_images_core.cxx`), minus `Fl_PNM_Image` (PNM/PPM/
  PGM/PBM -- not ported, no client needs it yet).
- **`Fl_Shared_Image`'s deferred-resize-on-draw `scale()`/
  `scaled_image_` (an FLTK 1.3.4-and-later, `FLTK_ABI_VERSION`-gated
  HiDPI/printing feature) is not ported** -- draw() always delegates
  straight to the wrapped image at its native size, matching upstream's
  own pre-1.3.4 fallback behavior (still upstream's *real*, shipped
  code path, not a cfltk shortcut).
- **`Fl_PNG_Image`/`Fl_JPEG_Image`'s memory-constructor auto-registration
  with `Fl_Shared_Image` (the `friend class` relationship in upstream)
  is not ported** -- consistent with the same note already in
  `Fl_PNG_Image.h`/`Fl_JPEG_Image.h`. Use
  `Fl_Shared_Image_get_from_rgb()` to add an already-loaded image to
  the cache by hand.
- **Fixed while porting `Fl_Shared_Image`: a real null-pointer crash
  caught by interactive testing.** `Fl_Shared_Image::reload()` in
  upstream does `if (alloc_image_) delete image_;` before `image_` has
  ever been assigned on first load -- safe in C++ only because
  `delete nullptr` is a guaranteed no-op there. `Fl_Image_delete()` has
  no such built-in null-safety, so the direct translation of that line
  segfaulted immediately on the very first `Fl_Shared_Image_get()`
  call in `examples/shared_image`. Fixed with an explicit
  `self->wrapped` guard at that one call site (matching the guard
  upstream's own C++ runtime provides implicitly) rather than making
  `Fl_Image_delete()` itself null-tolerant, which would silently mask
  genuine null-pointer bugs at every *other* call site instead of just
  this one legitimate case.
- **`Fl_File_Browser` has no `Fl_File_Icon`** (the vector-icon-per-
  MIME-pattern registry upstream uses to draw a small icon next to
  each entry). Entries list with text only -- this is a graceful
  degradation upstream itself already supports natively: real upstream
  `Fl_File_Browser` checks `Fl_File_Icon::first() == NULL` and falls
  back to text-only rendering with no icon-space reservation whenever
  no icons have been registered, which is unconditionally the case
  here. Consistent with the pre-existing "`Fl_Browser` has no icon
  support" note above. Directories are still sorted first and shown in
  **bold**, matching upstream's own `item_draw()` behavior for that
  part.
- **`Fl_File_Browser_load("")`'s "list all mounted filesystems" mode
  only ports the plain Linux `/etc/mnttab`-or-`/etc/mtab`-or-
  `/etc/fstab`-or-`/etc/vfstab` fallback chain**, not the Windows/OS2/
  macOS/AIX/NetBSD-specific branches, matching `fl_filename.h`'s own
  scope note.
- **Fixed while building and interactively testing
  `examples/file_browser`: the X11 backend never actually implemented
  click-multiplicity detection.** Every `ButtonPress`/`ButtonRelease`
  hardcoded `clicks=1` in the call to `fl_backend_set_event_state()`
  regardless of timing or position, so `Fl_event_clicks()` could never
  distinguish a plain click from a double-click -- nothing before this
  had exercised that path with an actual double-click gesture. Fixed
  by porting (in simplified form) upstream's own `checkdouble()`/
  `set_event_xy()` algorithm from `src/Fl_x.cxx`: a `ButtonPress` of
  the same button within 3px and 1000ms of the previous one increments
  a click counter (0 = plain click, 1 = double-click, 2 = triple-
  click, ...); anything else resets it to 0. `ButtonRelease` reuses the
  count from the `ButtonPress` that started the click, matching
  upstream (release never recomputes it). This is a real backend gap,
  not specific to `Fl_File_Browser` -- any widget relying on
  `Fl_event_clicks()` (word/line-select-on-multi-click in
  `Fl_Text_Display`, double-click-to-navigate here) was affected.
- **Not a bug, but easy to trip over (caught building
  `examples/file_browser`): a plain `Fl_Browser` (`Fl_File_Browser`'s
  base, unchanged) defaults to `FL_NORMAL_BROWSER`, which never
  selects on click or fires its callback at all** -- that is genuinely
  upstream's own behavior; `FL_HOLD_BROWSER`/`FL_SELECT_BROWSER`/
  `FL_MULTI_BROWSER` exist specifically to add click-selection on top
  of the plain type, and callers wanting double-click-to-open behavior
  must also OR `FL_WHEN_ENTER_KEY` into `when()` (FLTK's real, if
  confusingly-named, flag for "also fire the callback on a
  double-click"). `examples/file_browser` sets both explicitly.
- **`Fl_Double_Window`'s offscreen buffer is a plain X `Pixmap` +
  `XCopyArea()`, never the Xdbe (X double-buffer extension) path
  upstream tries first.** This is upstream's own documented fallback
  ("if not [available], it will draw the window data into an
  off-screen pixmap, and then copy it to the on-screen window") and is
  what actually runs on the vast majority of modern X servers anyway
  (Xdbe support is spotty to nonexistent under most compositors), so
  this isn't a narrowed feature so much as always taking the code path
  upstream itself falls back to.
- **How double-buffering was threaded through the X11 backend without
  touching any of `fl_x11_driver.c`'s ~26 drawing call sites**: every
  one of them already drew through `fl_x11_current_target->xid`/`->gc`/
  `->xft_draw` exclusively (never a separate "the real window" handle).
  `Fl_X11_Window` (`fl_x11_internal.h`) now keeps *two* identities: a
  `real_xid`/`real_xft_draw` pair (always the actual mapped window --
  used for map/unmap/move/resize/grab, and as the final blit
  destination) and the original `xid`/`gc`/`xft_draw` fields, which for
  a double-buffered window are redirected to point at an offscreen
  `Pixmap` instead (a single shared `gc` works for both, since X11 GCs
  are depth/visual-scoped, not tied to one specific drawable instance).
  Every existing driver call site keeps working unmodified, because it
  was already only ever touching the "current draw target," which now
  simply might not be the real window. `fl_backend_window_flush()`
  blits the offscreen buffer onto `real_xid` with one `XCopyArea()`
  after drawing, only when double-buffered.
- **Fixed while implementing this: `fl_x11_event.c`'s `find_window()`
  matched incoming X events against `xw->xid`, which -- after the
  double-buffering split above -- points at an offscreen Pixmap for a
  double-buffered window and would never match any real event (events
  always carry the actual window's XID).** This would have silently
  broken all input for every `Fl_Double_Window`. Caught during review
  while making the split (not by a failing test -- the fix landed in
  the same change as the feature, before any interactive testing could
  have hit it), by checking every other place `xw->xid` was read and
  reasoning about which ones need "the current draw target" (all of
  `fl_x11_driver.c`) versus "the real window" (this one).
- **No `Fl_Overlay_Window`** (a specialized double-buffered window with
  an additional transparent overlay drawing layer, historically used
  for e.g. rubber-band selection rectangles). No established use case
  for it yet; can be added on top of the same offscreen-buffer
  machinery `Fl_Double_Window` now has if needed.
- **`Fl_Menu_Window`'s hardware-overlay-plane support is a no-op**
  (`set_overlay()`/`clear_overlay()`/`overlay()` toggle the
  `FL_WIDGET_NO_OVERLAY` widget flag but nothing reads it). Hardware
  overlay planes are a 1990s X11 server feature essentially
  unavailable on any modern compositing display; upstream's own
  overlay path already silently falls back to normal drawing when the
  server doesn't support it, which is effectively always true today.
- **`Fl_Tile`'s pre-drag layout snapshot lives as file-static state in
  `Fl_Tile.c`, not a reuse of `Fl_Group`'s own `sizes` cache.** Fl_Tile's
  algorithm needs upstream's exact `4*(children+2)`-int layout (a
  block for the group, a block for the *clipped resizable bounds*,
  then one block per child) -- `Fl_Group_resize()`'s own simplified
  `sizes` cache (see `Fl_Group.h`'s known differences) doesn't carry
  that second block, since `Fl_Group_resize()` computes the resizable's
  clipped bounds a different way. Recomputed fresh at the start of
  every drag gesture rather than lazily cached until an explicit
  invalidation, which sidesteps a real upstream hazard (forget to call
  `init_sizes()` after changing children post-construction and the
  cached layout goes stale) at the cost of one array rebuild per
  drag -- cheap, and only one `Fl_Tile` can be mid-drag at a time
  anyway (matches upstream's own function-static drag-state variables
  in `Fl_Tile::handle()`).
- **No custom mouse cursor shapes while hovering an `Fl_Tile` draggable
  border, and `Fl_Wizard::value()` no longer resets the cursor to
  default on pane switch** -- both are the same pre-existing "no
  cursor-shape API" limitation noted for `Fl_Text_Display`.

## Next phases (not started)

1. The rest of the official FLTK example suite (see the contract's
   "Required Validation Programs" list), each one both a port target
   and a regression check on the core.
2. Automated regression tests (widget lifecycle, event propagation,
   layout/resize, parent/child bookkeeping) — none exist yet; phase 1
   was validated by visual inspection of `examples/hello` only.
3. Dillo integration once the widget set Dillo's `dw::fltk` needs is
   covered.
4. A NuttX/NX backend implementing the exact `src/backend/fl_backend.h`
   seam the X11 backend implements now.

## Reference source

`../reference/fltk-1.3.11-reference/` holds the official FLTK 1.3.11
source (downloaded from
`https://github.com/fltk/fltk/releases/download/release-1.3.11/fltk-1.3.11-source.tar.gz`),
kept read-only as the ground truth for behavior. It is not built and is
not part of cfltk's own source tree.
