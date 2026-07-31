/*
 * cfltk - Fl_Light_Button.h
 *
 * C translation of FLTK 1.3 FL/Fl_Light_Button.H and
 * FL/Fl_Radio_Light_Button.H (consolidated into one file: the radio
 * variant adds nothing beyond type(FL_RADIO_BUTTON), the same relationship
 * Fl_Radio_Button has to Fl_Button).
 *
 * Original class : Fl_Light_Button : public Fl_Button (own draw()/
 *                   handle(), no extra fields); Fl_Radio_Light_Button :
 *                   public Fl_Light_Button (constructor-only).
 * New C structure : none of its own; reuses struct Fl_Button. The "light"
 *                   vs. pushed-in look is entirely a function of down_box()
 *                   and selection_color(), both already Fl_Button fields.
 * Vtbl            : fl_light_button_ops (own draw/handle; the rest falls
 *                   back to Fl_Widget's defaults). Fl_Round_Button and
 *                   Fl_Check_Button reuse this same vtable and only vary
 *                   the constructor's default box()/down_box()/
 *                   selection_color().
 * Known differences:
 *   - No color-scheme support (upstream's Fl::is_scheme("plastic")/
 *     ("gtk+") branches in draw()), so the light indicator always uses
 *     the plain FL_THIN_DOWN_BOX + fl_pie() rendering. See docs/DESIGN.md.
 */
#ifndef CFLTK_FL_LIGHT_BUTTON_H
#define CFLTK_FL_LIGHT_BUTTON_H

#include "cfltk/Fl_Button.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const Fl_WidgetOps fl_light_button_ops;

void Fl_Light_Button_init(Fl_Button *self, int x, int y, int w, int h, const char *label);
Fl_Button *Fl_Light_Button_new(int x, int y, int w, int h, const char *label);

void Fl_Light_Button_draw(Fl_Widget *self);
int Fl_Light_Button_handle(Fl_Widget *self, int event);

Fl_Button *Fl_Radio_Light_Button_new(int x, int y, int w, int h, const char *label);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_LIGHT_BUTTON_H */
