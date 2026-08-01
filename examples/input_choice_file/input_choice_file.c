/*
 * cfltk example: input_choice_file
 * Exercises Fl_Input_Choice and Fl_File_Input.
 */
#include <stdio.h>
#include "cfltk/Fl.h"
#include "cfltk/Fl_Window.h"
#include "cfltk/Fl_Box.h"
#include "cfltk/Fl_Input_Choice.h"
#include "cfltk/Fl_File_Input.h"

static void choice_cb(Fl_Widget *w, void *data) {
    (void)data;
    Fl_Input_Choice *choice = (Fl_Input_Choice *)w;
    printf("Input_Choice value: %s\n", Fl_Input_Choice_value(choice));
    fflush(stdout);
}

static void file_cb(Fl_Widget *w, void *data) {
    (void)data;
    Fl_File_Input *fi = (Fl_File_Input *)w;
    printf("File_Input value: %s\n", Fl_File_Input_value(fi));
    fflush(stdout);
}

int main(void) {
    Fl_Window *window = Fl_Window_new(0, 0, 420, 160, "cfltk input_choice/file_input");

    Fl_Box_new(10, 10, 150, 20, "Fl_Input_Choice:");
    Fl_Input_Choice *choice = Fl_Input_Choice_new(170, 10, 200, 25, NULL);
    Fl_Input_Choice_add(choice, "Red");
    Fl_Input_Choice_add(choice, "Orange");
    Fl_Input_Choice_add(choice, "Yellow");
    Fl_Widget_set_callback(&choice->group.widget, choice_cb, NULL);

    Fl_Box_new(10, 60, 150, 20, "Fl_File_Input:");
    Fl_File_Input *fi = Fl_File_Input_new(10, 90, 400, 40, NULL);
    Fl_File_Input_set_value_str(fi, "/home/user/docs/report.txt");
    Fl_Widget_set_when(&fi->input.widget, FL_WHEN_CHANGED | FL_WHEN_RELEASE);
    Fl_Widget_set_callback(&fi->input.widget, file_cb, NULL);

    Fl_Group_end(&window->group);
    Fl_Widget_show(FL_WIDGET(window));

    return Fl_run();
}
