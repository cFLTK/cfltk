/*
 * cfltk - Fl_Multiline_Input.h
 * C translation of FLTK 1.3 FL/Fl_Multiline_Input.H.
 * Original class : Fl_Multiline_Input : public Fl_Input (constructor-only:
 *                   input_type(FL_MULTILINE_INPUT)).
 * New C structure : none of its own; reuses struct Fl_Input.
 * Known differences: no word-wrap (see Fl_Input.h).
 */
#ifndef CFLTK_FL_MULTILINE_INPUT_H
#define CFLTK_FL_MULTILINE_INPUT_H

#include "cfltk/Fl_Input.h"

#ifdef __cplusplus
extern "C" {
#endif

void Fl_Multiline_Input_init(Fl_Input *self, int x, int y, int w, int h, const char *label);
Fl_Input *Fl_Multiline_Input_new(int x, int y, int w, int h, const char *label);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_MULTILINE_INPUT_H */
