/*
 * cfltk - Fl_Choice.h
 *
 * C translation of FLTK 1.3 FL/Fl_Choice.H.
 *
 * Original class : Fl_Choice : public Fl_Menu_ (own draw()/handle();
 *                   displays the current selection's label inside the
 *                   box with a dropdown arrow, unlike Fl_Menu_Button
 *                   which shows its own label outside the box).
 * New C structure : none of its own; reuses struct Fl_Menu_.
 * Vtbl            : fl_choice_ops.
 * Known differences: no color-scheme-dependent arrow styles (see
 *                    docs/DESIGN.md's "no color schemes" note) -- always
 *                    renders the single default-scheme arrow.
 */
#ifndef CFLTK_FL_CHOICE_H
#define CFLTK_FL_CHOICE_H

#include "cfltk/Fl_Menu_.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const Fl_WidgetOps fl_choice_ops;

void Fl_Choice_init(Fl_Menu_ *self, int x, int y, int w, int h, const char *label);
Fl_Menu_ *Fl_Choice_new(int x, int y, int w, int h, const char *label);

void Fl_Choice_draw(Fl_Widget *self);
int Fl_Choice_handle(Fl_Widget *self, int event);

static inline int Fl_Choice_value(const Fl_Menu_ *self) { return Fl_Menu_value(self); }
int Fl_Choice_set_value(Fl_Menu_ *self, int v);
int Fl_Choice_set_value_item(Fl_Menu_ *self, const Fl_Menu_Item *v);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_CHOICE_H */
