/*
 * cfltk - Fl_Nice_Slider.h
 * C translation of FLTK 1.3 FL/Fl_Nice_Slider.H.
 * Original class : Fl_Nice_Slider : public Fl_Slider (constructor-only:
 *                   type(FL_VERT_NICE_SLIDER), box(FL_FLAT_BOX)).
 * New C structure : none of its own; reuses struct Fl_Slider.
 */
#ifndef CFLTK_FL_NICE_SLIDER_H
#define CFLTK_FL_NICE_SLIDER_H

#include "cfltk/Fl_Slider.h"

#ifdef __cplusplus
extern "C" {
#endif

Fl_Slider *Fl_Nice_Slider_new(int x, int y, int w, int h, const char *label);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_NICE_SLIDER_H */
