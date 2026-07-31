/*
 * cfltk - Fl_Light_Button.c
 * See include/cfltk/Fl_Light_Button.h for the class-conversion notes.
 * Translated from src/Fl_Light_Button.cxx.
 */
#include <stdlib.h>

#include "cfltk/Fl_Light_Button.h"
#include "cfltk/Fl.h"
#include "cfltk/fl_draw.h"

const Fl_WidgetOps fl_light_button_ops = {
    Fl_Light_Button_draw,
    Fl_Light_Button_handle,
    NULL, NULL, NULL,
    Fl_Widget_base_destroy,
    NULL, NULL
};

void Fl_Light_Button_init(Fl_Button *self, int x, int y, int w, int h, const char *label) {
    Fl_Button_init(self, x, y, w, h, label);
    self->widget.ops = &fl_light_button_ops;
    Fl_Widget_set_type(&self->widget, FL_TOGGLE_BUTTON);
    Fl_Widget_set_selection_color(&self->widget, FL_YELLOW);
    Fl_Widget_set_align(&self->widget, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
}

Fl_Button *Fl_Light_Button_new(int x, int y, int w, int h, const char *label) {
    Fl_Button *self = (Fl_Button *)malloc(sizeof(Fl_Button));
    Fl_Light_Button_init(self, x, y, w, h, label);
    return self;
}

Fl_Button *Fl_Radio_Light_Button_new(int x, int y, int w, int h, const char *label) {
    Fl_Button *self = Fl_Light_Button_new(x, y, w, h, label);
    Fl_Widget_set_type(&self->widget, FL_RADIO_BUTTON);
    return self;
}

void Fl_Light_Button_draw(Fl_Widget *self_w) {
    Fl_Button *self = (Fl_Button *)self_w;
    Fl_Color col;
    int W, bx, dx, dy, lx;

    if (Fl_Widget_box(self_w)) {
        fl_draw_box(self_w == Fl_pushed() ? fl_down(Fl_Widget_box(self_w)) : Fl_Widget_box(self_w),
                    self_w->x, self_w->y, self_w->w, self_w->h, Fl_Widget_color(self_w));
    }
    col = Fl_Button_value(self)
              ? (Fl_Widget_active_r(self_w) ? Fl_Widget_selection_color(self_w) : fl_inactive(Fl_Widget_selection_color(self_w)))
              : Fl_Widget_color(self_w);

    W = Fl_Widget_labelsize(self_w);
    bx = fl_box_dx(Fl_Widget_box(self_w));
    dx = bx + 2;
    dy = (self_w->h - W) / 2;
    lx = 0;

    if (self->down_box_) {
        switch (self->down_box_) {
            case FL_DOWN_BOX:
            case FL_UP_BOX: {
                fl_draw_box(self->down_box_, self_w->x + dx, self_w->y + dy, W, W, FL_BACKGROUND2_COLOR);
                if (Fl_Button_value(self)) {
                    int tx = self_w->x + dx + 3;
                    int tw = W - 6;
                    int d1 = tw / 3;
                    int d2 = tw - d1;
                    int ty = self_w->y + dy + (W + d2) / 2 - d1 - 2;
                    int n;
                    fl_color(col);
                    for (n = 0; n < 3; n++, ty++) {
                        fl_line(tx, ty, tx + d1, ty + d1);
                        fl_line(tx + d1, ty + d1, tx + tw - 1, ty + d1 - d2 + 1);
                    }
                }
                break;
            }
            case FL_ROUND_DOWN_BOX:
            case FL_ROUND_UP_BOX: {
                fl_draw_box(self->down_box_, self_w->x + dx, self_w->y + dy, W, W, FL_BACKGROUND2_COLOR);
                if (Fl_Button_value(self)) {
                    int tW = (W - fl_box_dw(self->down_box_)) / 2 + 1;
                    int tdx, tdy;
                    if ((W - tW) & 1) tW++;
                    tdx = dx + (W - tW) / 2;
                    tdy = dy + (W - tW) / 2;
                    fl_color(col);
                    fl_pie(self_w->x + tdx, self_w->y + tdy, tW, tW, 0.0, 360.0);
                }
                break;
            }
            default:
                fl_draw_box(self->down_box_, self_w->x + dx, self_w->y + dy, W, W, col);
                break;
        }
        lx = dx + W + 2;
    } else {
        int hh = self_w->h - 2 * dy - 2;
        int ww = W / 2 + 1;
        int xx = dx;
        if (self_w->w < ww + 2 * xx) xx = (self_w->w - ww) / 2;
        fl_draw_box(FL_THIN_DOWN_BOX, self_w->x + xx, self_w->y + dy + 1, ww, hh, col);
        lx = dx + ww + 2;
    }

    Fl_Widget_draw_label_in(self_w, self_w->x + lx, self_w->y, self_w->w - lx - bx, self_w->h);
    if (Fl_focus() == self_w) Fl_Widget_draw_focus(self_w, Fl_Widget_box(self_w), self_w->x, self_w->y, self_w->w, self_w->h);
}

int Fl_Light_Button_handle(Fl_Widget *self_w, int event) {
    if (event == FL_RELEASE && Fl_Widget_box(self_w)) Fl_Widget_redraw(self_w);
    return Fl_Button_handle(self_w, event);
}
