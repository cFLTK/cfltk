/*
 * cfltk example: valuators
 *
 * Not a direct upstream port -- exercises the Fl_Valuator family:
 * Fl_Slider (vertical + horizontal), Fl_Value_Slider, Fl_Scrollbar,
 * Fl_Dial (normal/line/fill), Fl_Counter/Fl_Simple_Counter, Fl_Roller,
 * Fl_Value_Input, Fl_Value_Output, and Fl_Adjuster, all wired to a
 * status Fl_Output so values are observable without reading pixels.
 */
#include <stdio.h>

#include "cfltk/Fl.h"
#include "cfltk/Fl_Window.h"
#include "cfltk/Fl_Box.h"
#include "cfltk/Fl_Output.h"
#include "cfltk/Fl_Slider.h"
#include "cfltk/Fl_Hor_Slider.h"
#include "cfltk/Fl_Value_Slider.h"
#include "cfltk/Fl_Scrollbar.h"
#include "cfltk/Fl_Dial.h"
#include "cfltk/Fl_Fill_Dial.h"
#include "cfltk/Fl_Line_Dial.h"
#include "cfltk/Fl_Counter.h"
#include "cfltk/Fl_Simple_Counter.h"
#include "cfltk/Fl_Roller.h"
#include "cfltk/Fl_Value_Input.h"
#include "cfltk/Fl_Value_Output.h"
#include "cfltk/Fl_Adjuster.h"

static Fl_Input *status;

static void report(const char *what, double v) {
    char buf[128];
    snprintf(buf, sizeof(buf), "%s: %.3f", what, v);
    Fl_Input_set_value_str(status, buf);
}

static void slider_cb(Fl_Widget *w, void *data) {
    report((const char *)data, Fl_Valuator_value((Fl_Valuator *)w));
}

static void scrollbar_cb(Fl_Widget *w, void *data) {
    (void)data;
    report("scrollbar", (double)Fl_Scrollbar_value((Fl_Scrollbar *)w));
}

int main(void) {
    Fl_Window *window = Fl_Window_new(0, 0, 480, 470, "cfltk valuators");

    Fl_Box *b1 = Fl_Box_new(20, 10, 200, 20, "Vertical / horizontal sliders");
    Fl_Widget_set_align(&b1->widget, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    Fl_Slider *vslider = Fl_Slider_new(20, 35, 30, 120, NULL);
    Fl_Valuator_set_range(&vslider->valuator, 0.0, 100.0);
    Fl_Valuator_set_value(&vslider->valuator, 50.0);
    Fl_Widget_set_callback(&vslider->valuator.widget, slider_cb, (void *)"v-slider");

    Fl_Slider *hslider = Fl_Hor_Slider_new(70, 80, 200, 25, NULL);
    Fl_Valuator_set_range(&hslider->valuator, 0.0, 100.0);
    Fl_Valuator_set_value(&hslider->valuator, 25.0);
    Fl_Widget_set_callback(&hslider->valuator.widget, slider_cb, (void *)"h-slider");

    Fl_Value_Slider *vs = Fl_Value_Slider_new(70, 115, 200, 25, NULL);
    Fl_Widget_set_type(&vs->slider.valuator.widget, FL_HOR_SLIDER);
    Fl_Valuator_set_range(&vs->slider.valuator, 0.0, 10.0);
    Fl_Valuator_set_value(&vs->slider.valuator, 5.0);
    Fl_Widget_set_callback(&vs->slider.valuator.widget, slider_cb, (void *)"value-slider");

    Fl_Scrollbar *sb = Fl_Scrollbar_new(70, 150, 200, 20, NULL);
    Fl_Widget_set_type(&sb->slider.valuator.widget, FL_HOR_SLIDER);
    Fl_Scrollbar_set_value_range(sb, 0, 10, 0, 100);
    Fl_Widget_set_callback(&sb->slider.valuator.widget, scrollbar_cb, NULL);

    Fl_Box *b2 = Fl_Box_new(300, 10, 160, 20, "Dials");
    Fl_Widget_set_align(&b2->widget, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    Fl_Dial *dial = Fl_Dial_new(300, 35, 60, 60, NULL);
    Fl_Valuator_set_range(&dial->valuator, 0.0, 100.0);
    Fl_Valuator_set_value(&dial->valuator, 30.0);
    Fl_Widget_set_callback(&dial->valuator.widget, slider_cb, (void *)"dial");

    Fl_Dial *linedial = Fl_Line_Dial_new(370, 35, 60, 60, NULL);
    Fl_Valuator_set_range(&linedial->valuator, 0.0, 100.0);
    Fl_Valuator_set_value(&linedial->valuator, 70.0);
    Fl_Widget_set_callback(&linedial->valuator.widget, slider_cb, (void *)"line-dial");

    Fl_Dial *filldial = Fl_Fill_Dial_new(300, 105, 60, 60, NULL);
    Fl_Valuator_set_range(&filldial->valuator, 0.0, 100.0);
    Fl_Valuator_set_value(&filldial->valuator, 45.0);
    Fl_Widget_set_callback(&filldial->valuator.widget, slider_cb, (void *)"fill-dial");

    Fl_Box *b3 = Fl_Box_new(20, 190, 200, 20, "Counters / roller");
    Fl_Widget_set_align(&b3->widget, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    Fl_Counter *counter = Fl_Counter_new(20, 215, 180, 25, NULL);
    Fl_Valuator_set_range(&counter->valuator, 0.0, 100.0);
    Fl_Valuator_set_value(&counter->valuator, 10.0);
    Fl_Widget_set_callback(&counter->valuator.widget, slider_cb, (void *)"counter");

    Fl_Counter *simple = Fl_Simple_Counter_new(20, 250, 180, 25, NULL);
    Fl_Valuator_set_range(&simple->valuator, 0.0, 20.0);
    Fl_Valuator_set_value(&simple->valuator, 3.0);
    Fl_Widget_set_callback(&simple->valuator.widget, slider_cb, (void *)"simple-counter");

    Fl_Roller *roller = Fl_Roller_new(20, 285, 180, 25, NULL);
    Fl_Widget_set_type(&roller->widget, FL_HORIZONTAL);
    Fl_Valuator_set_range(roller, 0.0, 100.0);
    Fl_Valuator_set_value(roller, 50.0);
    Fl_Widget_set_callback(&roller->widget, slider_cb, (void *)"roller");

    Fl_Box *b4 = Fl_Box_new(300, 190, 160, 20, "Value in/out, adjuster");
    Fl_Widget_set_align(&b4->widget, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    Fl_Value_Input *vin = Fl_Value_Input_new(300, 215, 160, 25, NULL);
    Fl_Valuator_set_range(&vin->valuator, 0.0, 100.0);
    Fl_Valuator_set_step(&vin->valuator, 1.0);
    Fl_Valuator_set_value(&vin->valuator, 42.0);
    Fl_Widget_set_callback(&vin->valuator.widget, slider_cb, (void *)"value-input");

    Fl_Value_Output *vout = Fl_Value_Output_new(300, 250, 160, 25, NULL);
    Fl_Valuator_set_range(&vout->valuator, 0.0, 100.0);
    Fl_Valuator_set_step(&vout->valuator, 1.0);
    Fl_Valuator_set_value(&vout->valuator, 17.0);
    Fl_Widget_set_callback(&vout->valuator.widget, slider_cb, (void *)"value-output");

    Fl_Adjuster *adj = Fl_Adjuster_new(300, 285, 160, 25, NULL);
    Fl_Valuator_set_range(&adj->valuator, 0.0, 100.0);
    Fl_Valuator_set_value(&adj->valuator, 50.0);
    Fl_Widget_set_callback(&adj->valuator.widget, slider_cb, (void *)"adjuster");

    status = Fl_Output_new(20, 410, 440, 25, NULL);
    Fl_Input_set_value_str(status, "Drag/click a valuator");

    Fl_Group_end(&window->group);
    Fl_Widget_show(FL_WIDGET(window));

    return Fl_run();
}
