/*
 * cfltk - Fl_Repeat_Button.h
 *
 * C translation of FLTK 1.3 FL/Fl_Repeat_Button.H.
 *
 * Original class : Fl_Repeat_Button : public Fl_Button (own handle();
 *                   draw() is inherited unchanged from Fl_Button).
 * New C structure : none of its own; reuses struct Fl_Button.
 * Vtbl            : fl_repeat_button_ops (draw falls back to
 *                   Fl_Button_draw; only handle() differs).
 * Ownership       : none beyond Fl_Button's, but note the pending
 *                   Fl_add_timeout() this widget may have registered
 *                   with itself as `data` -- Fl_Repeat_Button_handle()
 *                   removes it on FL_HIDE/FL_DEACTIVATE/FL_RELEASE, and
 *                   Fl_Widget_base_destroy() does NOT remove it, so a
 *                   button destroyed while its repeat timer is armed
 *                   (i.e. destroyed mid-press without going through a
 *                   RELEASE/HIDE/DEACTIVATE first) would leave a
 *                   dangling timer. In practice FL_HIDE fires when its
 *                   window is destroyed, which covers normal shutdown;
 *                   documented here because it's the one sharp edge in
 *                   this widget's lifetime, matching upstream (which has
 *                   the same property: repeat_callback keeps a raw
 *                   Fl_Button* with no liveness check).
 */
#ifndef CFLTK_FL_REPEAT_BUTTON_H
#define CFLTK_FL_REPEAT_BUTTON_H

#include "cfltk/Fl_Button.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const Fl_WidgetOps fl_repeat_button_ops;

void Fl_Repeat_Button_init(Fl_Button *self, int x, int y, int w, int h, const char *label);
Fl_Button *Fl_Repeat_Button_new(int x, int y, int w, int h, const char *label);

int Fl_Repeat_Button_handle(Fl_Widget *self, int event);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_REPEAT_BUTTON_H */
