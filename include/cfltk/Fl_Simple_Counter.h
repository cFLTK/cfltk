/*
 * cfltk - Fl_Simple_Counter.h
 * C translation of FLTK 1.3 FL/Fl_Simple_Counter.H.
 * Original class : Fl_Simple_Counter : public Fl_Counter (constructor-only:
 *                   type(FL_SIMPLE_COUNTER)).
 * New C structure : none of its own; reuses struct Fl_Counter.
 */
#ifndef CFLTK_FL_SIMPLE_COUNTER_H
#define CFLTK_FL_SIMPLE_COUNTER_H

#include "cfltk/Fl_Counter.h"

#ifdef __cplusplus
extern "C" {
#endif

Fl_Counter *Fl_Simple_Counter_new(int x, int y, int w, int h, const char *label);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_SIMPLE_COUNTER_H */
