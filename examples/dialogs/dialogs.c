/*
 * cfltk example: dialogs
 * Exercises fl_message/fl_alert/fl_choice/fl_input from fl_ask.h.
 */
#include <stdio.h>
#include "cfltk/Fl.h"
#include "cfltk/Fl_Window.h"
#include "cfltk/Fl_Button.h"
#include "cfltk/fl_ask.h"

static void message_cb(Fl_Widget *w, void *data) {
    (void)w; (void)data;
    fl_message("This is an informational message.");
}

static void alert_cb(Fl_Widget *w, void *data) {
    (void)w; (void)data;
    fl_alert("Something went wrong: %d", 42);
}

static void choice_cb(Fl_Widget *w, void *data) {
    (void)w; (void)data;
    int r = fl_choice("Save changes before closing?", "Cancel", "Discard", "Save");
    printf("fl_choice returned %d\n", r);
    fflush(stdout);
}

static void input_cb(Fl_Widget *w, void *data) {
    (void)w; (void)data;
    const char *s = fl_input("What is your name?", "World");
    printf("fl_input returned: %s\n", s ? s : "(cancelled)");
    fflush(stdout);
}

int main(void) {
    Fl_Window *window = Fl_Window_new(0, 0, 220, 190, "cfltk dialogs");

    Fl_Button *b1 = Fl_Button_new(20, 10, 180, 30, "fl_message");
    Fl_Widget_set_callback(&b1->widget, message_cb, NULL);

    Fl_Button *b2 = Fl_Button_new(20, 50, 180, 30, "fl_alert");
    Fl_Widget_set_callback(&b2->widget, alert_cb, NULL);

    Fl_Button *b3 = Fl_Button_new(20, 90, 180, 30, "fl_choice");
    Fl_Widget_set_callback(&b3->widget, choice_cb, NULL);

    Fl_Button *b4 = Fl_Button_new(20, 130, 180, 30, "fl_input");
    Fl_Widget_set_callback(&b4->widget, input_cb, NULL);

    Fl_Group_end(&window->group);
    Fl_Widget_show(FL_WIDGET(window));

    return Fl_run();
}
