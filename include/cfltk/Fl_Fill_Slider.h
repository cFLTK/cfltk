/*
 * cfltk - Fl_Fill_Slider.h
 * C translation of FLTK 1.3 FL/Fl_Fill_Slider.H.
 * Original class : Fl_Fill_Slider : public Fl_Slider (constructor-only:
 *                   type(FL_VERT_FILL_SLIDER)).
 * New C structure : none of its own; reuses struct Fl_Slider.
 */
#ifndef CFLTK_FL_FILL_SLIDER_H
#define CFLTK_FL_FILL_SLIDER_H

#include "cfltk/Fl_Slider.h"

#ifdef __cplusplus
extern "C" {
#endif

Fl_Slider *Fl_Fill_Slider_new(int x, int y, int w, int h, const char *label);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_FILL_SLIDER_H */
