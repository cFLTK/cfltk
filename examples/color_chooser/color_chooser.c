/*
 * cfltk example: color_chooser
 * Exercises Fl_Color_Chooser embedded directly, plus the fl_color_chooser()
 * popup dialog triggered by a button.
 */
#include <stdio.h>
#include "cfltk/Fl.h"
#include "cfltk/Fl_Window.h"
#include "cfltk/Fl_Button.h"
#include "cfltk/Fl_Box.h"
#include "cfltk/Fl_Color_Chooser.h"

static Fl_Box *swatch;

static void chooser_cb(Fl_Widget *w, void *data) {
    (void)data;
    Fl_Color_Chooser *c = (Fl_Color_Chooser *)w;
    unsigned char r = (unsigned char)(255 * Fl_Color_Chooser_r(c) + .5);
    unsigned char g = (unsigned char)(255 * Fl_Color_Chooser_g(c) + .5);
    unsigned char b = (unsigned char)(255 * Fl_Color_Chooser_b(c) + .5);
    printf("chooser -> r=%d g=%d b=%d\n", r, g, b);
    fflush(stdout);
    Fl_Widget_set_color(&swatch->widget, fl_rgb_color(r, g, b));
    Fl_Widget_redraw(&swatch->widget);
}

static void popup_cb(Fl_Widget *w, void *data) {
    (void)w; (void)data;
    double r = 0.2, g = 0.6, b = 0.9;
    int ok = fl_color_chooser_d("Pick a color", &r, &g, &b, -1);
    printf("popup -> ok=%d r=%.2f g=%.2f b=%.2f\n", ok, r, g, b);
    fflush(stdout);
}

int main(void) {
    Fl_Window *window = Fl_Window_new(0, 0, 420, 220, "cfltk color_chooser");

    Fl_Color_Chooser *cc = Fl_Color_Chooser_new(10, 10, 200, 95, NULL);
    Fl_Widget_set_callback(&cc->group.widget, chooser_cb, NULL);
    Fl_Color_Chooser_rgb(cc, 1.0, 0.0, 0.0);

    swatch = Fl_Box_new(230, 10, 100, 95, NULL);
    Fl_Widget_set_box(&swatch->widget, FL_ENGRAVED_BOX);
    Fl_Widget_set_color(&swatch->widget, FL_RED);

    Fl_Button *popup_btn = Fl_Button_new(10, 130, 200, 30, "Open fl_color_chooser()");
    Fl_Widget_set_callback(&popup_btn->widget, popup_cb, NULL);

    Fl_Group_end(&window->group);
    Fl_Widget_show(FL_WIDGET(window));

    return Fl_run();
}
