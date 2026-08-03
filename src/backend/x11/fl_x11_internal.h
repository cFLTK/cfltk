/*
 * cfltk - fl_x11_internal.h (backend-private)
 * Shared state between fl_x11_window.c, fl_x11_event.c and
 * fl_x11_driver.c. Not installed, not part of the public API.
 */
#ifndef CFLTK_X11_INTERNAL_H
#define CFLTK_X11_INTERNAL_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>

#include "cfltk/Fl_Window.h"
#include "cfltk/fl_draw.h"
#include "cfltk/fl_colormap.h"

typedef struct Fl_X11_Window {
    /* Current draw target -- what every Fl_Graphics_Driver call in
     * fl_x11_driver.c actually draws into. Equal to real_xid/
     * real_xft_draw for an ordinary (single-buffered) window; for a
     * double-buffered window (see Fl_Double_Window.h) these instead
     * point at `offscreen` for the duration of drawing, and
     * fl_backend_window_flush() blits `offscreen` onto real_xid with
     * one XCopyArea() afterward -- eliminating the partial-redraw
     * flicker a directly-drawn-to window shows. Every one of
     * fl_x11_driver.c's ~26 call sites already goes through these two
     * fields, so this needed no changes there at all. */
    Window xid;
    GC gc; /* shared: GCs are depth/visual-scoped in X11, not tied to
            * one specific drawable instance, so the same GC works for
            * both real_xid and an offscreen Pixmap of matching depth. */
    XftDraw *xft_draw;

    /* The actual mapped X11 window: always valid, used for map/unmap/
     * move/resize/property calls and as the final blit destination. */
    Window real_xid;
    XftDraw *real_xft_draw;

    /* Double-buffering (Fl_Double_Window only). offscreen==0 means
     * single-buffered (xid==real_xid, xft_draw==real_xft_draw). */
    Pixmap offscreen;
    XftDraw *offscreen_xft_draw;
    int offscreen_w, offscreen_h;
} Fl_X11_Window;

extern Display *fl_x11_display;
extern int fl_x11_screen;
extern Visual *fl_x11_visual;
extern Colormap fl_x11_colormap;
extern Window fl_x11_root;
extern Atom fl_x11_wm_delete_window;

/* The window currently being drawn into; every Fl_Graphics_Driver call in
 * fl_x11_driver.c targets this. Set by fl_backend_window_flush() around
 * the Fl_Widget_draw() call. */
extern Fl_X11_Window *fl_x11_current_target;

const Fl_Graphics_Driver *fl_x11_graphics_driver(void);
void fl_x11_driver_init(void);

Fl_X11_Window *fl_x11_window_data(Fl_Window *win);
Fl_Window *find_window(Window xid); /* fl_x11_event.c, shared with fl_x11_dnd.c */

/* fl_x11_dnd.c -- Xdnd protocol (drag-and-drop), backing Fl_dnd() and
 * the FL_DND_ENTER/FL_DND_DRAG/FL_DND_LEAVE/FL_DND_RELEASE events
 * dispatched into FL_PASTE on drop. */
void fl_x11_dnd_window_created(Window xid); /* sets XdndAware; call once per real window */
/* Called from dispatch_one()'s ClientMessage/SelectionNotify cases;
 * returns 1 if the event was Xdnd-related and has been handled. */
int fl_x11_dnd_handle_client_message(XEvent *ev);
int fl_x11_dnd_handle_selection_notify(XEvent *ev);
int fl_x11_dnd_handle_selection_request(XEvent *ev);
/* Source side (int fl_backend_dnd_start(const char*, int), implemented
 * directly under that name in fl_x11_dnd.c) declared in
 * ../fl_backend.h, not repeated here. */

/* fl_x11_offscreen.c -- see include/cfltk/fl_draw.h for the public
 * fl_create_offscreen()/... wrappers these back. */
Fl_Offscreen fl_x11_create_offscreen(int w, int h);
void fl_x11_delete_offscreen(Fl_Offscreen o);
void fl_x11_begin_offscreen(Fl_Offscreen o);
void fl_x11_end_offscreen(void);
void fl_x11_copy_offscreen(int x, int y, int w, int h, Fl_Offscreen o, int srcx, int srcy);

#endif /* CFLTK_X11_INTERNAL_H */
