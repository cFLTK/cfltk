/*
 * cfltk - Fl_Scrollbar.c
 * See include/cfltk/Fl_Scrollbar.h for the class-conversion notes.
 * Translated from src/Fl_Scrollbar.cxx.
 */
#include <stdlib.h>

#include "cfltk/Fl_Scrollbar.h"
#include "cfltk/Fl.h"
#include "cfltk/fl_draw.h"

#define INITIAL_REPEAT 0.5
#define REPEAT 0.05

const Fl_WidgetOps fl_scrollbar_ops = {
    Fl_Scrollbar_draw,
    Fl_Scrollbar_handle,
    NULL, NULL, NULL,
    Fl_Scrollbar_destroy,
    NULL, NULL
};

void Fl_Scrollbar_init(Fl_Scrollbar *self, int x, int y, int w, int h, const char *label) {
    Fl_Slider_init(&self->slider, x, y, w, h, label);
    self->slider.valuator.widget.ops = &fl_scrollbar_ops;
    self->slider.valuator.widget.box = FL_FLAT_BOX;
    Fl_Widget_set_color(&self->slider.valuator.widget, FL_DARK2);
    Fl_Slider_set_slider(&self->slider, FL_UP_BOX);
    self->linesize_ = 16;
    self->pushed_ = 0;
    Fl_Valuator_set_step_ratio(&self->slider.valuator, 1, 1);
}

Fl_Scrollbar *Fl_Scrollbar_new(int x, int y, int w, int h, const char *label) {
    Fl_Scrollbar *self = (Fl_Scrollbar *)malloc(sizeof(Fl_Scrollbar));
    Fl_Scrollbar_init(self, x, y, w, h, label);
    return self;
}

static void timeout_cb(void *v);

void Fl_Scrollbar_destroy(Fl_Widget *self_w) {
    Fl_Scrollbar *self = (Fl_Scrollbar *)self_w;
    if (self->pushed_) Fl_remove_timeout(timeout_cb, self);
    Fl_Widget_base_destroy(self_w);
}

static void increment_cb(Fl_Scrollbar *self) {
    Fl_Valuator *v = &self->slider.valuator;
    char inv = Fl_Valuator_maximum(v) < Fl_Valuator_minimum(v);
    int ls = inv ? -self->linesize_ : self->linesize_;
    int i;

    switch (self->pushed_) {
        case 1: i = -ls; break;
        default: i = ls; break;
        case 5:
            i = -(int)((Fl_Valuator_maximum(v) - Fl_Valuator_minimum(v)) * Fl_Slider_slider_size(&self->slider) / (1.0 - Fl_Slider_slider_size(&self->slider)));
            if (inv) { if (i < -ls) i = -ls; } else { if (i > -ls) i = -ls; }
            break;
        case 6:
            i = (int)((Fl_Valuator_maximum(v) - Fl_Valuator_minimum(v)) * Fl_Slider_slider_size(&self->slider) / (1.0 - Fl_Slider_slider_size(&self->slider)));
            if (inv) { if (i > ls) i = ls; } else { if (i < ls) i = ls; }
            break;
    }
    Fl_Valuator_handle_drag(v, Fl_Valuator_clamp(v, Fl_Valuator_value(v) + i));
}

static void timeout_cb(void *v) {
    Fl_Scrollbar *self = (Fl_Scrollbar *)v;
    increment_cb(self);
    Fl_add_timeout(REPEAT, timeout_cb, self);
}

int Fl_Scrollbar_handle(Fl_Widget *self_w, int event) {
    Fl_Scrollbar *self = (Fl_Scrollbar *)self_w;
    Fl_Valuator *v = &self->slider.valuator;
    int area;
    int X = self_w->x, Y = self_w->y, W = self_w->w, H = self_w->h;
    int relx, ww;

    if (Fl_Valuator_horizontal(v)) {
        if (W >= 3 * H) { X += H; W -= 2 * H; }
    } else {
        if (H >= 3 * W) { Y += W; H -= 2 * W; }
    }

    if (Fl_Valuator_horizontal(v)) { relx = Fl_event_x() - X; ww = W; }
    else { relx = Fl_event_y() - Y; ww = H; }

    if (relx < 0) area = 1;
    else if (relx >= ww) area = 2;
    else {
        int T = (Fl_Valuator_horizontal(v) ? H : W) / 2 + 1;
        int S = (int)(Fl_Slider_slider_size(&self->slider) * ww + .5);
        double val = (Fl_Valuator_maximum(v) - Fl_Valuator_minimum(v)) != 0
                         ? (Fl_Valuator_value(v) - Fl_Valuator_minimum(v)) / (Fl_Valuator_maximum(v) - Fl_Valuator_minimum(v))
                         : 0.5;
        int sliderx;
        uchar type = Fl_Widget_type(self_w);
        if (type == FL_VERT_NICE_SLIDER || type == FL_HOR_NICE_SLIDER) T += 4;
        if (S < T) S = T;
        if (val >= 1.0) sliderx = ww - S;
        else if (val <= 0.0) sliderx = 0;
        else sliderx = (int)(val * (ww - S) + .5);

        if (Fl_event_button() == FL_MIDDLE_MOUSE) area = 8;
        else if (relx < sliderx) area = 5;
        else if (relx >= sliderx + S) area = 6;
        else area = 8;
    }

    switch (event) {
        case FL_ENTER:
        case FL_LEAVE:
            return 1;
        case FL_RELEASE:
            Fl_Widget_set_damage(self_w, FL_DAMAGE_ALL);
            if (self->pushed_) {
                Fl_remove_timeout(timeout_cb, self);
                self->pushed_ = 0;
            }
            Fl_Valuator_handle_release(v);
            return 1;
        case FL_PUSH:
            if (self->pushed_) return 1;
            if (area != 8) self->pushed_ = area;
            if (self->pushed_) {
                Fl_Valuator_handle_push(v);
                Fl_add_timeout(INITIAL_REPEAT, timeout_cb, self);
                increment_cb(self);
                Fl_Widget_set_damage(self_w, FL_DAMAGE_ALL);
                return 1;
            }
            return Fl_Slider_handle_in(&self->slider, event, X, Y, W, H);
        case FL_DRAG:
            if (self->pushed_) return 1;
            return Fl_Slider_handle_in(&self->slider, event, X, Y, W, H);
        case FL_MOUSEWHEEL:
            if (Fl_Valuator_horizontal(v)) {
                int ls;
                if (Fl_event_dx() == 0) return 0;
                ls = Fl_Valuator_maximum(v) >= Fl_Valuator_minimum(v) ? self->linesize_ : -self->linesize_;
                Fl_Valuator_handle_drag(v, Fl_Valuator_clamp(v, Fl_Valuator_value(v) + ls * Fl_event_dx()));
                return 1;
            } else {
                int ls;
                if (Fl_event_dy() == 0) return 0;
                ls = Fl_Valuator_maximum(v) >= Fl_Valuator_minimum(v) ? self->linesize_ : -self->linesize_;
                Fl_Valuator_handle_drag(v, Fl_Valuator_clamp(v, Fl_Valuator_value(v) + ls * Fl_event_dy()));
                return 1;
            }
        case FL_SHORTCUT:
        case FL_KEYBOARD: {
            int val = (int)Fl_Valuator_value(v);
            int ls = Fl_Valuator_maximum(v) >= Fl_Valuator_minimum(v) ? self->linesize_ : -self->linesize_;
            if (Fl_Valuator_horizontal(v)) {
                switch (Fl_event_key()) {
                    case FL_Left: val -= ls; break;
                    case FL_Right: val += ls; break;
                    default: return 0;
                }
            } else {
                switch (Fl_event_key()) {
                    case FL_Up: val -= ls; break;
                    case FL_Down: val += ls; break;
                    case FL_Page_Up:
                        if (Fl_Slider_slider_size(&self->slider) >= 1.0) return 0;
                        val -= (int)((Fl_Valuator_maximum(v) - Fl_Valuator_minimum(v)) * Fl_Slider_slider_size(&self->slider) / (1.0 - Fl_Slider_slider_size(&self->slider)));
                        val += ls;
                        break;
                    case FL_Page_Down:
                        if (Fl_Slider_slider_size(&self->slider) >= 1.0) return 0;
                        val += (int)((Fl_Valuator_maximum(v) - Fl_Valuator_minimum(v)) * Fl_Slider_slider_size(&self->slider) / (1.0 - Fl_Slider_slider_size(&self->slider)));
                        val -= ls;
                        break;
                    case FL_Home: val = (int)Fl_Valuator_minimum(v); break;
                    case FL_End: val = (int)Fl_Valuator_maximum(v); break;
                    default: return 0;
                }
            }
            val = (int)Fl_Valuator_clamp(v, val);
            if (val != (int)Fl_Valuator_value(v)) {
                Fl_Valuator_set_value(v, val);
                Fl_Widget_set_damage(self_w, FL_DAMAGE_EXPOSE);
                Fl_Widget_set_changed(self_w);
                Fl_Widget_do_callback(self_w);
            }
            return 1;
        }
        default:
            return 0;
    }
}

void Fl_Scrollbar_draw(Fl_Widget *self_w) {
    Fl_Scrollbar *self = (Fl_Scrollbar *)self_w;
    Fl_Valuator *v = &self->slider.valuator;
    int X, Y, W, H;

    if (self_w->damage & FL_DAMAGE_ALL) fl_draw_box(self_w->box, self_w->x, self_w->y, self_w->w, self_w->h, self_w->color);
    X = self_w->x + fl_box_dx(self_w->box);
    Y = self_w->y + fl_box_dy(self_w->box);
    W = self_w->w - fl_box_dw(self_w->box);
    H = self_w->h - fl_box_dh(self_w->box);

    if (Fl_Valuator_horizontal(v)) {
        if (W < 3 * H) { Fl_Slider_draw_in(&self->slider, X, Y, W, H); return; }
        Fl_Slider_draw_in(&self->slider, X + H, Y, W - 2 * H, H);
        if (self_w->damage & FL_DAMAGE_ALL) {
            int w1, x1, yy1;
            fl_draw_box(self->pushed_ == 1 ? fl_down(Fl_Slider_slider(&self->slider)) : Fl_Slider_slider(&self->slider), X, Y, H, H, self_w->color2);
            fl_draw_box(self->pushed_ == 2 ? fl_down(Fl_Slider_slider(&self->slider)) : Fl_Slider_slider(&self->slider), X + W - H, Y, H, H, self_w->color2);
            fl_color(Fl_Widget_active_r(self_w) ? Fl_Widget_labelcolor(self_w) : fl_inactive(Fl_Widget_labelcolor(self_w)));
            w1 = (H - 4) / 3; if (w1 < 1) w1 = 1;
            x1 = X + (H - w1 - 1) / 2;
            yy1 = Y + (H - 2 * w1 - 1) / 2;
            fl_polygon3(x1, yy1 + w1, x1 + w1, yy1 + 2 * w1, x1 + w1, yy1);
            x1 += (W - H);
            fl_polygon3(x1, yy1, x1, yy1 + 2 * w1, x1 + w1, yy1 + w1);
        }
    } else {
        if (H < 3 * W) { Fl_Slider_draw_in(&self->slider, X, Y, W, H); return; }
        Fl_Slider_draw_in(&self->slider, X, Y + W, W, H - 2 * W);
        if (self_w->damage & FL_DAMAGE_ALL) {
            int w1, x1, yy1;
            fl_draw_box(self->pushed_ == 1 ? fl_down(Fl_Slider_slider(&self->slider)) : Fl_Slider_slider(&self->slider), X, Y, W, W, self_w->color2);
            fl_draw_box(self->pushed_ == 2 ? fl_down(Fl_Slider_slider(&self->slider)) : Fl_Slider_slider(&self->slider), X, Y + H - W, W, W, self_w->color2);
            fl_color(Fl_Widget_active_r(self_w) ? Fl_Widget_labelcolor(self_w) : fl_inactive(Fl_Widget_labelcolor(self_w)));
            w1 = (W - 4) / 3; if (w1 < 1) w1 = 1;
            x1 = X + (W - 2 * w1 - 1) / 2;
            yy1 = Y + (W - w1 - 1) / 2;
            fl_polygon3(x1, yy1 + w1, x1 + 2 * w1, yy1 + w1, x1 + w1, yy1);
            yy1 += H - W;
            fl_polygon3(x1, yy1, x1 + w1, yy1 + w1, x1 + 2 * w1, yy1);
        }
    }
}
