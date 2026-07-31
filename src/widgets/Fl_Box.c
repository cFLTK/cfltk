/*
 * cfltk - Fl_Box.c
 * See include/cfltk/Fl_Box.h for the class-conversion notes.
 * Translated from src/Fl_Box.cxx.
 */
#include <stdlib.h>

#include "cfltk/Fl_Box.h"
#include "cfltk/fl_draw.h"

const Fl_WidgetOps fl_box_ops = {
    Fl_Box_draw,
    NULL, /* handle: Fl_Widget's default (ignores everything) */
    NULL, /* resize: Fl_Widget's default */
    NULL, /* show: Fl_Widget's default */
    NULL, /* hide: Fl_Widget's default */
    Fl_Widget_base_destroy,
    NULL, /* as_group */
    NULL  /* as_window */
};

void Fl_Box_init(Fl_Box *self, int x, int y, int w, int h, const char *label) {
    Fl_Widget_init(&self->widget, &fl_box_ops, x, y, w, h, label);
}

Fl_Box *Fl_Box_new(int x, int y, int w, int h, const char *label) {
    Fl_Box *self = (Fl_Box *)malloc(sizeof(Fl_Box));
    Fl_Box_init(self, x, y, w, h, label);
    return self;
}

Fl_Box *Fl_Box_new_with_type(uchar boxtype, int x, int y, int w, int h, const char *label) {
    Fl_Box *self = Fl_Box_new(x, y, w, h, label);
    self->widget.box = boxtype;
    return self;
}

void Fl_Box_draw(Fl_Widget *self) {
    if (Fl_Widget_box(self) != FL_NO_BOX) {
        fl_draw_box(Fl_Widget_box(self), self->x, self->y, self->w, self->h, Fl_Widget_color(self));
    }
    Fl_Widget_draw_label(self);
}
