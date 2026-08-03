/*
 * cfltk - fl_x11_window.c
 *
 * X11 native window lifecycle: display connection, top-level window
 * create/show/hide/destroy, and flushing cfltk's drawing into the
 * window. Translated in spirit from src/Fl_x.cxx (Fl_X::make /
 * Fl_Window::show / Fl_Window::flush for the X11 platform).
 */
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE /* strdup() under strict -std=c99 */
#endif
#include <stdlib.h>
#include <string.h>

#include <X11/XKBlib.h>
#include <X11/cursorfont.h>

#include "fl_x11_internal.h"
#include "../fl_backend.h"

Display *fl_x11_display = NULL;
int fl_x11_screen = 0;
Visual *fl_x11_visual = NULL;
Colormap fl_x11_colormap = 0;
Window fl_x11_root = 0;
Atom fl_x11_wm_delete_window = 0;

static int g_initialized = 0;
static char *g_default_xclass = NULL;

void Fl_Window_default_xclass(const char *xc) {
    free(g_default_xclass);
    g_default_xclass = xc ? strdup(xc) : NULL;
}

const char *Fl_Window_default_xclass_get(void) { return g_default_xclass; }

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

static void resize_offscreen(Fl_Window *win, Fl_X11_Window *xw, int width, int height);

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

    xw->real_xid = XCreateWindow(fl_x11_display, fl_x11_root, x, y, (unsigned)width, (unsigned)height, 0,
                                  DefaultDepth(fl_x11_display, fl_x11_screen), InputOutput, fl_x11_visual,
                                  mask, &attrs);

    xw->gc = XCreateGC(fl_x11_display, xw->real_xid, 0, NULL);
    xw->real_xft_draw = XftDrawCreate(fl_x11_display, xw->real_xid, fl_x11_visual, fl_x11_colormap);

    XSetWMProtocols(fl_x11_display, xw->real_xid, &fl_x11_wm_delete_window, 1);
    fl_x11_dnd_window_created(xw->real_xid);
    fl_x11_xembed_window_created(win, xw->real_xid);

    {
        const char *label = Fl_Window_label(win);
        if (label) XStoreName(fl_x11_display, xw->real_xid, label);
    }
    {
        const char *icon_label = Fl_Window_icon_label(win);
        if (icon_label) XSetIconName(fl_x11_display, xw->real_xid, icon_label);
    }

    if (g_default_xclass) {
        /* WM_CLASS carries both an instance name and a class name;
         * matches upstream's own Fl_X::make_xid() (duplicates the same
         * xclass string into both slots via XChangeProperty on
         * XA_WM_CLASS) rather than trying to derive a separate
         * instance name from argv[0]. */
        XClassHint hint;
        hint.res_name = g_default_xclass;
        hint.res_class = g_default_xclass;
        XSetClassHint(fl_x11_display, xw->real_xid, &hint);
    }

    if (!Fl_Window_border(win)) {
        XSetWindowAttributes ov;
        ov.override_redirect = True;
        XChangeWindowAttributes(fl_x11_display, xw->real_xid, CWOverrideRedirect, &ov);
    }

    {
        XSizeHints hints;
        hints.flags = PPosition | PSize;
        hints.x = x; hints.y = y; hints.width = width; hints.height = height;
        if (win->min_w > 0 || win->min_h > 0) {
            hints.flags |= PMinSize;
            hints.min_width = win->min_w > 0 ? win->min_w : 1;
            hints.min_height = win->min_h > 0 ? win->min_h : 1;
        }
        if (win->max_w > 0 || win->max_h > 0) {
            hints.flags |= PMaxSize;
            hints.max_width = win->max_w > 0 ? win->max_w : 32767;
            hints.max_height = win->max_h > 0 ? win->max_h : 32767;
        }
        XSetWMNormalHints(fl_x11_display, xw->real_xid, &hints);
    }

    if (win->double_buffered) {
        resize_offscreen(win, xw, width, height);
    } else {
        xw->xid = xw->real_xid;
        xw->xft_draw = xw->real_xft_draw;
    }

    win->backend_data = xw;
}

void fl_backend_window_show(Fl_Window *win) {
    Fl_X11_Window *xw = fl_x11_window_data(win);
    if (!xw) return;
    XMapRaised(fl_x11_display, xw->real_xid);
    XFlush(fl_x11_display);
}

void fl_backend_window_hide(Fl_Window *win) {
    Fl_X11_Window *xw = fl_x11_window_data(win);
    if (!xw) return;
    XUnmapWindow(fl_x11_display, xw->real_xid);
    XFlush(fl_x11_display);
}

/* Recreates the offscreen draw buffer at the window's current size --
 * called on show() (first size) and whenever the window is resized.
 * The old buffer's *contents* are not preserved (matches upstream:
 * a resized double-buffered window redraws its full contents anyway,
 * via the resize->damage-all path already in Fl_Group_resize()). */
static void resize_offscreen(Fl_Window *win, Fl_X11_Window *xw, int width, int height) {
    if (width < 1) width = 1;
    if (height < 1) height = 1;
    if (xw->offscreen && xw->offscreen_w == width && xw->offscreen_h == height) return;

    if (xw->offscreen) {
        XftDrawDestroy(xw->offscreen_xft_draw);
        XFreePixmap(fl_x11_display, xw->offscreen);
    }
    xw->offscreen = XCreatePixmap(fl_x11_display, xw->real_xid, (unsigned)width, (unsigned)height,
                                   (unsigned)DefaultDepth(fl_x11_display, fl_x11_screen));
    xw->offscreen_xft_draw = XftDrawCreate(fl_x11_display, xw->offscreen, fl_x11_visual, fl_x11_colormap);
    xw->offscreen_w = width;
    xw->offscreen_h = height;
    xw->xid = xw->offscreen;
    xw->xft_draw = xw->offscreen_xft_draw;
    (void)win;
}

void fl_backend_window_reshape(Fl_Window *win) {
    Fl_X11_Window *xw = fl_x11_window_data(win);
    Fl_Widget *w = FL_WIDGET(win);
    int width, height;
    if (!xw) return;
    width = w->w > 0 ? w->w : 1;
    height = w->h > 0 ? w->h : 1;
    XMoveResizeWindow(fl_x11_display, xw->real_xid, w->x, w->y, (unsigned)width, (unsigned)height);
    if (win->double_buffered) resize_offscreen(win, xw, width, height);
}

void fl_backend_window_relabel(Fl_Window *win) {
    Fl_X11_Window *xw = fl_x11_window_data(win);
    const char *label, *icon_label;
    if (!xw) return;
    label = Fl_Window_label(win);
    if (label) XStoreName(fl_x11_display, xw->real_xid, label);
    icon_label = Fl_Window_icon_label(win);
    if (icon_label) XSetIconName(fl_x11_display, xw->real_xid, icon_label);
}

void fl_backend_window_make_current(Fl_Window *win) {
    Fl_X11_Window *xw = fl_x11_window_data(win);
    if (!xw) return;
    fl_x11_current_target = xw;
}

void fl_backend_window_resize_hints(Fl_Window *win) {
    Fl_X11_Window *xw = fl_x11_window_data(win);
    XSizeHints hints;
    if (!xw) return;
    memset(&hints, 0, sizeof(hints));
    if (win->min_w > 0 || win->min_h > 0) {
        hints.flags |= PMinSize;
        hints.min_width = win->min_w > 0 ? win->min_w : 1;
        hints.min_height = win->min_h > 0 ? win->min_h : 1;
    }
    if (win->max_w > 0 || win->max_h > 0) {
        hints.flags |= PMaxSize;
        hints.max_width = win->max_w > 0 ? win->max_w : 32767;
        hints.max_height = win->max_h > 0 ? win->max_h : 32767;
    }
    XSetWMNormalHints(fl_x11_display, xw->real_xid, &hints);
}

void fl_backend_window_destroy(Fl_Window *win) {
    Fl_X11_Window *xw = fl_x11_window_data(win);
    if (!xw) return;
    if (xw->offscreen) {
        XftDrawDestroy(xw->offscreen_xft_draw);
        XFreePixmap(fl_x11_display, xw->offscreen);
    }
    if (xw->real_xft_draw) XftDrawDestroy(xw->real_xft_draw);
    XFreeGC(fl_x11_display, xw->gc);
    XDestroyWindow(fl_x11_display, xw->real_xid);
    free(xw);
    win->backend_data = NULL;
    XFlush(fl_x11_display);
}

/* Swallows BadDrawable/BadWindow from drawing into a window whose XID
 * became invalid without cfltk's own knowledge - the one real way
 * this can happen is an XEmbed-embedded window (Fl_Window_set_embed_xid())
 * whose embedder disappeared (crashed, exited without reparenting its
 * children back out first): the X server destroys every reparented
 * descendant along with a destroyed parent, but cfltk has no
 * synchronous way to find out before the next flush already tries to
 * draw into it. Same "catch and continue" precedent as
 * fl_x11_driver.c's d_read_image() and fl_x11_dnd.c/fl_x11_xembed.c's
 * own handlers - confirmed necessary by reproducing exactly this
 * scenario (a test embedder reparenting a cfltk window in, then
 * exiting without cleanup), which used to take the whole embedded
 * process down over the embedder's own unrelated exit. */
static int flush_err_handler(Display *d, XErrorEvent *e) { (void)d; (void)e; return 0; }

void fl_backend_window_flush(Fl_Window *win) {
    Fl_X11_Window *xw = fl_x11_window_data(win);
    Fl_Widget *w = FL_WIDGET(win);
    XErrorHandler old = NULL;
    if (!xw) return;

    /* Only embedded windows are actually at risk (see the handler's
     * own comment above) - an ordinary top-level's XID only ever goes
     * away through cfltk's own Fl_Window_hide() teardown, which
     * already stops flushing it first. Guarding unconditionally would
     * also force an XSync() (see below) on every single flush of every
     * window, a real round-trip-per-redraw cost normal, non-embedded
     * windows (the overwhelming majority of cfltk usage) have no
     * reason to pay. */
    if (win->embed_xid) old = XSetErrorHandler(flush_err_handler);

    if (win->double_buffered) resize_offscreen(win, xw, w->w > 0 ? w->w : 1, w->h > 0 ? w->h : 1);

    fl_x11_current_target = xw;
    fl_push_no_clip();
    Fl_Widget_draw(FL_WIDGET(win));
    fl_pop_clip();
    fl_x11_current_target = NULL;

    if (win->double_buffered)
        XCopyArea(fl_x11_display, xw->offscreen, xw->real_xid, xw->gc, 0, 0,
                  (unsigned)xw->offscreen_w, (unsigned)xw->offscreen_h, 0, 0);

    if (win->embed_xid) {
        /* XSync(), not just XFlush(): the drawing calls above are all
         * asynchronous (fire-and-forget) requests, so a resulting
         * BadDrawable/BadWindow only actually reaches this client's
         * event queue - and this temporary handler - once something
         * forces a round-trip. Plain XFlush() doesn't wait for (or
         * guarantee processing of) any error the server sends back,
         * which could otherwise still arrive later, during a
         * subsequent XNextEvent() in the main loop, after `old`
         * (possibly Xlib's own fatal default) is back in place. */
        XSync(fl_x11_display, False);
        XSetErrorHandler(old);
    } else {
        XFlush(fl_x11_display);
    }
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
        if (XGrabPointer(fl_x11_display, xw->real_xid, False,
                          ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                          GrabModeAsync, GrabModeAsync, None, None, CurrentTime) == GrabSuccess)
            break;
        XSync(fl_x11_display, False);
    }
    XGrabKeyboard(fl_x11_display, xw->real_xid, False, GrabModeAsync, GrabModeAsync, CurrentTime);
}

void fl_backend_ungrab(void) {
    if (!fl_x11_display) return;
    XUngrabPointer(fl_x11_display, CurrentTime);
    XUngrabKeyboard(fl_x11_display, CurrentTime);
}

void fl_backend_screen_dpi(float *dpi_x, float *dpi_y) {
    int wmm = 0, hmm = 0, wpx = 0, hpx = 0;
    if (fl_x11_display) {
        wpx = DisplayWidth(fl_x11_display, fl_x11_screen);
        hpx = DisplayHeight(fl_x11_display, fl_x11_screen);
        wmm = DisplayWidthMM(fl_x11_display, fl_x11_screen);
        hmm = DisplayHeightMM(fl_x11_display, fl_x11_screen);
    }
    *dpi_x = (wmm > 0) ? (float)wpx * 25.4f / (float)wmm : 96.0f;
    *dpi_y = (hmm > 0) ? (float)hpx * 25.4f / (float)hmm : 96.0f;
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

void fl_backend_query_pointer(int *x_root, int *y_root) {
    Window root_ret, child_ret;
    int win_x, win_y;
    unsigned int mask;
    if (fl_x11_display && XQueryPointer(fl_x11_display, fl_x11_root, &root_ret, &child_ret,
                                         x_root, y_root, &win_x, &win_y, &mask)) {
        return;
    }
    *x_root = 0;
    *y_root = 0;
}

void fl_backend_beep(int type) {
    if (!fl_backend_init()) return;
    /* FL_BEEP_DEFAULT(0)/FL_BEEP_ERROR(2): louder; everything else:
     * quieter -- matches upstream's XBell(fl_display,100)/(...,50) split. */
    XBell(fl_x11_display, (type == 0 || type == 2) ? 100 : 50);
}

/* ------------------------------------------------------------------ */
/* Cursor shapes (Fl_Window_set_cursor(), see Fl_Window.h)             */
/* ------------------------------------------------------------------ */

static unsigned int x11_cursor_shape(Fl_Cursor c) {
    switch (c) {
        case FL_CURSOR_ARROW:  return XC_left_ptr;
        case FL_CURSOR_CROSS:  return XC_crosshair;
        case FL_CURSOR_WAIT:   return XC_watch;
        case FL_CURSOR_INSERT: return XC_xterm;
        case FL_CURSOR_HAND:   return XC_hand2;
        case FL_CURSOR_HELP:   return XC_question_arrow;
        case FL_CURSOR_MOVE:   return XC_fleur;
        case FL_CURSOR_NS:     return XC_sb_v_double_arrow;
        case FL_CURSOR_WE:     return XC_sb_h_double_arrow;
        case FL_CURSOR_NWSE:   return XC_top_left_corner;
        case FL_CURSOR_NESW:   return XC_top_right_corner;
        case FL_CURSOR_N:      return XC_top_side;
        case FL_CURSOR_NE:     return XC_top_right_corner;
        case FL_CURSOR_E:      return XC_right_side;
        case FL_CURSOR_SE:     return XC_bottom_right_corner;
        case FL_CURSOR_S:      return XC_bottom_side;
        case FL_CURSOR_SW:     return XC_bottom_left_corner;
        case FL_CURSOR_W:      return XC_left_side;
        case FL_CURSOR_NW:     return XC_top_left_corner;
        case FL_CURSOR_DEFAULT:
        default:               return XC_left_ptr;
    }
}

/* One cached X Cursor per shape, created on first use and kept for the
 * process lifetime (same convention as cfltk's font cache) - X cursor
 * objects are cheap, shared, and there are at most ~20 distinct shapes,
 * so there is no reason to ever destroy one. Indexed by Fl_Cursor's
 * raw enum value (0..255, see Enumerations.h) via a sparse lookup
 * rather than a 256-entry array, since only a handful are ever used. */
#define CURSOR_CACHE_SIZE 32
typedef struct { Fl_Cursor shape; Cursor xcursor; } CursorCacheEntry;
static CursorCacheEntry g_cursor_cache[CURSOR_CACHE_SIZE];
static int g_cursor_cache_count = 0;
static Cursor g_none_cursor = 0;

static Cursor x11_cursor_for(Fl_Cursor c) {
    int i;
    Cursor xc;

    if (c == FL_CURSOR_NONE) {
        if (!g_none_cursor) {
            char data = 0;
            Pixmap blank = XCreateBitmapFromData(fl_x11_display, fl_x11_root, &data, 1, 1);
            XColor black;
            memset(&black, 0, sizeof(black));
            g_none_cursor = XCreatePixmapCursor(fl_x11_display, blank, blank, &black, &black, 0, 0);
            XFreePixmap(fl_x11_display, blank);
        }
        return g_none_cursor;
    }

    for (i = 0; i < g_cursor_cache_count; i++)
        if (g_cursor_cache[i].shape == c) return g_cursor_cache[i].xcursor;

    xc = XCreateFontCursor(fl_x11_display, x11_cursor_shape(c));
    if (g_cursor_cache_count < CURSOR_CACHE_SIZE) {
        g_cursor_cache[g_cursor_cache_count].shape = c;
        g_cursor_cache[g_cursor_cache_count].xcursor = xc;
        g_cursor_cache_count++;
    }
    return xc;
}

void Fl_Window_set_cursor(Fl_Window *self, Fl_Cursor c) {
    Fl_X11_Window *xw;
    if (!fl_x11_display || !self) return;
    xw = (Fl_X11_Window *)self->backend_data;
    if (!xw) return; /* not shown yet: nothing to define a cursor on */
    XDefineCursor(fl_x11_display, xw->real_xid, x11_cursor_for(c));
}
