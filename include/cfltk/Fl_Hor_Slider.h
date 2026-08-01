/*
 * cfltk - Fl_Hor_Slider.h
 * C translation of FLTK 1.3 FL/Fl_Hor_Slider.H.
 * Original class : Fl_Hor_Slider : public Fl_Slider (constructor-only:
 *                   type(FL_HOR_SLIDER)).
 * New C structure : none of its own; reuses struct Fl_Slider.
 */
#ifndef CFLTK_FL_HOR_SLIDER_H
#define CFLTK_FL_HOR_SLIDER_H

#include "cfltk/Fl_Slider.h"

#ifdef __cplusplus
extern "C" {
#endif

Fl_Slider *Fl_Hor_Slider_new(int x, int y, int w, int h, const char *label);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_HOR_SLIDER_H */
