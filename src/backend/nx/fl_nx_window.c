/*
 * cfltk - fl_nx_window.c
 *
 * NuttX/Microwindows-mwin native window lifecycle: engine/mouse/
 * keyboard init, top-level window create/show/hide/destroy, and
 * flushing cfltk's drawing into the window. Modeled on
 * fl_x11_window.c's structure; see fl_nx_internal.h for why mwin
 * (Win32-compatible) rather than the raw engine or the Nano-X
 * client/server protocol.
 *
 * Known gaps, each documented at its own stub below rather than
 * silently omitted: no resize hints, no real screen DPI, no beep.
 * Matches the X11 backend's own precedent of a "known differences"
 * banner instead of pretending these are done. Drag-and-drop is
 * implemented, in fl_nx_dnd.c.
 */
#include <nuttx/config.h> /* must be the first include in any NuttX
                            * source file -- see NuttX's own coding
                            * standard. windows.h (below) transitively
                            * pulls NuttX's <stdio.h> -> <nuttx/fs/fs.h>
                            * -> <nuttx/mutex.h>, which assumes OK/ERROR
                            * are already defined by the time it's
                            * parsed; without this first, that chain
                            * fails with "'OK' undeclared" deep inside a
                            * header this file never asked for. */
#include <stdlib.h>
#include <string.h>

#include "fl_nx_internal.h" /* pulls in windows.h -- must come before
                              * wintern.h below, which assumes windows.h's
                              * typedefs (HWND, BOOL, ...) already exist */
#include "../fl_backend.h"
#include "cfltk/Fl.h" /* Fl_first_window()/Fl_next_window(), used by
                        * fl_nx_find_window() below */

#include <wintern.h> /* MwInitialize() -- not re-exported via windows.h,
                       * but part of the same vendored microwindows tree
                       * every mwin app already builds against. */

int fl_nx_screen_w = 0;
int fl_nx_screen_h = 0;
Fl_NX_Window *fl_nx_current_target = NULL;

static int g_initialized = 0;
static const char k_window_class[] = "CfltkWindow";

int fl_backend_init(void) {
    WNDCLASS wc;

    if (g_initialized) return 1;

    if (MwInitialize() < 0) return 0;

    memset(&wc, 0, sizeof(wc));
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = fl_nx_wndproc;
    wc.hbrBackground = CreateSolidBrush(RGB(192, 192, 192)); /* FL_GRAY-ish */
    wc.lpszClassName = k_window_class;
    RegisterClass(&wc);

    fl_nx_screen_w = GetSystemMetrics(SM_CXSCREEN);
    fl_nx_screen_h = GetSystemMetrics(SM_CYSCREEN);

    fl_nx_driver_init();
    fl_nx_fdwatch_init();

    g_initialized = 1;
    return 1;
}

void fl_backend_shutdown(void) {
    /* No engine teardown yet (GdCloseScreen/GdCloseKeyboard/GdCloseMouse
     * are private to winmain.c, not exposed for reuse here) -- safe for
     * now since every real caller of this is a single-shot embedded
     * app exiting the process right after, which reclaims everything.
     * Revisit if/when cfltk needs to restart the backend mid-process. */
    g_initialized = 0;
}

Fl_NX_Window *fl_nx_window_data(Fl_Window *win) { return (Fl_NX_Window *)win->backend_data; }

Fl_Window *fl_nx_find_window(HWND hwnd) {
    Fl_Window *w;
    for (w = Fl_first_window(); w; w = Fl_next_window(w)) {
        Fl_NX_Window *nw = fl_nx_window_data(w);
        if (nw && nw->hwnd == hwnd) return w;
    }
    return NULL;
}

/* (Re)creates the offscreen draw buffer at the window's current size --
 * called on create()/flush() (first size) and whenever the window is
 * resized. The old buffer's *contents* are not preserved (matches the
 * X11 backend's own resize_offscreen(): a resized double-buffered
 * window redraws its full contents anyway, via the resize->damage-all
 * path already in Fl_Group_resize()). */
static void resize_offscreen(Fl_NX_Window *nw, int width, int height) {
    if (width < 1) width = 1;
    if (height < 1) height = 1;
    if (nw->offscreen_bmp && nw->offscreen_w == width && nw->offscreen_h == height) return;

    if (nw->offscreen_bmp) {
        DeleteObject(nw->offscreen_bmp);
        DeleteDC(nw->offscreen_hdc);
    }
    nw->offscreen_hdc = CreateCompatibleDC(nw->real_hdc);
    nw->offscreen_bmp = CreateCompatibleBitmap(nw->real_hdc, width, height);
    SelectObject(nw->offscreen_hdc, nw->offscreen_bmp);
    nw->offscreen_w = width;
    nw->offscreen_h = height;
    nw->hdc = nw->offscreen_hdc;
}

void fl_backend_window_create(Fl_Window *win) {
    Fl_NX_Window *nw;
    Fl_Widget *w = FL_WIDGET(win);
    int x = w->x, y = w->y, width = w->w > 0 ? w->w : 1, height = w->h > 0 ? w->h : 1;
    DWORD style = Fl_Window_border(win) ? WS_OVERLAPPEDWINDOW : WS_POPUP;
    const char *label = Fl_Window_label(win);

    nw = (Fl_NX_Window *)calloc(1, sizeof(Fl_NX_Window));

    nw->hwnd = CreateWindowEx(0, k_window_class, label ? label : "", style,
                               x, y, width, height, HWND_DESKTOP, NULL, 0, NULL);
    nw->real_hdc = GetDC(nw->hwnd);

    if (win->double_buffered) {
        resize_offscreen(nw, width, height);
    } else {
        nw->hdc = nw->real_hdc;
    }

    win->backend_data = nw;
}

void fl_backend_window_show(Fl_Window *win) {
    Fl_NX_Window *nw = fl_nx_window_data(win);
    if (!nw) return;
    ShowWindow(nw->hwnd, SW_SHOW);
    UpdateWindow(nw->hwnd);
    /* mwin does not move keyboard focus to a newly shown window on its
     * own -- MwInitialize() leaves the internal "DeskTop" root window
     * (a plain DefWindowProc target, see winmain.c) as focuswp forever
     * unless something calls SetFocus() explicitly. Without this, every
     * WM_KEYDOWN/WM_CHAR winevent.c posts goes to that root window's
     * queue -- which nothing here ever reads -- and this window's
     * WndProc never sees a single keystroke, no matter how correctly
     * the rest of the input pipeline works. Found by tracing a real
     * keyboard-input bug all the way from X11 through
     * sim_kbdevent()/nuttxkbd_Read() and confirming each of those
     * genuinely worked before finding this was the actual last gap. */
    SetFocus(nw->hwnd);
}

void fl_backend_window_hide(Fl_Window *win) {
    Fl_NX_Window *nw = fl_nx_window_data(win);
    if (!nw) return;
    ShowWindow(nw->hwnd, SW_HIDE);
}

void fl_backend_window_destroy(Fl_Window *win) {
    Fl_NX_Window *nw = fl_nx_window_data(win);
    if (!nw) return;
    if (nw->offscreen_bmp) {
        DeleteObject(nw->offscreen_bmp);
        DeleteDC(nw->offscreen_hdc);
    }
    ReleaseDC(nw->hwnd, nw->real_hdc);
    DestroyWindow(nw->hwnd);
    free(nw);
    win->backend_data = NULL;
}

void fl_backend_window_reshape(Fl_Window *win) {
    Fl_NX_Window *nw = fl_nx_window_data(win);
    Fl_Widget *w = FL_WIDGET(win);
    int width, height;
    if (!nw) return;
    width = w->w > 0 ? w->w : 1;
    height = w->h > 0 ? w->h : 1;
    MoveWindow(nw->hwnd, w->x, w->y, width, height, TRUE);
    if (win->double_buffered) resize_offscreen(nw, width, height);
}

void fl_backend_window_relabel(Fl_Window *win) {
    Fl_NX_Window *nw = fl_nx_window_data(win);
    const char *label;
    if (!nw) return;
    label = Fl_Window_label(win);
    SetWindowText(nw->hwnd, label ? label : "");
}

void fl_backend_window_make_current(Fl_Window *win) {
    fl_nx_current_target = fl_nx_window_data(win);
}

void fl_backend_window_resize_hints(Fl_Window *win) {
    /* mwin doesn't expose a WM_NORMAL_HINTS-equivalent through its
     * public API in this pass -- min/max size are accepted by cfltk's
     * core (Fl_Window_set_size_range()) but not yet enforced by the
     * window manager itself. Follow-up milestone. */
    (void)win;
}

void fl_backend_window_flush(Fl_Window *win) {
    Fl_NX_Window *nw = fl_nx_window_data(win);
    Fl_Widget *w = FL_WIDGET(win);
    if (!nw) return;

    if (win->double_buffered)
        resize_offscreen(nw, w->w > 0 ? w->w : 1, w->h > 0 ? w->h : 1);

    fl_nx_current_target = nw;
    fl_push_no_clip();
    Fl_Widget_draw(FL_WIDGET(win));
    fl_pop_clip();
    fl_nx_current_target = NULL;

    if (win->double_buffered)
        BitBlt(nw->real_hdc, 0, 0, nw->offscreen_w, nw->offscreen_h,
               nw->offscreen_hdc, 0, 0, SRCCOPY);
}

void fl_backend_grab(Fl_Window *win) {
    Fl_NX_Window *nw = fl_nx_window_data(win);
    if (!nw) return;
    SetCapture(nw->hwnd);
}

void fl_backend_ungrab(void) {
    ReleaseCapture();
}

void fl_backend_screen_size(int *w, int *h) {
    *w = fl_nx_screen_w > 0 ? fl_nx_screen_w : 1024;
    *h = fl_nx_screen_h > 0 ? fl_nx_screen_h : 768;
}

void fl_backend_screen_dpi(float *dpi_x, float *dpi_y) {
    /* mwin doesn't expose the display's physical size in millimeters
     * through its public API -- same 96dpi fallback the X11 backend
     * uses when it can't determine the real value either. */
    *dpi_x = 96.0f;
    *dpi_y = 96.0f;
}

void fl_backend_query_pointer(int *x_root, int *y_root) {
    POINT pt;
    if (GetCursorPos(&pt)) {
        *x_root = pt.x;
        *y_root = pt.y;
        return;
    }
    *x_root = 0;
    *y_root = 0;
}

void fl_backend_beep(int type) {
    /* No system-beep API found in this microwindows tree's public mwin
     * headers (no MessageBeep/Beep). Silently does nothing rather than
     * failing -- matches Fl_beep()'s documented "may do nothing on
     * some platforms" contract. */
    (void)type;
}

/* ------------------------------------------------------------------ */
/* Cursor shapes (Fl_Window_set_cursor())                              */
/* ------------------------------------------------------------------ */

void Fl_Window_set_cursor(Fl_Window *self, Fl_Cursor c) {
    /* mwin declares SetCursor()/LoadCursor() in winuser.h but both are
     * commented "not yet implemented" in this Microwindows tree --
     * there is no working cursor-shape API to call into here at all,
     * unlike fl_x11_driver's real XDefineCursor(). A silent no-op
     * (rather than a crash or a build failure) is the correct behavior
     * until mwin itself grows a real implementation; matches
     * fl_backend_beep()'s "documented gap, not a bug" shape. */
    (void)self;
    (void)c;
}

/* ------------------------------------------------------------------ */
/* Default window class (Fl_Window_default_xclass())                   */
/* ------------------------------------------------------------------ */

/* Real, stateful (not a stub): mwin/NuttX has no WM_CLASS-equivalent
 * concept to actually apply this to, but the getter/setter contract
 * itself is portable and cheap to honor for real, same as
 * fl_x11_window.c's own g_default_xclass. */
static char *g_default_xclass = NULL;

void Fl_Window_default_xclass(const char *xc) {
    free(g_default_xclass);
    g_default_xclass = xc ? strdup(xc) : NULL;
}

const char *Fl_Window_default_xclass_get(void) { return g_default_xclass; }
