/*
 * cfltk - Fl_Fill_Dial.h
 * C translation of FLTK 1.3 FL/Fl_Fill_Dial.H.
 * Original class : Fl_Fill_Dial : public Fl_Dial (constructor-only:
 *                   type(FL_FILL_DIAL)).
 * New C structure : none of its own; reuses struct Fl_Dial.
 */
#ifndef CFLTK_FL_FILL_DIAL_H
#define CFLTK_FL_FILL_DIAL_H

#include "cfltk/Fl_Dial.h"

#ifdef __cplusplus
extern "C" {
#endif

Fl_Dial *Fl_Fill_Dial_new(int x, int y, int w, int h, const char *label);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_FILL_DIAL_H */
