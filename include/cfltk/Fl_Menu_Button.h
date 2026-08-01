/*
 * cfltk - Fl_Menu_Button.h
 *
 * C translation of FLTK 1.3 FL/Fl_Menu_Button.H.
 *
 * Original class : Fl_Menu_Button : public Fl_Menu_ (own draw()/
 *                   handle(); type() selects which mouse button(s) pop
 *                   it up when box() is set, or right-click-anywhere
 *                   behavior when box() is FL_NO_BOX).
 * New C structure : none of its own; reuses struct Fl_Menu_.
 * Vtbl            : fl_menu_button_ops.
 */
#ifndef CFLTK_FL_MENU_BUTTON_H
#define CFLTK_FL_MENU_BUTTON_H

#include "cfltk/Fl_Menu_.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    FL_POPUP1 = 1,
    FL_POPUP2,
    FL_POPUP12,
    FL_POPUP3,
    FL_POPUP13,
    FL_POPUP23,
    FL_POPUP123
};

extern const Fl_WidgetOps fl_menu_button_ops;

void Fl_Menu_Button_init(Fl_Menu_ *self, int x, int y, int w, int h, const char *label);
Fl_Menu_ *Fl_Menu_Button_new(int x, int y, int w, int h, const char *label);

void Fl_Menu_Button_draw(Fl_Widget *self);
int Fl_Menu_Button_handle(Fl_Widget *self, int event);
/* Pops the menu up right now (as if clicked), blocking until dismissed;
 * returns the picked item or NULL. */
const Fl_Menu_Item *Fl_Menu_Button_popup(Fl_Menu_ *self);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_MENU_BUTTON_H */
