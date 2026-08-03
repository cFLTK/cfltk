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
    char *icon_label_copy; /* owned copy made by Fl_Window_set_icon_label(),
                             * or NULL (falls back to label_copy - see
                             * Fl_Window_set_icon_label()'s own comment) */
    void *backend_data;  /* opaque handle owned by the platform backend */

    int shown;
    Fl_Window *next_shown; /* intrusive list of currently-shown windows,
                               owned by Fl.c (Fl_context_register_window) */

    /* Set by Fl_Double_Window_init(); read by the backend at
     * fl_backend_window_create() time to decide whether to allocate an
     * offscreen draw buffer. See Fl_Double_Window.h. */
    int double_buffered;

    /* Set by Fl_Window_set_size_range() (0 = unset/no constraint in
     * that direction, matching upstream's default). Applied as
     * WM_NORMAL_HINTS min/max size at window-creation time, or
     * immediately via fl_backend_window_relabel()-style live update if
     * the window is already shown when set. */
    int min_w, min_h, max_w, max_h;
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

/* Sets the window's separate taskbar/icon label (X11 WM_ICON_NAME),
 * matching upstream's two-arg Fl_Window::label(title, iconlabel) -
 * split into its own setter here since C has no overloading. Call
 * Fl_Window_set_label() for the title and this for the icon label
 * (order doesn't matter, either can be called first or alone). */
void Fl_Window_set_icon_label(Fl_Window *self, const char *text);
static inline const char *Fl_Window_icon_label(const Fl_Window *self) {
    return self->icon_label_copy ? self->icon_label_copy : Fl_Window_label(self);
}

/* Sets the window's min/max resizable bounds (X11: WM_NORMAL_HINTS'
 * PMinSize/PMaxSize), matching upstream's
 * Fl_Window::size_range(minw,minh,maxw,maxh). 0 means "no constraint
 * in that direction", matching upstream's default. Applied immediately
 * if the window is already shown, otherwise takes effect at the next
 * show(). */
void Fl_Window_set_size_range(Fl_Window *self, int minw, int minh, int maxw, int maxh);

/* Marks this window modal: once shown, it becomes Fl_modal() and
 * input events (push/release/drag/move/keyboard/wheel) targeting any
 * *other* window are dropped until it's hidden again - matching
 * upstream's Fl_Window::set_modal(). Call before show(). */
static inline void Fl_Window_set_modal(Fl_Window *self) {
    self->group.widget.flags |= FL_WIDGET_MODAL;
}
/* Marks this window "non-modal": stays on top like a modal window but
 * does not block input to other windows - matching upstream's
 * Fl_Window::set_non_modal(). */
static inline void Fl_Window_set_non_modal(Fl_Window *self) {
    self->group.widget.flags |= FL_WIDGET_NON_MODAL;
}

/* Forces this window's drawing context current - matches upstream's
 * Fl_Window::make_current(), used before issuing draw/measure calls
 * outside the normal expose/flush callback (e.g. an immediately-drawn
 * cmdline-supplied URL, before the event loop's first real expose).
 * No-op if the window isn't shown yet. */
void Fl_Window_make_current(Fl_Window *self);

static inline unsigned int Fl_Window_border(const Fl_Window *self) {
    return !(self->group.widget.flags & FL_WIDGET_NOBORDER);
}
void Fl_Window_set_border(Fl_Window *self, int b);
static inline void Fl_Window_clear_border(Fl_Window *self) {
    self->group.widget.flags |= FL_WIDGET_NOBORDER;
}

/* Named to match upstream's Fl_Window::set_override() - functionally
 * identical to Fl_Window_set_border(self, 0) here, not a separate
 * mechanism: the X11 backend already sets real override-redirect
 * (XSetWindowAttributes.override_redirect, so the window manager never
 * decorates/reparents the window) for any window with its border
 * cleared, at window-creation time (fl_x11_window.c's
 * fl_backend_window_create()). Call before Fl_Widget_show(). */
static inline void Fl_Window_set_override(Fl_Window *self) {
    Fl_Window_set_border(self, 0);
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
