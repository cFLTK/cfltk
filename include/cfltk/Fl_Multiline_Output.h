/*
 * cfltk - Fl_Multiline_Output.h
 * C translation of FLTK 1.3 FL/Fl_Multiline_Output.H.
 * Original class : Fl_Multiline_Output : public Fl_Output (constructor-only:
 *                   type(FL_MULTILINE_OUTPUT)).
 * New C structure : none of its own; reuses struct Fl_Input.
 */
#ifndef CFLTK_FL_MULTILINE_OUTPUT_H
#define CFLTK_FL_MULTILINE_OUTPUT_H

#include "cfltk/Fl_Output.h"

#ifdef __cplusplus
extern "C" {
#endif

void Fl_Multiline_Output_init(Fl_Input *self, int x, int y, int w, int h, const char *label);
Fl_Input *Fl_Multiline_Output_new(int x, int y, int w, int h, const char *label);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_MULTILINE_OUTPUT_H */
