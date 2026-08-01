#include <stdlib.h>
#include "cfltk/Fl_Hor_Nice_Slider.h"

Fl_Slider *Fl_Hor_Nice_Slider_new(int x, int y, int w, int h, const char *label) {
    return Fl_Slider_new_typed(FL_HOR_NICE_SLIDER, x, y, w, h, label);
}
