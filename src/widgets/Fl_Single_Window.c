/*
 * cfltk - Fl_Single_Window.c
 * See include/cfltk/Fl_Single_Window.h for the class-conversion notes.
 */
#include <stdlib.h>

#include "cfltk/Fl_Single_Window.h"

void Fl_Single_Window_init(Fl_Single_Window *self, int x, int y, int w, int h, const char *label) {
    Fl_Window_init(&self->window, x, y, w, h, label);
}

Fl_Single_Window *Fl_Single_Window_new(int x, int y, int w, int h, const char *label) {
    Fl_Single_Window *self = (Fl_Single_Window *)malloc(sizeof(Fl_Single_Window));
    Fl_Single_Window_init(self, x, y, w, h, label);
    return self;
}
