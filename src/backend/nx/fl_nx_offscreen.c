/*
 * cfltk - fl_nx_offscreen.c
 *
 * Off-screen drawing surfaces (Fl_Offscreen), backing
 * fl_create_offscreen()/fl_delete_offscreen()/fl_begin_offscreen()/
 * fl_end_offscreen()/fl_copy_offscreen() (see include/cfltk/fl_draw.h).
 *
 * An Fl_Offscreen wraps a memory HDC/HBITMAP pair the same way
 * Fl_NX_Window wraps a real window's HDC. begin_offscreen()/
 * end_offscreen() simply swap fl_nx_current_target to point at the
 * offscreen's embedded Fl_NX_Window-shaped target -- every one of
 * fl_nx_driver.c's drawing calls already goes through
 * fl_nx_current_target->hdc, so none of them needed to change for
 * this. Modeled directly on fl_x11_offscreen.c.
 */
#include <nuttx/config.h> /* must be first -- see fl_nx_window.c's comment */
#include <stdlib.h>

#include "fl_nx_internal.h"

struct Fl_Offscreen_ {
    /* Reuses Fl_NX_Window's shape purely so fl_nx_current_target can
     * point straight at &off->target without any driver-side special
     * casing; hwnd/real_hdc/offscreen_* are unused here (an offscreen
     * surface has no real window and is never itself double-buffered
     * -- target.hdc *is* the offscreen memory DC). */
    Fl_NX_Window target;
    int w, h;
};

/* Single-level save slot for the target being temporarily replaced by
 * begin_offscreen() -- matches the X11 backend's own g_saved_target,
 * likewise not reentrant across a nested begin_offscreen()/
 * end_offscreen() pair. */
static Fl_NX_Window *g_saved_target = NULL;
static int g_offscreen_active = 0;

Fl_Offscreen fl_nx_create_offscreen(int w, int h) {
    struct Fl_Offscreen_ *off;
    HDC screen_hdc;

    if (w < 1) w = 1;
    if (h < 1) h = 1;

    off = (struct Fl_Offscreen_ *)calloc(1, sizeof(struct Fl_Offscreen_));
    if (!off) return NULL;

    off->w = w;
    off->h = h;

    screen_hdc = GetDC(NULL);
    off->target.hdc = CreateCompatibleDC(screen_hdc);
    off->target.offscreen_bmp = CreateCompatibleBitmap(screen_hdc, w, h);
    SelectObject(off->target.hdc, off->target.offscreen_bmp);
    ReleaseDC(NULL, screen_hdc);

    return (Fl_Offscreen)off;
}

void fl_nx_delete_offscreen(Fl_Offscreen o) {
    struct Fl_Offscreen_ *off = (struct Fl_Offscreen_ *)o;
    if (!off) return;

    if (off->target.offscreen_bmp) DeleteObject(off->target.offscreen_bmp);
    if (off->target.hdc) DeleteDC(off->target.hdc);
    free(off);
}

void fl_nx_begin_offscreen(Fl_Offscreen o) {
    struct Fl_Offscreen_ *off = (struct Fl_Offscreen_ *)o;
    if (!off) return;

    g_saved_target = fl_nx_current_target;
    g_offscreen_active = 1;
    fl_nx_current_target = &off->target;
}

void fl_nx_end_offscreen(void) {
    if (!g_offscreen_active) return;
    fl_nx_current_target = g_saved_target;
    g_saved_target = NULL;
    g_offscreen_active = 0;
}

void fl_nx_copy_offscreen(int x, int y, int w, int h, Fl_Offscreen o, int srcx, int srcy) {
    struct Fl_Offscreen_ *off = (struct Fl_Offscreen_ *)o;
    if (!off || !fl_nx_current_target || w <= 0 || h <= 0) return;

    BitBlt(fl_nx_current_target->hdc, x, y, w, h,
           off->target.hdc, srcx, srcy, SRCCOPY);
}
