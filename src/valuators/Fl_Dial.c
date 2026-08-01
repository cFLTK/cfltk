/*
 * cfltk - Fl_Dial.c
 * See include/cfltk/Fl_Dial.h for the class-conversion notes.
 * Translated from src/Fl_Dial.cxx; the FL_NORMAL_DIAL/FL_LINE_DIAL
 * indicator uses a trig-computed dot/needle instead of upstream's
 * matrix-transformed polygons (see Fl_Dial.h).
 */
#include <math.h>
#include <stdlib.h>

#include "cfltk/Fl_Dial.h"
#include "cfltk/Fl.h"
#include "cfltk/fl_draw.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const Fl_WidgetOps fl_dial_ops = {
    Fl_Dial_draw,
    Fl_Dial_handle,
    NULL, NULL, NULL,
    Fl_Widget_base_destroy,
    NULL, NULL
};

void Fl_Dial_init(Fl_Dial *self, int x, int y, int w, int h, const char *label) {
    Fl_Valuator_init(&self->valuator, &fl_dial_ops, x, y, w, h, label);
    self->valuator.widget.box = FL_OVAL_BOX;
    Fl_Widget_set_selection_color(&self->valuator.widget, FL_INACTIVE_COLOR);
    self->a1 = 45;
    self->a2 = 315;
}

Fl_Dial *Fl_Dial_new(int x, int y, int w, int h, const char *label) {
    Fl_Dial *self = (Fl_Dial *)malloc(sizeof(Fl_Dial));
    Fl_Dial_init(self, x, y, w, h, label);
    return self;
}

void Fl_Dial_draw(Fl_Widget *self_w) {
    Fl_Dial *self = (Fl_Dial *)self_w;
    Fl_Valuator *v = &self->valuator;
    int X = self_w->x, Y = self_w->y, W = self_w->w, H = self_w->h;
    double angle;

    if (self_w->damage & FL_DAMAGE_ALL) fl_draw_box(self_w->box, X, Y, W, H, self_w->color);
    X += fl_box_dx(self_w->box);
    Y += fl_box_dy(self_w->box);
    W -= fl_box_dw(self_w->box);
    H -= fl_box_dh(self_w->box);

    angle = (self->a2 - self->a1) * (Fl_Valuator_value(v) - Fl_Valuator_minimum(v)) /
            (Fl_Valuator_maximum(v) - Fl_Valuator_minimum(v)) + self->a1;

    if (Fl_Widget_type(self_w) == FL_FILL_DIAL) {
        fl_color(Fl_Widget_active_r(self_w) ? self_w->color : fl_inactive(self_w->color));
        fl_pie(X, Y, W, H, 270 - self->a1, angle > self->a1 ? 360 + 270 - angle : 270 - 360 - angle);
        fl_color(Fl_Widget_active_r(self_w) ? self_w->color2 : fl_inactive(self_w->color2));
        fl_pie(X, Y, W, H, 270 - angle, 270 - self->a1);
    } else {
        if (!(self_w->damage & FL_DAMAGE_ALL)) {
            fl_color(Fl_Widget_active_r(self_w) ? self_w->color : fl_inactive(self_w->color));
            fl_pie(X + 1, Y + 1, W - 2, H - 2, 0, 360);
        }

        /* Trig-computed indicator: theta measured so dial angle 0
         * points screen-south and increases counterclockwise, matching
         * upstream's mouse->angle mapping in handle() below (derived
         * from its atan2(-my,mx) convention, not from the removed
         * matrix code). */
        {
            int cx = X + W / 2, cy = Y + H / 2;
            double theta = (270.0 - angle) * M_PI / 180.0;
            double dx = cos(theta), dy = -sin(theta);
            double r = (W < H ? W : H) / 2.0 - 2.0;
            int tipx = cx + (int)(dx * r);
            int tipy = cy + (int)(dy * r);

            fl_color(Fl_Widget_active_r(self_w) ? self_w->color2 : fl_inactive(self_w->color2));
            if (Fl_Widget_type(self_w) == FL_LINE_DIAL) {
                fl_line(cx, cy, tipx, tipy);
                fl_line(cx + 1, cy, tipx + 1, tipy);
            } else {
                int rr = r > 6 ? 5 : (int)r / 2 + 1;
                fl_pie(tipx - rr, tipy - rr, 2 * rr, 2 * rr, 0, 360);
            }
        }
    }

    Fl_Widget_draw_label(self_w);
}

int Fl_Dial_handle(Fl_Widget *self_w, int event) {
    Fl_Dial *self = (Fl_Dial *)self_w;
    Fl_Valuator *v = &self->valuator;
    int X = self_w->x, Y = self_w->y, W = self_w->w, H = self_w->h;

    switch (event) {
        case FL_PUSH:
            Fl_Valuator_handle_push(v);
            /* fallthrough */
        case FL_DRAG: {
            int mx = (Fl_event_x() - X - W / 2) * H;
            int my = (Fl_event_y() - Y - H / 2) * W;
            double angle, oldangle, val;
            if (!mx && !my) return 1;
            angle = 270 - atan2((double)-my, (double)mx) * 180 / M_PI;
            oldangle = (self->a2 - self->a1) * (Fl_Valuator_value(v) - Fl_Valuator_minimum(v)) /
                       (Fl_Valuator_maximum(v) - Fl_Valuator_minimum(v)) + self->a1;
            while (angle < oldangle - 180) angle += 360;
            while (angle > oldangle + 180) angle -= 360;
            if ((self->a1 < self->a2) ? (angle <= self->a1) : (angle >= self->a1)) val = Fl_Valuator_minimum(v);
            else if ((self->a1 < self->a2) ? (angle >= self->a2) : (angle <= self->a2)) val = Fl_Valuator_maximum(v);
            else val = Fl_Valuator_minimum(v) + (Fl_Valuator_maximum(v) - Fl_Valuator_minimum(v)) * (angle - self->a1) / (self->a2 - self->a1);
            Fl_Valuator_handle_drag(v, Fl_Valuator_clamp(v, Fl_Valuator_round(v, val)));
            return 1;
        }
        case FL_RELEASE:
            Fl_Valuator_handle_release(v);
            return 1;
        case FL_ENTER:
        case FL_LEAVE:
            return 1;
        default:
            return 0;
    }
}
