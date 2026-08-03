/*
 * cfltk - Fl_Window.h
 *
 * C translation of FLTK 1.3 FL/Fl_Window.H.
 *
 * Original class : Fl_Window : public Fl_Group
 * New C structure : struct Fl_Window { Fl_Group group; ... }, embedding
 *                    Fl_Group (which embeds Fl_Widget) as its first member.
 * Inheritance     : Fl_Window IS-A Fl_Group IS-A Fl_Widget through
 *                    embedding; FL_WIDGET(win), FL_GROUP(win) and plain
 *                    &win->group.widget all yield the same address.
 * Vtbl            : shares Fl_WidgetOps with Fl_Group; draw()/handle()
 *                    default to Fl_Group's, resize()/show()/hide() are
 *                    overridden to also drive the platform backend
 *                    (create the native window, map/unmap it).
 * Ownership       : a window owns its backend handle (created in show(),
 *                    released in destroy()); it owns its children exactly
 *                    like any Fl_Group.
 * Known differences vs upstream:
 *   - No multi-platform Fl_X abstraction; window <-> platform-handle
 *     mapping goes through backend_data (opaque, backend-owned) instead
 *     of a shared native-handle table (see src/backend/fl_backend.h).
 *   - fullscreen(), icon() are not implemented yet; see docs/DESIGN.md.
 *     xclass()/default_xclass() and size_range() are now implemented
 *     (Fl_Window_default_xclass(), Fl_Window_set_size_range()).
 */
#ifndef CFLTK_FL_WINDOW_H
#define CFLTK_FL_WINDOW_H

#include "cfltk/Fl_Group.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Fl_Window {
    Fl_Group group;

    char *label_copy;    /* owned copy made by Fl_Window_set_label(), or NULL */
    void *backend_data;  /* opaque handle owned by the platform backend */

    int shown;
    Fl_Window *next_shown; /* intrusive list of currently-shown windows,
                               owned by Fl.c (Fl_context_register_window) */

    /* Set by Fl_Double_Window_init(); read by the backend at
     * fl_backend_window_create() time to decide whether to allocate an
     * offscreen draw buffer. See Fl_Double_Window.h. */
    int double_buffered;
};

extern const Fl_WidgetOps fl_window_ops;

/* The as_group()/as_window() vtable slots shared by fl_window_ops --
 * exported so any other Fl_Window-based popup with its own custom
 * Fl_WidgetOps (e.g. fl_menu_popup.c, Fl_Tooltip.c, fl_ask.c) can wire
 * them in too. Skipping this makes Fl_Widget_window() wrongly return
 * NULL for every widget inside that popup instead of the popup itself,
 * since Fl_Widget_window() walks up via Fl_Widget_as_window() checks
 * (see Fl_Widget_window() in Fl_Widget.c) -- found while chasing a
 * layout bug in fl_ask.c's dialog (see Fl_Group_resize()'s dw==0/dh==0
 * fix in Fl_Group.c, a related but distinct bug in the same area). */
Fl_Group *Fl_Window_as_group(Fl_Widget *self);
Fl_Window *Fl_Window_as_window(Fl_Widget *self);

/* Constructs a top-level window. Equivalent to Fl_Window(x,y,w,h,label)
 * upstream (position variant) -- the (w,h,label) constructor upstream
 * lets the window manager place the window; pass x=y=0 here and call
 * Fl_Window_set_force_position(win, 0) if you want that behavior once
 * placement hints are implemented. */
void Fl_Window_init(Fl_Window *self, int x, int y, int w, int h, const char *label);
Fl_Window *Fl_Window_new(int x, int y, int w, int h, const char *label);

void Fl_Window_destroy(Fl_Widget *self); /* Fl_WidgetOps.destroy */

void Fl_Window_show(Fl_Widget *self);
void Fl_Window_hide(Fl_Widget *self);
void Fl_Window_resize(Fl_Widget *self, int x, int y, int w, int h);

static inline int Fl_Window_shown(const Fl_Window *self) { return self->shown; }

void Fl_Window_set_label(Fl_Window *self, const char *text);
static inline const char *Fl_Window_label(const Fl_Window *self) {
    return Fl_Widget_label(&self->group.widget);
}

static inline unsigned int Fl_Window_border(const Fl_Window *self) {
    return !(self->group.widget.flags & FL_WIDGET_NOBORDER);
}
void Fl_Window_set_border(Fl_Window *self, int b);
static inline void Fl_Window_clear_border(Fl_Window *self) {
    self->group.widget.flags |= FL_WIDGET_NOBORDER;
}

/* Marks the window (and, transitively, everything inside it) as needing
 * a redraw and a fresh backend blit on the next flush. */
void Fl_Window_flush(Fl_Window *self);

/* Sets the mouse pointer shape shown while over this window (X11
 * backend: XCreateFontCursor()/XDefineCursor() against the window's
 * real_xid, cached per shape - never freed, matching the process-
 * lifetime cache every other cfltk resource cache already uses). A
 * no-op if the window isn't currently shown (no XID to define a
 * cursor on yet) - matches upstream's own documented behavior of
 * silently doing nothing before show(). */
void Fl_Window_set_cursor(Fl_Window *self, Fl_Cursor c);

/* Sets the process-wide default X11 WM_CLASS hint (both the instance
 * and class name slots, matching upstream FLTK's own Fl_X::make_xid(),
 * which duplicates the same xclass string into both) applied to every
 * window created after this call - matches upstream's
 * Fl_Window::default_xclass(const char*) (a static/global setting, not
 * per-window). Affects window-manager taskbar/icon grouping only. Call
 * before creating any windows for it to take effect on all of them. */
void Fl_Window_default_xclass(const char *xclass);
const char *Fl_Window_default_xclass_get(void);

/* Positions the window so that its own point (X,Y) lands at the
 * current (live-queried) mouse position, clamped to stay fully
 * on-screen unless offscreen is set. Mirrors upstream's
 * Fl_Window::hotspot(int,int,int) (src/Fl_Window_hotspot.cxx). */
void Fl_Window_hotspot(Fl_Window *self, int X, int Y, int offscreen);
/* Same, but (X,Y) is the center of widget `w` (which must be this
 * window or one of its descendants), converted to this window's local
 * coordinate space by walking w's window() chain. Mirrors upstream's
 * Fl_Window::hotspot(const Fl_Widget*,int). */
void Fl_Window_hotspot_widget(Fl_Window *self, const Fl_Widget *w, int offscreen);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_WINDOW_H */
