#include <stdlib.h>
#include "cfltk/Fl_Line_Dial.h"

Fl_Dial *Fl_Line_Dial_new(int x, int y, int w, int h, const char *label) {
    Fl_Dial *self = Fl_Dial_new(x, y, w, h, label);
    Fl_Widget_set_type(&self->valuator.widget, FL_LINE_DIAL);
    return self;
}
