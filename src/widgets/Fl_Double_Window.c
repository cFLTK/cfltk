/*
 * cfltk - Fl_Double_Window.c
 * See include/cfltk/Fl_Double_Window.h for the class-conversion notes.
 * Translated from src/Fl_Double_Window.cxx (the platform-independent
 * shape of it -- the actual double-buffering mechanics live in the
 * X11 backend, see fl_x11_window.c).
 */
#include <stdlib.h>

#include "cfltk/Fl_Double_Window.h"

void Fl_Double_Window_init(Fl_Double_Window *self, int x, int y, int w, int h, const char *label) {
    Fl_Window_init(&self->window, x, y, w, h, label);
    self->window.double_buffered = 1;
}

Fl_Double_Window *Fl_Double_Window_new(int x, int y, int w, int h, const char *label) {
    Fl_Double_Window *self = (Fl_Double_Window *)malloc(sizeof(Fl_Double_Window));
    Fl_Double_Window_init(self, x, y, w, h, label);
    return self;
}
