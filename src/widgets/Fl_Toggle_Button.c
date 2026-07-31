/*
 * cfltk - Fl_Toggle_Button.c
 * See include/cfltk/Fl_Toggle_Button.h for the class-conversion notes.
 */
#include <stdlib.h>

#include "cfltk/Fl_Toggle_Button.h"

void Fl_Toggle_Button_init(Fl_Button *self, int x, int y, int w, int h, const char *label) {
    Fl_Button_init(self, x, y, w, h, label);
    Fl_Widget_set_type(&self->widget, FL_TOGGLE_BUTTON);
}

Fl_Button *Fl_Toggle_Button_new(int x, int y, int w, int h, const char *label) {
    Fl_Button *self = (Fl_Button *)malloc(sizeof(Fl_Button));
    Fl_Toggle_Button_init(self, x, y, w, h, label);
    return self;
}
