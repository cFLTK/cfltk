/*
 * cfltk - Fl_Roller.h
 *
 * C translation of FLTK 1.3 FL/Fl_Roller.H.
 *
 * Original class : Fl_Roller : public Fl_Valuator (own draw()/handle();
 *                   a rotating drum/wheel knob, dragged along its
 *                   horizontal or vertical axis).
 * New C structure : none of its own beyond Fl_Valuator -- Fl_Roller
 *                   adds no fields upstream either, only draw()/handle(),
 *                   so it's `typedef Fl_Valuator Fl_Roller` in spirit;
 *                   cfltk keeps them as distinct type names for API
 *                   clarity but the layout is identical.
 * Vtbl            : fl_roller_ops.
 */
#ifndef CFLTK_FL_ROLLER_H
#define CFLTK_FL_ROLLER_H

#include "cfltk/Fl_Valuator.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef Fl_Valuator Fl_Roller;

extern const Fl_WidgetOps fl_roller_ops;

void Fl_Roller_init(Fl_Roller *self, int x, int y, int w, int h, const char *label);
Fl_Roller *Fl_Roller_new(int x, int y, int w, int h, const char *label);

void Fl_Roller_draw(Fl_Widget *self);
int Fl_Roller_handle(Fl_Widget *self, int event);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_ROLLER_H */
