/*
 * cfltk example: xpm_xbm
 * Exercises Fl_XPM_Image/Fl_XBM_Image loading from disk.
 */
#include <stdio.h>
#include "cfltk/Fl.h"
#include "cfltk/Fl_Window.h"
#include "cfltk/Fl_Box.h"
#include "cfltk/Fl_XPM_Image.h"
#include "cfltk/Fl_XBM_Image.h"

int main(void) {
    Fl_Window *window = Fl_Window_new(0, 0, 220, 120, "cfltk xpm/xbm");

    Fl_Pixmap *xpm = Fl_XPM_Image_new("examples/xpm_xbm/sample.xpm");
    printf("XPM loaded: %dx%d\n", Fl_Pixmap_w(xpm), Fl_Pixmap_h(xpm));

    Fl_Bitmap *xbm = Fl_XBM_Image_new("examples/xpm_xbm/sample.xbm");
    printf("XBM loaded: %dx%d\n", Fl_Bitmap_w(xbm), Fl_Bitmap_h(xbm));
    fflush(stdout);

    Fl_Box *b1 = Fl_Box_new(10, 10, 100, 100, NULL);
    Fl_Widget_set_image(&b1->widget, (Fl_Image *)xpm);

    Fl_Box *b2 = Fl_Box_new(120, 10, 100, 100, NULL);
    Fl_Widget_set_labelcolor(&b2->widget, FL_BLACK);
    Fl_Widget_set_image(&b2->widget, (Fl_Image *)xbm);

    Fl_Group_end(&window->group);
    Fl_Widget_show(FL_WIDGET(window));

    return Fl_run();
}
