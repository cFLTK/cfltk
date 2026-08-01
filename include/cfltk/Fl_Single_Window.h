/*
 * cfltk - Fl_Single_Window.h
 *
 * C translation of FLTK 1.3 FL/Fl_Single_Window.H.
 *
 * Original class : Fl_Single_Window : public Fl_Window -- upstream's
 *                   own docs describe it as "the same as Fl_Window",
 *                   provided so code can force single-buffering
 *                   (`double_buffered` is already 0 by default -- see
 *                   Fl_Window.h) even on a platform whose default
 *                   window type double-buffers automatically.
 * New C structure : struct Fl_Single_Window { Fl_Window window; },
 *                    embedding Fl_Window as its first member, purely
 *                    for a distinct type name matching upstream's API
 *                    surface -- no behavior differs from a plain
 *                    Fl_Window in cfltk (which never double-buffers
 *                    unless explicitly asked via Fl_Double_Window).
 */
#ifndef CFLTK_FL_SINGLE_WINDOW_H
#define CFLTK_FL_SINGLE_WINDOW_H

#include "cfltk/Fl_Window.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Fl_Single_Window {
    Fl_Window window;
} Fl_Single_Window;

void Fl_Single_Window_init(Fl_Single_Window *self, int x, int y, int w, int h, const char *label);
Fl_Single_Window *Fl_Single_Window_new(int x, int y, int w, int h, const char *label);

#ifdef __cplusplus
}
#endif

#endif
