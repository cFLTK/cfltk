/*
 * cfltk - Fl_Secret_Input.c
 * See include/cfltk/Fl_Secret_Input.h for the class-conversion notes.
 */
#include <stdlib.h>

#include "cfltk/Fl_Secret_Input.h"

void Fl_Secret_Input_init(Fl_Input *self, int x, int y, int w, int h, const char *label) {
    Fl_Input_init(self, x, y, w, h, label);
    Fl_Input_set_input_type(self, FL_SECRET_INPUT);
}

Fl_Input *Fl_Secret_Input_new(int x, int y, int w, int h, const char *label) {
    Fl_Input *self = (Fl_Input *)malloc(sizeof(Fl_Input));
    Fl_Secret_Input_init(self, x, y, w, h, label);
    return self;
}
