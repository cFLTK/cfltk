/*
 * cfltk - Fl_Multiline_Output.c
 * See include/cfltk/Fl_Multiline_Output.h for the class-conversion notes.
 */
#include <stdlib.h>

#include "cfltk/Fl_Multiline_Output.h"

void Fl_Multiline_Output_init(Fl_Input *self, int x, int y, int w, int h, const char *label) {
    Fl_Output_init(self, x, y, w, h, label);
    Fl_Widget_set_type(&self->widget, FL_MULTILINE_OUTPUT);
}

Fl_Input *Fl_Multiline_Output_new(int x, int y, int w, int h, const char *label) {
    Fl_Input *self = (Fl_Input *)malloc(sizeof(Fl_Input));
    Fl_Multiline_Output_init(self, x, y, w, h, label);
    return self;
}
