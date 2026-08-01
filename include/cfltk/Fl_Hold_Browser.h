/*
 * cfltk - Fl_Hold_Browser.h
 * C translation of FLTK 1.3 FL/Fl_Hold_Browser.H.
 * Original class : Fl_Hold_Browser : public Fl_Browser (constructor-only:
 *                   type(FL_HOLD_BROWSER)).
 * New C structure : none of its own; reuses struct Fl_Browser.
 */
#ifndef CFLTK_FL_HOLD_BROWSER_H
#define CFLTK_FL_HOLD_BROWSER_H

#include "cfltk/Fl_Browser.h"

#ifdef __cplusplus
extern "C" {
#endif

Fl_Browser *Fl_Hold_Browser_new(int x, int y, int w, int h, const char *label);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_HOLD_BROWSER_H */
