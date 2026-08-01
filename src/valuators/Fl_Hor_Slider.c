#include <stdlib.h>
#include "cfltk/Fl_Hor_Slider.h"

Fl_Slider *Fl_Hor_Slider_new(int x, int y, int w, int h, const char *label) {
    return Fl_Slider_new_typed(FL_HOR_SLIDER, x, y, w, h, label);
}
