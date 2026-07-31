/*
 * cfltk - Fl_Box.h
 *
 * C translation of FLTK 1.3 FL/Fl_Box.H.
 *
 * Original class : Fl_Box : public Fl_Widget
 * New C structure : struct Fl_Box { Fl_Widget widget; }; -- Fl_Box adds no
 *                    fields of its own upstream either, it only overrides
 *                    draw().
 * Vtbl            : fl_box_ops overrides draw() to paint box()+label();
 *                    handle()/resize() fall back to Fl_Widget's defaults.
 * Ownership       : none beyond Fl_Widget's.
 */
#ifndef CFLTK_FL_BOX_H
#define CFLTK_FL_BOX_H

#include "cfltk/Fl_Widget.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Fl_Box {
    Fl_Widget widget;
} Fl_Box;

extern const Fl_WidgetOps fl_box_ops;

void Fl_Box_init(Fl_Box *self, int x, int y, int w, int h, const char *label);
Fl_Box *Fl_Box_new(int x, int y, int w, int h, const char *label);
Fl_Box *Fl_Box_new_with_type(uchar boxtype, int x, int y, int w, int h, const char *label);

void Fl_Box_draw(Fl_Widget *self);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_BOX_H */
