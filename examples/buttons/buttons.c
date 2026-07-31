/*
 * cfltk example: buttons
 * Direct port of FLTK 1.3's test/buttons.cxx.
 */
#include "cfltk/Fl.h"
#include "cfltk/Fl_Window.h"
#include "cfltk/Fl_Button.h"
#include "cfltk/Fl_Return_Button.h"
#include "cfltk/Fl_Repeat_Button.h"
#include "cfltk/Fl_Check_Button.h"
#include "cfltk/Fl_Light_Button.h"
#include "cfltk/Fl_Round_Button.h"

int main(void) {
    Fl_Window *window = Fl_Window_new(0, 0, 320, 130, NULL);

    Fl_Button *b = Fl_Button_new(10, 10, 130, 30, "Fl_Button");
    Fl_Widget_set_tooltip(&b->widget, "This is a Tooltip.");

    Fl_Return_Button_new(150, 10, 160, 30, "Fl_Return_Button");
    Fl_Repeat_Button_new(10, 50, 130, 30, "Fl_Repeat_Button");
    Fl_Light_Button_new(10, 90, 130, 30, "Fl_Light_Button");
    Fl_Round_Button_new(150, 50, 160, 30, "Fl_Round_Button");
    Fl_Check_Button_new(150, 90, 160, 30, "Fl_Check_Button");

    Fl_Group_end(&window->group);
    Fl_Widget_show(FL_WIDGET(window));

    return Fl_run();
}
