/*
 * cfltk example: double_window
 *
 * Exercises Fl_Double_Window: a box bounces left-right across the
 * window on a timer, forcing a full-window redraw (background fill +
 * box) every ~30ms. On a single-buffered window this kind of
 * whole-area animation is exactly what shows visible tearing/flicker;
 * on Fl_Double_Window it should look clean since every frame is fully
 * composed off-screen before being blitted in one XCopyArea(). Also
 * exercises live resizing, which recreates the offscreen buffer at
 * the new size (see fl_x11_window.c's resize_offscreen()).
 */
#include "cfltk/Fl.h"
#include "cfltk/Fl_Double_Window.h"
#include "cfltk/Fl_Box.h"

static Fl_Double_Window *dwin;
static Fl_Box *box;
static int x = 20, dir = 1;

static void tick(void *data) {
    int ww = dwin->window.group.widget.w;
    (void)data;
    x += dir * 6;
    if (x > ww - 80) { x = ww - 80; dir = -1; }
    if (x < 0) { x = 0; dir = 1; }
    Fl_Widget_resize(&box->widget, x, 80, 60, 60);
    Fl_Widget_redraw(FL_WIDGET(dwin));
    Fl_repeat_timeout(0.03, tick, NULL);
}

int main(void) {
    dwin = Fl_Double_Window_new(0, 0, 400, 200, "cfltk double_window");

    box = Fl_Box_new(x, 80, 60, 60, NULL);
    Fl_Widget_set_box(&box->widget, FL_FLAT_BOX);
    Fl_Widget_set_color(&box->widget, FL_RED);

    Fl_Group_end(&dwin->window.group);
    Fl_Widget_show(FL_WIDGET(dwin));

    Fl_add_timeout(0.03, tick, NULL);

    return Fl_run();
}
