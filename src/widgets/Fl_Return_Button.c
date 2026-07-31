/*
 * cfltk - Fl_Return_Button.c
 * See include/cfltk/Fl_Return_Button.h for the class-conversion notes.
 * Translated from src/Fl_Return_Button.cxx.
 */
#include <stdlib.h>

#include "cfltk/Fl_Return_Button.h"
#include "cfltk/Fl.h"
#include "cfltk/fl_draw.h"

const Fl_WidgetOps fl_return_button_ops = {
    Fl_Return_Button_draw,
    Fl_Return_Button_handle,
    NULL, NULL, NULL,
    Fl_Widget_base_destroy,
    NULL, NULL
};

void Fl_Return_Button_init(Fl_Button *self, int x, int y, int w, int h, const char *label) {
    Fl_Button_init(self, x, y, w, h, label);
    self->widget.ops = &fl_return_button_ops;
}

Fl_Button *Fl_Return_Button_new(int x, int y, int w, int h, const char *label) {
    Fl_Button *self = (Fl_Button *)malloc(sizeof(Fl_Button));
    Fl_Return_Button_init(self, x, y, w, h, label);
    return self;
}

/* The multi-segment fl_xyline()/fl_yxline() overloads upstream uses here
 * (each draws a connected vertical-horizontal-vertical, or
 * horizontal-vertical-horizontal, path in one call) are decomposed into
 * cfltk's single-segment primitives; same pixels, one call per segment. */
static void fl_return_arrow(int x, int y, int w, int h) {
    int size = w < h ? w : h;
    int d = (size + 2) / 4; if (d < 3) d = 3;
    int t = (size + 9) / 12; if (t < 1) t = 1;
    int x0 = x + (w - 2 * d - 2 * t - 1) / 2;
    int x1 = x0 + d;
    int y0 = y + h / 2;

    fl_color(FL_LIGHT3);
    fl_line(x0, y0, x1, y0 + d);
    fl_yxline(x1, y0 + d, y0 + t);
    fl_xyline(x1, y0 + t, x1 + d + 2 * t);
    fl_yxline(x1 + d + 2 * t, y0 + t, y0 - d);
    fl_yxline(x1, y0 - t, y0 - d);

    fl_color(fl_gray_ramp(0));
    fl_line(x0, y0, x1, y0 - d);

    fl_color(FL_DARK3);
    fl_xyline(x1 + 1, y0 - t, x1 + d);
    fl_yxline(x1 + d, y0 - t, y0 - d);
    fl_xyline(x1 + d, y0 - d, x1 + d + 2 * t);
}

void Fl_Return_Button_draw(Fl_Widget *self_w) {
    Fl_Button *self = (Fl_Button *)self_w;
    uchar bt;
    int dx, W;

    if (Fl_Widget_type(self_w) == FL_HIDDEN_BUTTON) return;

    bt = Fl_Button_value(self) ? (self->down_box_ ? self->down_box_ : fl_down(Fl_Widget_box(self_w))) : Fl_Widget_box(self_w);
    dx = fl_box_dx(bt);
    fl_draw_box(bt, self_w->x, self_w->y, self_w->w, self_w->h, Fl_Button_value(self) ? Fl_Widget_selection_color(self_w) : Fl_Widget_color(self_w));

    W = self_w->h;
    if (self_w->w / 3 < W) W = self_w->w / 3;
    fl_return_arrow(self_w->x + self_w->w - (W + dx), self_w->y, W, self_w->h);
    Fl_Widget_draw_label_in(self_w, self_w->x + dx, self_w->y, self_w->w - (dx + W + dx), self_w->h);
    if (Fl_focus() == self_w) Fl_Widget_draw_focus(self_w, bt, self_w->x, self_w->y, self_w->w, self_w->h);
}

int Fl_Return_Button_handle(Fl_Widget *self_w, int event) {
    Fl_Button *self = (Fl_Button *)self_w;
    if (event == FL_SHORTCUT && (Fl_event_key() == FL_Enter || Fl_event_key() == FL_KP_Enter)) {
        Fl_Button_simulate_key_action(self);
        Fl_Widget_do_callback(self_w);
        return 1;
    }
    return Fl_Button_handle(self_w, event);
}
