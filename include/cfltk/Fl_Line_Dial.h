/*
 * cfltk - Fl_Line_Dial.h
 * C translation of FLTK 1.3 FL/Fl_Line_Dial.H.
 * Original class : Fl_Line_Dial : public Fl_Dial (constructor-only:
 *                   type(FL_LINE_DIAL)).
 * New C structure : none of its own; reuses struct Fl_Dial.
 */
#ifndef CFLTK_FL_LINE_DIAL_H
#define CFLTK_FL_LINE_DIAL_H

#include "cfltk/Fl_Dial.h"

#ifdef __cplusplus
extern "C" {
#endif

Fl_Dial *Fl_Line_Dial_new(int x, int y, int w, int h, const char *label);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_LINE_DIAL_H */
