/*
 * cfltk - Fl_Adjuster.c
 * See include/cfltk/Fl_Adjuster.h for the class-conversion notes.
 * Translated from src/Fl_Adjuster.cxx (bitmap arrows simplified, see
 * header).
 */
#include <stdlib.h>

#include "cfltk/Fl_Adjuster.h"
#include "cfltk/Fl.h"
#include "cfltk/fl_draw.h"

const Fl_WidgetOps fl_adjuster_ops = {
    Fl_Adjuster_draw,
    Fl_Adjuster_handle,
    NULL, NULL, NULL,
    Fl_Widget_base_destroy,
    NULL, NULL
};

/* Appearance only depends on which button is pushed (drawn in _draw()
 * from `drag`, updated explicitly at push/release), not on value() --
 * matches upstream's value_damage() override, a deliberate no-op. */
static void adjuster_value_damage(Fl_Valuator *v) { (void)v; }

void Fl_Adjuster_init(Fl_Adjuster *self, int x, int y, int w, int h, const char *label) {
    Fl_Valuator_init(&self->valuator, &fl_adjuster_ops, x, y, w, h, label);
    self->valuator.value_damage = adjuster_value_damage;
    self->valuator.widget.box = FL_UP_BOX;
    Fl_Valuator_set_step_ratio(&self->valuator, 1, 10000);
    Fl_Widget_set_selection_color(&self->valuator.widget, FL_SELECTION_COLOR);
    self->drag = 0;
    self->ix = 0;
    self->soft_ = 1;
}

Fl_Adjuster *Fl_Adjuster_new(int x, int y, int w, int h, const char *label) {
    Fl_Adjuster *self = (Fl_Adjuster *)malloc(sizeof(Fl_Adjuster));
    Fl_Adjuster_init(self, x, y, w, h, label);
    return self;
}

/* count triangles pointing toward increasing value (right if the widget
 * is laid out horizontally, down if vertically) -- 1/2/3 for the slow/
 * medium/fast button, standing in for upstream's three distinct bitmap
 * glyphs (see header). */
static void draw_speed_arrows(int x, int y, int w, int h, int horizontal, int count, Fl_Color col) {
    int i;
    int step = (horizontal ? w : h) / (count + 1);
    fl_color(col);
    for (i = 0; i < count; i++) {
        int cx, cy, aw, ah;
        if (horizontal) {
            cx = x + step * (i + 1);
            cy = y + h / 2;
            aw = step > 10 ? 5 : step / 2;
            ah = h / 4 > 5 ? 5 : h / 4;
        } else {
            cx = x + w / 2;
            cy = y + step * (i + 1);
            aw = w / 4 > 5 ? 5 : w / 4;
            ah = step > 10 ? 5 : step / 2;
        }
        if (aw < 2) aw = 2;
        if (ah < 2) ah = 2;
        if (horizontal) fl_polygon3(cx - aw, cy - ah, cx - aw, cy + ah, cx + aw, cy);
        else fl_polygon3(cx - ah, cy - aw, cx + ah, cy - aw, cx, cy + aw);
    }
}

void Fl_Adjuster_draw(Fl_Widget *self_w) {
    Fl_Adjuster *self = (Fl_Adjuster *)self_w;
    int dx, dy, W, H;
    int horizontal = self_w->w >= self_w->h;
    Fl_Color sel;

    if (horizontal) {
        dx = W = self_w->w / 3;
        dy = 0; H = self_w->h;
    } else {
        dx = 0; W = self_w->w;
        dy = H = self_w->h / 3;
    }

    fl_draw_box(self->drag == 1 ? FL_DOWN_BOX : self_w->box, self_w->x, self_w->y + 2 * dy, W, H, self_w->color);
    fl_draw_box(self->drag == 2 ? FL_DOWN_BOX : self_w->box, self_w->x + dx, self_w->y + dy, W, H, self_w->color);
    fl_draw_box(self->drag == 3 ? FL_DOWN_BOX : self_w->box, self_w->x + 2 * dx, self_w->y, W, H, self_w->color);

    sel = Fl_Widget_active_r(self_w) ? Fl_Widget_selection_color(self_w) : fl_inactive(Fl_Widget_selection_color(self_w));
    draw_speed_arrows(self_w->x, self_w->y + 2 * dy, W, H, horizontal, 1, sel);
    draw_speed_arrows(self_w->x + dx, self_w->y + dy, W, H, horizontal, 2, sel);
    draw_speed_arrows(self_w->x + 2 * dx, self_w->y, W, H, horizontal, 3, sel);

    if (Fl_focus() == self_w) Fl_Widget_draw_focus(self_w, self_w->box, self_w->x, self_w->y, self_w->w, self_w->h);
}

int Fl_Adjuster_handle(Fl_Widget *self_w, int event) {
    Fl_Adjuster *self = (Fl_Adjuster *)self_w;
    Fl_Valuator *v = &self->valuator;
    double vv;
    int delta;
    int mx = Fl_event_x();

    switch (event) {
        case FL_PUSH:
            if (Fl_visible_focus()) Fl_set_focus(self_w);
            self->ix = mx;
            if (self_w->w >= self_w->h)
                self->drag = 3 * (mx - self_w->x) / self_w->w + 1;
            else
                self->drag = 3 - 3 * (Fl_event_y() - self_w->y - 1) / self_w->h;
            Fl_Valuator_handle_push(v);
            Fl_Widget_redraw(self_w);
            return 1;
        case FL_DRAG:
            if (self_w->w >= self_w->h) {
                delta = self_w->x + (self->drag - 1) * self_w->w / 3;
                if (mx < delta) delta = mx - delta;
                else if (mx > delta + self_w->w / 3) delta = mx - delta - self_w->w / 3;
                else delta = 0;
            } else {
                if (mx < self_w->x) delta = mx - self_w->x;
                else if (mx > self_w->x + self_w->w) delta = mx - self_w->x - self_w->w;
                else delta = 0;
            }
            switch (self->drag) {
                case 3: vv = Fl_Valuator_increment(v, Fl_Valuator_previous_value(v), delta); break;
                case 2: vv = Fl_Valuator_increment(v, Fl_Valuator_previous_value(v), delta * 10); break;
                default: vv = Fl_Valuator_increment(v, Fl_Valuator_previous_value(v), delta * 100); break;
            }
            Fl_Valuator_handle_drag(v, self->soft_ ? Fl_Valuator_softclamp(v, vv) : Fl_Valuator_clamp(v, vv));
            return 1;
        case FL_RELEASE:
            if (Fl_event_is_click()) {
                if (Fl_event_state() & (FL_SHIFT | FL_CAPS_LOCK | FL_CTRL | FL_ALT)) delta = -10;
                else delta = 10;
                switch (self->drag) {
                    case 3: vv = Fl_Valuator_increment(v, Fl_Valuator_previous_value(v), delta); break;
                    case 2: vv = Fl_Valuator_increment(v, Fl_Valuator_previous_value(v), delta * 10); break;
                    default: vv = Fl_Valuator_increment(v, Fl_Valuator_previous_value(v), delta * 100); break;
                }
                Fl_Valuator_handle_drag(v, self->soft_ ? Fl_Valuator_softclamp(v, vv) : Fl_Valuator_clamp(v, vv));
            }
            self->drag = 0;
            Fl_Widget_redraw(self_w);
            Fl_Valuator_handle_release(v);
            return 1;
        case FL_KEYBOARD:
            switch (Fl_event_key()) {
                case FL_Up:
                    if (self_w->w > self_w->h) return 0;
                    Fl_Valuator_handle_drag(v, Fl_Valuator_clamp(v, Fl_Valuator_increment(v, Fl_Valuator_value(v), -1)));
                    return 1;
                case FL_Down:
                    if (self_w->w > self_w->h) return 0;
                    Fl_Valuator_handle_drag(v, Fl_Valuator_clamp(v, Fl_Valuator_increment(v, Fl_Valuator_value(v), 1)));
                    return 1;
                case FL_Left:
                    if (self_w->w < self_w->h) return 0;
                    Fl_Valuator_handle_drag(v, Fl_Valuator_clamp(v, Fl_Valuator_increment(v, Fl_Valuator_value(v), -1)));
                    return 1;
                case FL_Right:
                    if (self_w->w < self_w->h) return 0;
                    Fl_Valuator_handle_drag(v, Fl_Valuator_clamp(v, Fl_Valuator_increment(v, Fl_Valuator_value(v), 1)));
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
