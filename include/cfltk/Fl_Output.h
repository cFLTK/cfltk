/*
 * cfltk - Fl_Output.h
 * C translation of FLTK 1.3 FL/Fl_Output.H.
 * Original class : Fl_Output : public Fl_Input (constructor-only:
 *                   type(FL_NORMAL_OUTPUT), i.e. FL_INPUT_READONLY set).
 * New C structure : none of its own; reuses struct Fl_Input.
 */
#ifndef CFLTK_FL_OUTPUT_H
#define CFLTK_FL_OUTPUT_H

#include "cfltk/Fl_Input.h"

#ifdef __cplusplus
extern "C" {
#endif

void Fl_Output_init(Fl_Input *self, int x, int y, int w, int h, const char *label);
Fl_Input *Fl_Output_new(int x, int y, int w, int h, const char *label);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_OUTPUT_H */
