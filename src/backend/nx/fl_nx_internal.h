/*
 * cfltk - fl_nx_internal.h (backend-private)
 *
 * Shared state between fl_nx_window.c, fl_nx_event.c and fl_nx_driver.c.
 * Not installed, not part of the public API.
 *
 * Targets Microwindows' mwin layer (Win32-compatible window management
 * built directly on the Microwindows engine -- CreateWindowEx/
 * ShowWindow/GetMessage/DispatchMessage/WndProc), not the raw Nano-X
 * engine and not the Nano-X client/server (Gr*) protocol. See
 * docs/analysis.md Sec.4 in the ClauCoDillo_NuttX project for why: mwin
 * is the only windowing layer the NuttX apps/graphics/microwindows
 * package currently builds that has its own window/event/damage model
 * (the raw engine has no window concept at all; the Gr* protocol isn't
 * wired into the NuttX package).
 */
#ifndef CFLTK_NX_INTERNAL_H
#define CFLTK_NX_INTERNAL_H

/* NuttX's <unistd.h> (transitively <sys/types.h>) must come before
 * <windows.h>: windows.h pulls in NuttX's own <stdio.h> -> <nuttx/
 * fs/fs.h> -> <nuttx/mutex.h>, which assumes OK/ERROR (defined in
 * sys/types.h) already exist. apps/examples/microwindows/mwdemo_main.c
 * -- the one other NuttX source file in this tree that includes
 * windows.h -- does the exact same thing via its own "uni_std.h"
 * wrapper (a thin #include <unistd.h>); this just does it directly. */
#include <unistd.h>
#include <windows.h>

#include "cfltk/Fl_Window.h"
#include "cfltk/fl_draw.h"
#include "cfltk/fl_colormap.h"

typedef struct Fl_NX_Window {
    HWND hwnd;
    HDC hdc; /* obtained once at create time, held for the window's
              * life (mirrors the X11 backend's persistent GC) rather
              * than a per-WM_PAINT GetDC/ReleaseDC pair -- cfltk's
              * Fl_Graphics_Driver calls draw primitives from
              * fl_backend_window_flush(), not from inside a WndProc's
              * WM_PAINT handler. */
} Fl_NX_Window;

extern int fl_nx_screen_w, fl_nx_screen_h;

/* The window currently being drawn into; every Fl_Graphics_Driver call
 * in fl_nx_driver.c targets this. Set by fl_backend_window_flush()
 * around the Fl_Widget_draw() call, same convention as the X11
 * backend's fl_x11_current_target. */
extern Fl_NX_Window *fl_nx_current_target;

const Fl_Graphics_Driver *fl_nx_graphics_driver(void);
void fl_nx_driver_init(void);

Fl_NX_Window *fl_nx_window_data(Fl_Window *win);
Fl_Window *fl_nx_find_window(HWND hwnd); /* fl_nx_event.c */

/* fl_nx_event.c -- registered as the window class's WndProc in
 * fl_backend_init(); translates WM_* messages and dispatches them into
 * Fl_context_handle(). */
LRESULT CALLBACK fl_nx_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

#endif /* CFLTK_NX_INTERNAL_H */
