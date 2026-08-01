/*
 * cfltk - fl_x11_window.c
 *
 * X11 native window lifecycle: display connection, top-level window
 * create/show/hide/destroy, and flushing cfltk's drawing into the
 * window. Translated in spirit from src/Fl_x.cxx (Fl_X::make /
 * Fl_Window::show / Fl_Window::flush for the X11 platform).
 */
#include <stdlib.h>
#include <string.h>

#include <X11/XKBlib.h>

#include "fl_x11_internal.h"
#include "../fl_backend.h"

Display *fl_x11_display = NULL;
int fl_x11_screen = 0;
Visual *fl_x11_visual = NULL;
Colormap fl_x11_colormap = 0;
Window fl_x11_root = 0;
Atom fl_x11_wm_delete_window = 0;

static int g_initialized = 0;

int fl_backend_init(void) {
    if (g_initialized) return 1;

    fl_x11_display = XOpenDisplay(NULL);
    if (!fl_x11_display) return 0;

    fl_x11_screen = DefaultScreen(fl_x11_display);
    fl_x11_visual = DefaultVisual(fl_x11_display, fl_x11_screen);
    fl_x11_colormap = DefaultColormap(fl_x11_display, fl_x11_screen);
    fl_x11_root = RootWindow(fl_x11_display, fl_x11_screen);
    fl_x11_wm_delete_window = XInternAtom(fl_x11_display, "WM_DELETE_WINDOW", False);

    /* Without this, a held key generates alternating KeyRelease/KeyPress
     * "repeat" pairs at the same coordinates instead of repeated KeyPress
     * with one real KeyRelease at the end -- indistinguishable from a
     * genuine rapid press/release storm downstream. Matters most for
     * code that (like the menu popup engine) grabs the keyboard. */
    {
        Bool supported = False;
        XkbSetDetectableAutoRepeat(fl_x11_display, True, &supported);
    }

    fl_x11_driver_init();

    g_initialized = 1;
    return 1;
}

void fl_backend_shutdown(void) {
    if (!g_initialized) return;
    XCloseDisplay(fl_x11_display);
    fl_x11_display = NULL;
    g_initialized = 0;
}

Fl_X11_Window *fl_x11_window_data(Fl_Window *win) { return (Fl_X11_Window *)win->backend_data; }

void fl_backend_window_create(Fl_Window *win) {
    Fl_X11_Window *xw;
    XSetWindowAttributes attrs;
    unsigned long mask;
    Fl_Widget *w = FL_WIDGET(win);
    int x = w->x, y = w->y, width = w->w > 0 ? w->w : 1, height = w->h > 0 ? w->h : 1;

    xw = (Fl_X11_Window *)calloc(1, sizeof(Fl_X11_Window));

    memset(&attrs, 0, sizeof(attrs));
    attrs.background_pixel = WhitePixel(fl_x11_display, fl_x11_screen);
    attrs.border_pixel = BlackPixel(fl_x11_display, fl_x11_screen);
    attrs.colormap = fl_x11_colormap;
    attrs.event_mask = ExposureMask | StructureNotifyMask |
                        ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
                        KeyPressMask | KeyReleaseMask | EnterWindowMask | LeaveWindowMask |
                        FocusChangeMask;
    mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

    xw->xid = XCreateWindow(fl_x11_display, fl_x11_root, x, y, (unsigned)width, (unsigned)height, 0,
                             DefaultDepth(fl_x11_display, fl_x11_screen), InputOutput, fl_x11_visual,
                             mask, &attrs);

    xw->gc = XCreateGC(fl_x11_display, xw->xid, 0, NULL);
    xw->xft_draw = XftDrawCreate(fl_x11_display, xw->xid, fl_x11_visual, fl_x11_colormap);

    XSetWMProtocols(fl_x11_display, xw->xid, &fl_x11_wm_delete_window, 1);

    {
        const char *label = Fl_Window_label(win);
        if (label) XStoreName(fl_x11_display, xw->xid, label);
    }

    if (!Fl_Window_border(win)) {
        XSetWindowAttributes ov;
        ov.override_redirect = True;
        XChangeWindowAttributes(fl_x11_display, xw->xid, CWOverrideRedirect, &ov);
    }

    {
        XSizeHints hints;
        hints.flags = PPosition | PSize;
        hints.x = x; hints.y = y; hints.width = width; hints.height = height;
        XSetWMNormalHints(fl_x11_display, xw->xid, &hints);
    }

    win->backend_data = xw;
}

void fl_backend_window_show(Fl_Window *win) {
    Fl_X11_Window *xw = fl_x11_window_data(win);
    if (!xw) return;
    XMapRaised(fl_x11_display, xw->xid);
    XFlush(fl_x11_display);
}

void fl_backend_window_hide(Fl_Window *win) {
    Fl_X11_Window *xw = fl_x11_window_data(win);
    if (!xw) return;
    XUnmapWindow(fl_x11_display, xw->xid);
    XFlush(fl_x11_display);
}

void fl_backend_window_reshape(Fl_Window *win) {
    Fl_X11_Window *xw = fl_x11_window_data(win);
    Fl_Widget *w = FL_WIDGET(win);
    if (!xw) return;
    XMoveResizeWindow(fl_x11_display, xw->xid, w->x, w->y,
                       (unsigned)(w->w > 0 ? w->w : 1), (unsigned)(w->h > 0 ? w->h : 1));
}

void fl_backend_window_destroy(Fl_Window *win) {
    Fl_X11_Window *xw = fl_x11_window_data(win);
    if (!xw) return;
    if (xw->xft_draw) XftDrawDestroy(xw->xft_draw);
    XFreeGC(fl_x11_display, xw->gc);
    XDestroyWindow(fl_x11_display, xw->xid);
    free(xw);
    win->backend_data = NULL;
    XFlush(fl_x11_display);
}

void fl_backend_window_flush(Fl_Window *win) {
    Fl_X11_Window *xw = fl_x11_window_data(win);
    if (!xw) return;
    fl_x11_current_target = xw;
    fl_push_no_clip();
    Fl_Widget_draw(FL_WIDGET(win));
    fl_pop_clip();
    fl_x11_current_target = NULL;
    XFlush(fl_x11_display);
}

void fl_backend_grab(Fl_Window *win) {
    Fl_X11_Window *xw = fl_x11_window_data(win);
    int tries;
    if (!xw) return;

    /* XMapRaised() is asynchronous; grabbing before the server has
     * actually made the window viewable fails with GrabNotViewable.
     * Force the map to complete, then retry briefly if needed. */
    XSync(fl_x11_display, False);
    for (tries = 0; tries < 20; tries++) {
        if (XGrabPointer(fl_x11_display, xw->xid, False,
                          ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                          GrabModeAsync, GrabModeAsync, None, None, CurrentTime) == GrabSuccess)
            break;
        XSync(fl_x11_display, False);
    }
    XGrabKeyboard(fl_x11_display, xw->xid, False, GrabModeAsync, GrabModeAsync, CurrentTime);
}

void fl_backend_ungrab(void) {
    if (!fl_x11_display) return;
    XUngrabPointer(fl_x11_display, CurrentTime);
    XUngrabKeyboard(fl_x11_display, CurrentTime);
}

void fl_backend_screen_size(int *w, int *h) {
    if (fl_x11_display) {
        *w = DisplayWidth(fl_x11_display, fl_x11_screen);
        *h = DisplayHeight(fl_x11_display, fl_x11_screen);
    } else {
        *w = 1024;
        *h = 768;
    }
}
