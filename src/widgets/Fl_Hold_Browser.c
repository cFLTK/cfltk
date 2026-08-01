#include <stdlib.h>
#include "cfltk/Fl_Hold_Browser.h"

Fl_Browser *Fl_Hold_Browser_new(int x, int y, int w, int h, const char *label) {
    Fl_Browser *self = Fl_Browser_new(x, y, w, h, label);
    Fl_Widget_set_type(&self->browser_.group.widget, FL_HOLD_BROWSER);
    return self;
}
