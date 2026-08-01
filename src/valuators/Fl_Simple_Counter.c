#include <stdlib.h>
#include "cfltk/Fl_Simple_Counter.h"

Fl_Counter *Fl_Simple_Counter_new(int x, int y, int w, int h, const char *label) {
    Fl_Counter *self = Fl_Counter_new(x, y, w, h, label);
    Fl_Widget_set_type(&self->valuator.widget, FL_SIMPLE_COUNTER);
    return self;
}
