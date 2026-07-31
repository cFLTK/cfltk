/*
 * cfltk example: hello
 * Direct port of FLTK 1.3's test/hello.cxx.
 */
#include "cfltk/Fl.h"
#include "cfltk/Fl_Window.h"
#include "cfltk/Fl_Box.h"

int main(void) {
    Fl_Window *window = Fl_Window_new(0, 0, 340, 180, NULL);
    Fl_Box *box = Fl_Box_new(20, 40, 300, 100, "Hello, World!");

    Fl_Widget_set_box(&box->widget, FL_UP_BOX);
    Fl_Widget_set_labelfont(&box->widget, FL_BOLD + FL_ITALIC);
    Fl_Widget_set_labelsize(&box->widget, 36);
    Fl_Widget_set_labeltype(&box->widget, FL_SHADOW_LABEL);

    Fl_Group_end(&window->group);
    Fl_Widget_show(FL_WIDGET(window));

    return Fl_run();
}
