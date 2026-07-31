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
    Window xid;
    GC gc;
    XftDraw *xft_draw;
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

#endif /* CFLTK_X11_INTERNAL_H */
