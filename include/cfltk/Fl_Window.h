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
 *   - fullscreen(), icon(), xclass(), size_range(), hotspot() are not
 *     implemented yet; see docs/DESIGN.md.
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

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_WINDOW_H */
