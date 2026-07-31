/*
 * cfltk - Fl_Check_Button.h
 *
 * C translation of FLTK 1.3 FL/Fl_Check_Button.H.
 *
 * Original class : Fl_Check_Button : public Fl_Light_Button
 *                   (constructor-only: box(FL_NO_BOX), down_box(FL_DOWN_BOX),
 *                   selection_color(FL_FOREGROUND_COLOR)).
 * New C structure : none of its own; reuses struct Fl_Button.
 * Vtbl            : fl_light_button_ops, unchanged from Fl_Light_Button.
 */
#ifndef CFLTK_FL_CHECK_BUTTON_H
#define CFLTK_FL_CHECK_BUTTON_H

#include "cfltk/Fl_Light_Button.h"

#ifdef __cplusplus
extern "C" {
#endif

void Fl_Check_Button_init(Fl_Button *self, int x, int y, int w, int h, const char *label);
Fl_Button *Fl_Check_Button_new(int x, int y, int w, int h, const char *label);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_CHECK_BUTTON_H */
