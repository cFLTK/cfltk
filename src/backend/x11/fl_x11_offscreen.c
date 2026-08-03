/*
 * cfltk - fl_x11_offscreen.c
 *
 * Off-screen drawing surfaces (Fl_Offscreen), backing
 * fl_create_offscreen()/fl_delete_offscreen()/fl_begin_offscreen()/
 * fl_end_offscreen()/fl_copy_offscreen() (see include/cfltk/fl_draw.h).
 *
 * An Fl_Offscreen wraps a Pixmap the same way Fl_X11_Window wraps a real
 * window: a GC (shared/depth-scoped, same convention Fl_X11_Window
 * already uses) and its own XftDraw, since Xft draw contexts are tied to
 * one specific drawable. begin_offscreen()/end_offscreen() simply swap
 * fl_x11_current_target to point at the offscreen's embedded
 * Fl_X11_Window-shaped target -- every one of fl_x11_driver.c's drawing
 * calls already goes through fl_x11_current_target->{xid,gc,xft_draw},
 * so none of them needed to change for this.
 */
#include <stdlib.h>

#include "fl_x11_internal.h"

struct Fl_Offscreen_ {
    /* Reuses Fl_X11_Window's shape purely so fl_x11_current_target can
     * point straight at &off->target without any driver-side special
     * casing; real_xid/real_xft_draw/offscreen* are unused here (an
     * offscreen surface has no real window and is never itself
     * double-buffered). */
    Fl_X11_Window target;
    int w, h;
};

/* Single-level save slot for the target being temporarily replaced by
 * begin_offscreen() -- matches upstream's own fl_window/gc globals,
 * which are likewise not reentrant across a nested
 * begin_offscreen()/end_offscreen() pair. */
static Fl_X11_Window *g_saved_target = NULL;
static int g_offscreen_active = 0;

Fl_Offscreen fl_x11_create_offscreen(int w, int h) {
    struct Fl_Offscreen_ *off;

    if (w < 1) w = 1;
    if (h < 1) h = 1;

    off = (struct Fl_Offscreen_ *)calloc(1, sizeof(struct Fl_Offscreen_));
    if (!off) return NULL;

    off->w = w;
    off->h = h;
    off->target.xid = XCreatePixmap(fl_x11_display, fl_x11_root, (unsigned)w, (unsigned)h,
                                     (unsigned)DefaultDepth(fl_x11_display, fl_x11_screen));
    off->target.gc = XCreateGC(fl_x11_display, off->target.xid, 0, NULL);
    off->target.xft_draw = XftDrawCreate(fl_x11_display, off->target.xid, fl_x11_visual, fl_x11_colormap);

    return (Fl_Offscreen)off;
}

void fl_x11_delete_offscreen(Fl_Offscreen o) {
    struct Fl_Offscreen_ *off = (struct Fl_Offscreen_ *)o;
    if (!off) return;

    if (off->target.xft_draw) XftDrawDestroy(off->target.xft_draw);
    if (off->target.gc) XFreeGC(fl_x11_display, off->target.gc);
    if (off->target.xid) XFreePixmap(fl_x11_display, off->target.xid);
    free(off);
}

void fl_x11_begin_offscreen(Fl_Offscreen o) {
    struct Fl_Offscreen_ *off = (struct Fl_Offscreen_ *)o;
    if (!off) return;

    g_saved_target = fl_x11_current_target;
    g_offscreen_active = 1;
    fl_x11_current_target = &off->target;
}

void fl_x11_end_offscreen(void) {
    if (!g_offscreen_active) return;
    fl_x11_current_target = g_saved_target;
    g_saved_target = NULL;
    g_offscreen_active = 0;
}

void fl_x11_copy_offscreen(int x, int y, int w, int h, Fl_Offscreen o, int srcx, int srcy) {
    struct Fl_Offscreen_ *off = (struct Fl_Offscreen_ *)o;
    if (!off || !fl_x11_current_target || w <= 0 || h <= 0) return;

    XCopyArea(fl_x11_display, off->target.xid, fl_x11_current_target->xid, fl_x11_current_target->gc,
              srcx, srcy, (unsigned)w, (unsigned)h, x, y);
}
