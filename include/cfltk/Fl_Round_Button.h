/*
 * cfltk - Fl_Round_Button.h
 *
 * C translation of FLTK 1.3 FL/Fl_Round_Button.H and
 * FL/Fl_Radio_Round_Button.H (consolidated, same rationale as
 * Fl_Light_Button.h/Fl_Radio_Light_Button).
 *
 * Original class : Fl_Round_Button : public Fl_Light_Button
 *                   (constructor-only: box(FL_NO_BOX), down_box(FL_ROUND_DOWN_BOX),
 *                   selection_color(FL_FOREGROUND_COLOR)); Fl_Radio_Round_Button :
 *                   public Fl_Round_Button (constructor-only: type(FL_RADIO_BUTTON)).
 * New C structure : none of its own; reuses struct Fl_Button.
 * Vtbl            : fl_light_button_ops, unchanged from Fl_Light_Button.
 */
#ifndef CFLTK_FL_ROUND_BUTTON_H
#define CFLTK_FL_ROUND_BUTTON_H

#include "cfltk/Fl_Light_Button.h"

#ifdef __cplusplus
extern "C" {
#endif

void Fl_Round_Button_init(Fl_Button *self, int x, int y, int w, int h, const char *label);
Fl_Button *Fl_Round_Button_new(int x, int y, int w, int h, const char *label);

Fl_Button *Fl_Radio_Round_Button_new(int x, int y, int w, int h, const char *label);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_ROUND_BUTTON_H */
