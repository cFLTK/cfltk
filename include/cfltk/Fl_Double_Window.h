/*
 * cfltk - Fl_Double_Window.h
 *
 * C translation of FLTK 1.3 FL/Fl_Double_Window.H.
 *
 * Original class : Fl_Double_Window : public Fl_Window -- a window
 *                   that draws into an offscreen buffer and blits it
 *                   to the screen in one shot, eliminating the
 *                   partial-redraw flicker a directly-drawn-to window
 *                   shows.
 * New C structure : struct Fl_Double_Window { Fl_Window window; },
 *                    embedding Fl_Window as its first member. No
 *                    fields or overridden behavior of its own --
 *                    Fl_Window_init() (called by
 *                    Fl_Double_Window_init()) just gets its
 *                    `double_buffered` field set to 1, and the X11
 *                    backend (fl_x11_window.c) reads that field to
 *                    decide whether to allocate an offscreen Pixmap
 *                    and blit it via XCopyArea() on flush -- see the
 *                    Fl_X11_Window struct in fl_x11_internal.h. Reuses
 *                    fl_window_ops verbatim.
 * Known differences:
 *   - No Xdbe (X double-buffer extension) support -- always uses the
 *     offscreen-Pixmap-plus-XCopyArea fallback upstream itself falls
 *     back to on servers without Xdbe, which is the common case today.
 */
#ifndef CFLTK_FL_DOUBLE_WINDOW_H
#define CFLTK_FL_DOUBLE_WINDOW_H

#include "cfltk/Fl_Window.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Fl_Double_Window {
    Fl_Window window;
} Fl_Double_Window;

void Fl_Double_Window_init(Fl_Double_Window *self, int x, int y, int w, int h, const char *label);
Fl_Double_Window *Fl_Double_Window_new(int x, int y, int w, int h, const char *label);

#ifdef __cplusplus
}
#endif

#endif
