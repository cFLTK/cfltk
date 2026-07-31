/*
 * cfltk - Fl_Toggle_Button.h
 *
 * C translation of FLTK 1.3 FL/Fl_Toggle_Button.H.
 *
 * Original class : Fl_Toggle_Button : public Fl_Button (adds no fields,
 *                   no overrides -- the constructor only sets
 *                   type(FL_TOGGLE_BUTTON)).
 * New C structure : none of its own; reuses struct Fl_Button verbatim,
 *                   since there is nothing to add. Fl_Toggle_Button_new()
 *                   is a factory function, not a distinct type.
 * Vtbl            : fl_button_ops (unchanged).
 */
#ifndef CFLTK_FL_TOGGLE_BUTTON_H
#define CFLTK_FL_TOGGLE_BUTTON_H

#include "cfltk/Fl_Button.h"

#ifdef __cplusplus
extern "C" {
#endif

void Fl_Toggle_Button_init(Fl_Button *self, int x, int y, int w, int h, const char *label);
Fl_Button *Fl_Toggle_Button_new(int x, int y, int w, int h, const char *label);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_TOGGLE_BUTTON_H */
