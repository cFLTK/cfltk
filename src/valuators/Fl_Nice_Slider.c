#include <stdlib.h>
#include "cfltk/Fl_Nice_Slider.h"

Fl_Slider *Fl_Nice_Slider_new(int x, int y, int w, int h, const char *label) {
    return Fl_Slider_new_typed(FL_VERT_NICE_SLIDER, x, y, w, h, label);
}
