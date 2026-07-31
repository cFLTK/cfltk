/*
 * cfltk example: radio
 *
 * Not an upstream port -- a regression check exercising Fl_Button's
 * push/toggle/radio-group interaction end to end (PUSH/DRAG/RELEASE
 * handling, redraw, value(), setonly() finding siblings via the parent
 * group, and callback dispatch driven by when()).
 */
#include <stdio.h>

#include "cfltk/Fl.h"
#include "cfltk/Fl_Window.h"
#include "cfltk/Fl_Box.h"
#include "cfltk/Fl_Button.h"

static Fl_Box *status;

static void report(const char *what) {
    static char buf[128];
    snprintf(buf, sizeof(buf), "%s", what);
    Fl_Widget_set_label(&status->widget, buf);
    Fl_Widget_redraw(&status->widget);
}

static void push_cb(Fl_Widget *w, void *data) {
    (void)w; (void)data;
    report("Fl_Button clicked");
}

static void toggle_cb(Fl_Widget *w, void *data) {
    (void)data;
    report(Fl_Button_value((Fl_Button *)w) ? "Toggle: ON" : "Toggle: OFF");
}

static void radio_cb(Fl_Widget *w, void *data) {
    (void)data;
    report(Fl_Widget_label(w));
}

int main(void) {
    Fl_Window *window = Fl_Window_new(0, 0, 320, 180, NULL);

    Fl_Button *push = Fl_Button_new(10, 10, 130, 30, "Push Me");
    Fl_Widget_set_callback(&push->widget, push_cb, NULL);

    Fl_Button *toggle = Fl_Button_new(10, 50, 130, 30, "Toggle");
    Fl_Widget_set_type(&toggle->widget, FL_TOGGLE_BUTTON);
    Fl_Widget_set_callback(&toggle->widget, toggle_cb, NULL);
    Fl_Widget_set_when(&toggle->widget, FL_WHEN_CHANGED);

    Fl_Button *r1 = Fl_Button_new(160, 10, 150, 30, "Radio A");
    Fl_Widget_set_type(&r1->widget, FL_RADIO_BUTTON);
    Fl_Widget_set_callback(&r1->widget, radio_cb, NULL);

    Fl_Button *r2 = Fl_Button_new(160, 50, 150, 30, "Radio B");
    Fl_Widget_set_type(&r2->widget, FL_RADIO_BUTTON);
    Fl_Widget_set_callback(&r2->widget, radio_cb, NULL);

    Fl_Button *r3 = Fl_Button_new(160, 90, 150, 30, "Radio C");
    Fl_Widget_set_type(&r3->widget, FL_RADIO_BUTTON);
    Fl_Widget_set_callback(&r3->widget, radio_cb, NULL);
    Fl_Button_setonly(r3);

    status = Fl_Box_new(10, 130, 300, 30, "Click a button");
    Fl_Widget_set_box(&status->widget, FL_DOWN_BOX);

    Fl_Group_end(&window->group);
    Fl_Widget_show(FL_WIDGET(window));

    return Fl_run();
}
