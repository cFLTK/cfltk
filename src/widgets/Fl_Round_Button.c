/*
 * cfltk - Fl_Round_Button.c
 * See include/cfltk/Fl_Round_Button.h for the class-conversion notes.
 */
#include <stdlib.h>

#include "cfltk/Fl_Round_Button.h"

void Fl_Round_Button_init(Fl_Button *self, int x, int y, int w, int h, const char *label) {
    Fl_Light_Button_init(self, x, y, w, h, label);
    Fl_Widget_set_box(&self->widget, FL_NO_BOX);
    Fl_Button_set_down_box(self, FL_ROUND_DOWN_BOX);
    Fl_Widget_set_selection_color(&self->widget, FL_FOREGROUND_COLOR);
}

Fl_Button *Fl_Round_Button_new(int x, int y, int w, int h, const char *label) {
    Fl_Button *self = (Fl_Button *)malloc(sizeof(Fl_Button));
    Fl_Round_Button_init(self, x, y, w, h, label);
    return self;
}

Fl_Button *Fl_Radio_Round_Button_new(int x, int y, int w, int h, const char *label) {
    Fl_Button *self = Fl_Round_Button_new(x, y, w, h, label);
    Fl_Widget_set_type(&self->widget, FL_RADIO_BUTTON);
    return self;
}
