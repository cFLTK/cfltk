#include <stdlib.h>
#include "cfltk/Fl_Hor_Value_Slider.h"

Fl_Value_Slider *Fl_Hor_Value_Slider_new(int x, int y, int w, int h, const char *label) {
    Fl_Value_Slider *self = Fl_Value_Slider_new(x, y, w, h, label);
    Fl_Widget_set_type(&self->slider.valuator.widget, FL_HOR_SLIDER);
    return self;
}
