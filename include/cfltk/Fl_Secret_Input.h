/*
 * cfltk - Fl_Secret_Input.h
 * C translation of FLTK 1.3 FL/Fl_Secret_Input.H.
 * Original class : Fl_Secret_Input : public Fl_Input (constructor-only:
 *                   input_type(FL_SECRET_INPUT)).
 * New C structure : none of its own; reuses struct Fl_Input.
 * Known differences: masks with ASCII '*' rather than the Unicode bullet
 *                   (U+2022) upstream uses under Xft; see Fl_Input.h.
 */
#ifndef CFLTK_FL_SECRET_INPUT_H
#define CFLTK_FL_SECRET_INPUT_H

#include "cfltk/Fl_Input.h"

#ifdef __cplusplus
extern "C" {
#endif

void Fl_Secret_Input_init(Fl_Input *self, int x, int y, int w, int h, const char *label);
Fl_Input *Fl_Secret_Input_new(int x, int y, int w, int h, const char *label);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_SECRET_INPUT_H */
