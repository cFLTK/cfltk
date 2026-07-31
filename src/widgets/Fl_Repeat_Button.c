/*
 * cfltk - Fl_Repeat_Button.c
 * See include/cfltk/Fl_Repeat_Button.h for the class-conversion notes.
 * Translated from src/Fl_Repeat_Button.cxx.
 */
#include <stdlib.h>

#include "cfltk/Fl_Repeat_Button.h"
#include "cfltk/Fl.h"

#define INITIAL_REPEAT 0.5
#define REPEAT 0.1

const Fl_WidgetOps fl_repeat_button_ops = {
    Fl_Button_draw,
    Fl_Repeat_Button_handle,
    NULL, NULL, NULL,
    Fl_Widget_base_destroy,
    NULL, NULL
};

void Fl_Repeat_Button_init(Fl_Button *self, int x, int y, int w, int h, const char *label) {
    Fl_Button_init(self, x, y, w, h, label);
    self->widget.ops = &fl_repeat_button_ops;
}

Fl_Button *Fl_Repeat_Button_new(int x, int y, int w, int h, const char *label) {
    Fl_Button *self = (Fl_Button *)malloc(sizeof(Fl_Button));
    Fl_Repeat_Button_init(self, x, y, w, h, label);
    return self;
}

static void repeat_callback(void *data) {
    Fl_Button *self = (Fl_Button *)data;
    Fl_add_timeout(REPEAT, repeat_callback, self);
    Fl_Widget_do_callback(&self->widget);
}

int Fl_Repeat_Button_handle(Fl_Widget *self_w, int event) {
    Fl_Button *self = (Fl_Button *)self_w;
    int newval;

    switch (event) {
        case FL_HIDE:
        case FL_DEACTIVATE:
        case FL_RELEASE:
            newval = 0;
            goto apply;
        case FL_PUSH:
        case FL_DRAG:
            if (Fl_visible_focus()) Fl_set_focus(self_w);
            newval = Fl_event_inside(self_w);
        apply:
            if (!Fl_Widget_active(self_w)) newval = 0;
            if (Fl_Button_set_value(self, newval)) {
                if (newval) {
                    Fl_add_timeout(INITIAL_REPEAT, repeat_callback, self);
                    Fl_Widget_do_callback(self_w);
                } else {
                    Fl_remove_timeout(repeat_callback, self);
                }
            }
            return 1;
        default:
            return Fl_Button_handle(self_w, event);
    }
}
