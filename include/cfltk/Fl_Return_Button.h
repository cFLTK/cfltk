/*
 * cfltk - Fl_Return_Button.h
 *
 * C translation of FLTK 1.3 FL/Fl_Return_Button.H.
 *
 * Original class : Fl_Return_Button : public Fl_Button (own draw()/
 *                   handle(); handle() additionally fires on FL_SHORTCUT
 *                   when the key is Enter/KP_Enter, no matter where
 *                   focus is -- the reason a dialog's default button is
 *                   usually a Fl_Return_Button).
 * New C structure : none of its own; reuses struct Fl_Button. The
 *                   carriage-return glyph is stateless decoration drawn
 *                   at draw() time, not stored.
 * Vtbl            : fl_return_button_ops (own draw/handle).
 */
#ifndef CFLTK_FL_RETURN_BUTTON_H
#define CFLTK_FL_RETURN_BUTTON_H

#include "cfltk/Fl_Button.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const Fl_WidgetOps fl_return_button_ops;

void Fl_Return_Button_init(Fl_Button *self, int x, int y, int w, int h, const char *label);
Fl_Button *Fl_Return_Button_new(int x, int y, int w, int h, const char *label);

void Fl_Return_Button_draw(Fl_Widget *self);
int Fl_Return_Button_handle(Fl_Widget *self, int event);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_RETURN_BUTTON_H */
