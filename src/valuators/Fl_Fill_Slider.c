#include <stdlib.h>
#include "cfltk/Fl_Fill_Slider.h"

Fl_Slider *Fl_Fill_Slider_new(int x, int y, int w, int h, const char *label) {
    return Fl_Slider_new_typed(FL_VERT_FILL_SLIDER, x, y, w, h, label);
}
