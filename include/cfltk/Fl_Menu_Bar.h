/*
 * cfltk - Fl_Menu_Bar.h
 *
 * C translation of FLTK 1.3 FL/Fl_Menu_Bar.H.
 *
 * Original class : Fl_Menu_Bar : public Fl_Menu_ (own draw()/handle();
 *                   lays out top-level items horizontally and drops
 *                   each one's submenu down below it).
 * New C structure : none of its own; reuses struct Fl_Menu_.
 * Vtbl            : fl_menu_bar_ops.
 */
#ifndef CFLTK_FL_MENU_BAR_H
#define CFLTK_FL_MENU_BAR_H

#include "cfltk/Fl_Menu_.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const Fl_WidgetOps fl_menu_bar_ops;

void Fl_Menu_Bar_init(Fl_Menu_ *self, int x, int y, int w, int h, const char *label);
Fl_Menu_ *Fl_Menu_Bar_new(int x, int y, int w, int h, const char *label);

void Fl_Menu_Bar_draw(Fl_Widget *self);
int Fl_Menu_Bar_handle(Fl_Widget *self, int event);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_MENU_BAR_H */
