/*
 * cfltk - Fl_Roller.c
 * See include/cfltk/Fl_Roller.h for the class-conversion notes.
 * Translated from src/Fl_Roller.cxx.
 */
#include <math.h>
#include <stdlib.h>

#include "cfltk/Fl_Roller.h"
#include "cfltk/Fl.h"
#include "cfltk/fl_draw.h"

const Fl_WidgetOps fl_roller_ops = {
    Fl_Roller_draw,
    Fl_Roller_handle,
    NULL, NULL, NULL,
    Fl_Widget_base_destroy,
    NULL, NULL
};

void Fl_Roller_init(Fl_Roller *self, int x, int y, int w, int h, const char *label) {
    Fl_Valuator_init(self, &fl_roller_ops, x, y, w, h, label);
    self->widget.box = FL_UP_BOX;
    Fl_Valuator_set_step_ratio(self, 1, 1000);
}

Fl_Roller *Fl_Roller_new(int x, int y, int w, int h, const char *label) {
    Fl_Roller *self = (Fl_Roller *)malloc(sizeof(Fl_Roller));
    Fl_Roller_init(self, x, y, w, h, label);
    return self;
}

int Fl_Roller_handle(Fl_Widget *self_w, int event) {
    Fl_Roller *self = (Fl_Roller *)self_w;
    static int ipos = 0;
    int newpos = Fl_Valuator_horizontal(self) ? Fl_event_x() : Fl_event_y();

    switch (event) {
        case FL_PUSH:
            if (Fl_visible_focus()) { Fl_set_focus(self_w); Fl_Widget_redraw(self_w); }
            Fl_Valuator_handle_push(self);
            ipos = newpos;
            return 1;
        case FL_DRAG:
            Fl_Valuator_handle_drag(self, Fl_Valuator_clamp(self, Fl_Valuator_round(self, Fl_Valuator_increment(self, Fl_Valuator_previous_value(self), newpos - ipos))));
            return 1;
        case FL_RELEASE:
            Fl_Valuator_handle_release(self);
            return 1;
        case FL_KEYBOARD:
            switch (Fl_event_key()) {
                case FL_Up:
                    if (Fl_Valuator_horizontal(self)) return 0;
                    Fl_Valuator_handle_drag(self, Fl_Valuator_clamp(self, Fl_Valuator_increment(self, Fl_Valuator_value(self), -1)));
                    return 1;
                case FL_Down:
                    if (Fl_Valuator_horizontal(self)) return 0;
                    Fl_Valuator_handle_drag(self, Fl_Valuator_clamp(self, Fl_Valuator_increment(self, Fl_Valuator_value(self), 1)));
                    return 1;
                case FL_Left:
                    if (!Fl_Valuator_horizontal(self)) return 0;
                    Fl_Valuator_handle_drag(self, Fl_Valuator_clamp(self, Fl_Valuator_increment(self, Fl_Valuator_value(self), -1)));
                    return 1;
                case FL_Right:
                    if (!Fl_Valuator_horizontal(self)) return 0;
                    Fl_Valuator_handle_drag(self, Fl_Valuator_clamp(self, Fl_Valuator_increment(self, Fl_Valuator_value(self), 1)));
                    return 1;
                default:
                    return 0;
            }
        case FL_FOCUS:
        case FL_UNFOCUS:
            if (Fl_visible_focus()) { Fl_Widget_redraw(self_w); return 1; }
            return 0;
        case FL_ENTER:
        case FL_LEAVE:
            return 1;
        default:
            return 0;
    }
}

void Fl_Roller_draw(Fl_Widget *self_w) {
    Fl_Roller *self = (Fl_Roller *)self_w;
    int X, Y, W, H;
    int offset;
    const double ARC = 1.5;
    const double delta = .2;

    if (self_w->damage & FL_DAMAGE_ALL) fl_draw_box(self_w->box, self_w->x, self_w->y, self_w->w, self_w->h, self_w->color);
    X = self_w->x + fl_box_dx(self_w->box);
    Y = self_w->y + fl_box_dy(self_w->box);
    W = self_w->w - fl_box_dw(self_w->box) - 1;
    H = self_w->h - fl_box_dh(self_w->box) - 1;
    if (W <= 0 || H <= 0) return;

    offset = Fl_Valuator_step(self) ? (int)(Fl_Valuator_value(self) / Fl_Valuator_step(self)) : 0;

    if (Fl_Valuator_horizontal(self)) {
        int h1 = W / 4 + 1;
        int i;
        fl_color(self_w->color);
        fl_rectf(X + h1, Y, W - 2 * h1, H);
        for (i = 0; h1; i++) {
            int h2 = (FL_GRAY - i - 1 > FL_DARK3) ? 2 * h1 / 3 + 1 : 0;
            fl_color((Fl_Color)(FL_GRAY - i - 1));
            fl_rectf(X + h2, Y, h1 - h2, H);
            fl_rectf(X + W - h1, Y, h1 - h2, H);
            h1 = h2;
        }
        if (Fl_Widget_active_r(self_w)) {
            double junk;
            double yy;
            for (yy = -ARC + modf(offset * sin(ARC) / (W / 2) / delta, &junk) * delta;; yy += delta) {
                int yy1 = (int)((sin(yy) / sin(ARC) + 1) * W / 2);
                if (yy1 <= 0) continue;
                if (yy1 >= W - 1) break;
                fl_color(FL_DARK3); fl_yxline(X + yy1, Y + 1, Y + H - 1);
                if (yy < 0) yy1--; else yy1++;
                fl_color(FL_LIGHT1); fl_yxline(X + yy1, Y + 1, Y + H - 1);
            }
            h1 = W / 8 + 1;
            fl_color(FL_DARK2);
            fl_xyline(X + h1, Y + H - 1, X + W - h1);
            fl_color(FL_DARK3);
            fl_yxline(X, Y + H, Y);
            fl_xyline(X, Y, X + h1);
            fl_xyline(X + W - h1, Y, X + W);
            fl_color(FL_LIGHT2);
            fl_xyline(X + h1, Y - 1, X + W - h1);
            fl_yxline(X + W, Y, Y + H);
            fl_xyline(X + W, Y + H, X + W - h1);
            fl_xyline(X + h1, Y + H, X);
        }
    } else {
        int h1 = H / 4 + 1;
        int i;
        fl_color(self_w->color);
        fl_rectf(X, Y + h1, W, H - 2 * h1);
        for (i = 0; h1; i++) {
            int h2 = (FL_GRAY - i - 1 > FL_DARK3) ? 2 * h1 / 3 + 1 : 0;
            fl_color((Fl_Color)(FL_GRAY - i - 1));
            fl_rectf(X, Y + h2, W, h1 - h2);
            fl_rectf(X, Y + H - h1, W, h1 - h2);
            h1 = h2;
        }
        if (Fl_Widget_active_r(self_w)) {
            double junk;
            double yy;
            for (yy = -ARC + modf(offset * sin(ARC) / (H / 2) / delta, &junk) * delta;; yy += delta) {
                int yy1 = (int)((sin(yy) / sin(ARC) + 1) * H / 2);
                if (yy1 <= 0) continue;
                if (yy1 >= H - 1) break;
                fl_color(FL_DARK3); fl_xyline(X + 1, Y + yy1, X + W - 1);
                if (yy < 0) yy1--; else yy1++;
                fl_color(FL_LIGHT1); fl_xyline(X + 1, Y + yy1, X + W - 1);
            }
            h1 = H / 8 + 1;
            fl_color(FL_DARK2);
            fl_yxline(X + W - 1, Y + h1, Y + H - h1);
            fl_color(FL_DARK3);
            fl_xyline(X + W, Y, X);
            fl_yxline(X, Y, Y + h1);
            fl_yxline(X, Y + H - h1, Y + H);
            fl_color(FL_LIGHT2);
            fl_yxline(X, Y + h1, Y + H - h1);
            fl_xyline(X, Y + H, X + W);
            fl_yxline(X + W, Y + H, Y + H - h1);
            fl_yxline(X + W, Y + h1, Y);
        }
    }

    if (Fl_focus() == self_w) Fl_Widget_draw_focus(self_w, FL_THIN_UP_FRAME, self_w->x, self_w->y, self_w->w, self_w->h);
}
