/*
 * cfltk - Fl_Menu_Window.c
 * See include/cfltk/Fl_Menu_Window.h for the class-conversion notes.
 */
#include <stdlib.h>

#include "cfltk/Fl_Menu_Window.h"

void Fl_Menu_Window_init(Fl_Menu_Window *self, int x, int y, int w, int h, const char *label) {
    Fl_Single_Window_init(&self->window, x, y, w, h, label);
}

Fl_Menu_Window *Fl_Menu_Window_new(int x, int y, int w, int h, const char *label) {
    Fl_Menu_Window *self = (Fl_Menu_Window *)malloc(sizeof(Fl_Menu_Window));
    Fl_Menu_Window_init(self, x, y, w, h, label);
    return self;
}
