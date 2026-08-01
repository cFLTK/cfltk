/*
 * cfltk - Fl_Hor_Value_Slider.h
 * C translation of FLTK 1.3 FL/Fl_Hor_Value_Slider.H.
 * Original class : Fl_Hor_Value_Slider : public Fl_Value_Slider
 *                   (constructor-only: type(FL_HOR_SLIDER)).
 * New C structure : none of its own; reuses struct Fl_Value_Slider.
 */
#ifndef CFLTK_FL_HOR_VALUE_SLIDER_H
#define CFLTK_FL_HOR_VALUE_SLIDER_H

#include "cfltk/Fl_Value_Slider.h"

#ifdef __cplusplus
extern "C" {
#endif

Fl_Value_Slider *Fl_Hor_Value_Slider_new(int x, int y, int w, int h, const char *label);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_HOR_VALUE_SLIDER_H */
