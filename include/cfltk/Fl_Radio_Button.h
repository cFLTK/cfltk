/*
 * cfltk - Fl_Radio_Button.h
 *
 * C translation of FLTK 1.3 FL/Fl_Radio_Button.H.
 *
 * Original class : Fl_Radio_Button : public Fl_Button (adds nothing;
 *                   constructor only sets type(FL_RADIO_BUTTON)).
 * New C structure : none of its own; reuses struct Fl_Button verbatim.
 * Vtbl            : fl_button_ops (unchanged).
 * Ownership       : Fl_Button_setonly() finds sibling radio buttons by
 *                   walking this widget's parent group, so radio buttons
 *                   only behave as a mutually-exclusive set when they
 *                   share a direct parent -- exactly like upstream.
 */
#ifndef CFLTK_FL_RADIO_BUTTON_H
#define CFLTK_FL_RADIO_BUTTON_H

#include "cfltk/Fl_Button.h"

#ifdef __cplusplus
extern "C" {
#endif

void Fl_Radio_Button_init(Fl_Button *self, int x, int y, int w, int h, const char *label);
Fl_Button *Fl_Radio_Button_new(int x, int y, int w, int h, const char *label);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_RADIO_BUTTON_H */
