/*
 * cfltk - Fl_Slider.c
 * See include/cfltk/Fl_Slider.h for the class-conversion notes.
 * Translated from src/Fl_Slider.cxx.
 */
#include <stdlib.h>

#include "cfltk/Fl_Slider.h"
#include "cfltk/Fl.h"
#include "cfltk/fl_draw.h"

const Fl_WidgetOps fl_slider_ops = {
    Fl_Slider_draw,
    Fl_Slider_handle,
    NULL, NULL, NULL,
    Fl_Widget_base_destroy,
    NULL, NULL
};

static void slider_common_init(Fl_Slider *self) {
    self->slider_size_ = 0;
    self->slider_ = 0;
}

void Fl_Slider_init(Fl_Slider *self, int x, int y, int w, int h, const char *label) {
    Fl_Valuator_init(&self->valuator, &fl_slider_ops, x, y, w, h, label);
    self->valuator.widget.box = FL_DOWN_BOX;
    slider_common_init(self);
}

void Fl_Slider_init_typed(Fl_Slider *self, uchar type, int x, int y, int w, int h, const char *label) {
    Fl_Valuator_init(&self->valuator, &fl_slider_ops, x, y, w, h, label);
    Fl_Widget_set_type(&self->valuator.widget, type);
    self->valuator.widget.box = (type == FL_HOR_NICE_SLIDER || type == FL_VERT_NICE_SLIDER) ? FL_FLAT_BOX : FL_DOWN_BOX;
    slider_common_init(self);
}

Fl_Slider *Fl_Slider_new(int x, int y, int w, int h, const char *label) {
    Fl_Slider *self = (Fl_Slider *)malloc(sizeof(Fl_Slider));
    Fl_Slider_init(self, x, y, w, h, label);
    return self;
}

Fl_Slider *Fl_Slider_new_typed(uchar type, int x, int y, int w, int h, const char *label) {
    Fl_Slider *self = (Fl_Slider *)malloc(sizeof(Fl_Slider));
    Fl_Slider_init_typed(self, type, x, y, w, h, label);
    return self;
}

void Fl_Slider_set_slider_size(Fl_Slider *self, double v) {
    if (v < 0) v = 0;
    if (v > 1) v = 1;
    if (self->slider_size_ != (float)v) {
        self->slider_size_ = (float)v;
        Fl_Widget_set_damage(&self->valuator.widget, FL_DAMAGE_EXPOSE);
    }
}

void Fl_Slider_set_bounds(Fl_Slider *self, double a, double b) {
    if (Fl_Valuator_minimum(&self->valuator) != a || Fl_Valuator_maximum(&self->valuator) != b) {
        Fl_Valuator_set_bounds(&self->valuator, a, b);
        Fl_Widget_set_damage(&self->valuator.widget, FL_DAMAGE_EXPOSE);
    }
}

int Fl_Slider_scrollvalue(Fl_Slider *self, int pos, int size, int first, int total) {
    Fl_Valuator_set_step_ratio(&self->valuator, 1, 1);
    if (pos + size > first + total) total = pos + size - first;
    Fl_Slider_set_slider_size(self, size >= total ? 1.0 : (double)size / (double)total);
    Fl_Slider_set_bounds(self, first, total - size + first);
    return Fl_Valuator_set_value(&self->valuator, pos);
}

static void draw_bg(Fl_Slider *self, int X, int Y, int W, int H) {
    Fl_Widget *w = &self->valuator.widget;
    Fl_Color black;

    fl_push_clip(X, Y, W, H);
    fl_draw_box(w->box, w->x, w->y, w->w, w->h, w->color);
    fl_pop_clip();

    black = Fl_Widget_active_r(w) ? FL_FOREGROUND_COLOR : FL_INACTIVE_COLOR;
    if (Fl_Widget_type(w) == FL_VERT_NICE_SLIDER) {
        fl_draw_box(FL_THIN_DOWN_BOX, X + W / 2 - 2, Y, 4, H, black);
    } else if (Fl_Widget_type(w) == FL_HOR_NICE_SLIDER) {
        fl_draw_box(FL_THIN_DOWN_BOX, X, Y + H / 2 - 2, W, 4, black);
    }
}

void Fl_Slider_draw_in(Fl_Slider *self, int X, int Y, int W, int H) {
    Fl_Widget *w = &self->valuator.widget;
    uchar type = Fl_Widget_type(w);
    double val;
    int ww, xx = 0, S;
    int xsl, ysl, wsl, hsl;
    uchar box1;

    if (Fl_Valuator_minimum(&self->valuator) == Fl_Valuator_maximum(&self->valuator)) {
        val = 0.5;
    } else {
        val = (Fl_Valuator_value(&self->valuator) - Fl_Valuator_minimum(&self->valuator)) /
              (Fl_Valuator_maximum(&self->valuator) - Fl_Valuator_minimum(&self->valuator));
        if (val > 1.0) val = 1.0;
        else if (val < 0.0) val = 0.0;
    }

    ww = Fl_Valuator_horizontal(&self->valuator) ? W : H;
    if (type == FL_HOR_FILL_SLIDER || type == FL_VERT_FILL_SLIDER) {
        S = (int)(val * ww + .5);
        if (Fl_Valuator_minimum(&self->valuator) > Fl_Valuator_maximum(&self->valuator)) { S = ww - S; xx = ww - S; }
        else xx = 0;
    } else {
        int T = (Fl_Valuator_horizontal(&self->valuator) ? H : W) / 2 + 1;
        S = (int)(self->slider_size_ * ww + .5);
        if (type == FL_VERT_NICE_SLIDER || type == FL_HOR_NICE_SLIDER) T += 4;
        if (S < T) S = T;
        xx = (int)(val * (ww - S) + .5);
    }

    if (Fl_Valuator_horizontal(&self->valuator)) {
        xsl = X + xx; wsl = S; ysl = Y; hsl = H;
    } else {
        ysl = Y + xx; hsl = S; xsl = X; wsl = W;
    }

    draw_bg(self, X, Y, W, H);

    box1 = self->slider_;
    if (!box1) { box1 = (uchar)(w->box & ~1); if (!box1) box1 = FL_UP_BOX; }

    if (type == FL_VERT_NICE_SLIDER) {
        fl_draw_box(box1, xsl, ysl, wsl, hsl, FL_GRAY);
        { int d = (hsl - 4) / 2; fl_draw_box(FL_THIN_DOWN_BOX, xsl + 2, ysl + d, wsl - 4, hsl - 2 * d, w->color2); }
    } else if (type == FL_HOR_NICE_SLIDER) {
        fl_draw_box(box1, xsl, ysl, wsl, hsl, FL_GRAY);
        { int d = (wsl - 4) / 2; fl_draw_box(FL_THIN_DOWN_BOX, xsl + d, ysl + 2, wsl - 2 * d, hsl - 4, w->color2); }
    } else if (wsl > 0 && hsl > 0) {
        fl_draw_box(box1, xsl, ysl, wsl, hsl, w->color2);
    }

    Fl_Widget_draw_label_at(w, xsl, ysl, wsl, hsl, Fl_Widget_align(w));
    if (Fl_focus() == w) {
        if (type == FL_HOR_FILL_SLIDER || type == FL_VERT_FILL_SLIDER) Fl_Widget_draw_focus(w, w->box, w->x, w->y, w->w, w->h);
        else Fl_Widget_draw_focus(w, box1, xsl, ysl, wsl, hsl);
    }
}

void Fl_Slider_draw(Fl_Widget *self_w) {
    Fl_Slider *self = (Fl_Slider *)self_w;
    if (self_w->damage & FL_DAMAGE_ALL) fl_draw_box(self_w->box, self_w->x, self_w->y, self_w->w, self_w->h, self_w->color);
    Fl_Slider_draw_in(self, self_w->x + fl_box_dx(self_w->box), self_w->y + fl_box_dy(self_w->box),
                       self_w->w - fl_box_dw(self_w->box), self_w->h - fl_box_dh(self_w->box));
}

int Fl_Slider_handle_in(Fl_Slider *self, int event, int X, int Y, int W, int H) {
    Fl_Widget *w = &self->valuator.widget;
    uchar type = Fl_Widget_type(w);
    static int offcenter = 0;

    switch (event) {
        case FL_PUSH:
            if (!Fl_event_inside_rect(X, Y, W, H)) return 0;
            Fl_Valuator_handle_push(&self->valuator);
            /* fallthrough */
        case FL_DRAG: {
            double val;
            int ww, mx, S, xx;
            double v;
            int try_again;

            if (Fl_Valuator_minimum(&self->valuator) == Fl_Valuator_maximum(&self->valuator)) val = 0.5;
            else {
                val = (Fl_Valuator_value(&self->valuator) - Fl_Valuator_minimum(&self->valuator)) /
                      (Fl_Valuator_maximum(&self->valuator) - Fl_Valuator_minimum(&self->valuator));
                if (val > 1.0) val = 1.0;
                else if (val < 0.0) val = 0.0;
            }

            ww = Fl_Valuator_horizontal(&self->valuator) ? W : H;
            mx = Fl_Valuator_horizontal(&self->valuator) ? Fl_event_x() - X : Fl_event_y() - Y;

            if (type == FL_HOR_FILL_SLIDER || type == FL_VERT_FILL_SLIDER) {
                S = 0;
                if (event == FL_PUSH) {
                    int xx0 = (int)(val * ww + .5);
                    offcenter = mx - xx0;
                    if (offcenter < -10 || offcenter > 10) offcenter = 0;
                    else return 1;
                }
            } else {
                int T = (Fl_Valuator_horizontal(&self->valuator) ? H : W) / 2 + 1;
                S = (int)(self->slider_size_ * ww + .5);
                if (S >= ww) return 0;
                if (type == FL_VERT_NICE_SLIDER || type == FL_HOR_NICE_SLIDER) T += 4;
                if (S < T) S = T;
                if (event == FL_PUSH) {
                    int xx0 = (int)(val * (ww - S) + .5);
                    offcenter = mx - xx0;
                    if (offcenter < 0) offcenter = 0;
                    else if (offcenter > S) offcenter = S;
                    else return 1;
                }
            }

            xx = mx - offcenter;
            v = 0;
            try_again = 1;
            while (try_again) {
                try_again = 0;
                if (xx < 0) {
                    xx = 0;
                    offcenter = mx; if (offcenter < 0) offcenter = 0;
                } else if (xx > (ww - S)) {
                    xx = ww - S;
                    offcenter = mx - xx; if (offcenter > S) offcenter = S;
                }
                v = Fl_Valuator_round(&self->valuator, xx * (Fl_Valuator_maximum(&self->valuator) - Fl_Valuator_minimum(&self->valuator)) / (ww - S) + Fl_Valuator_minimum(&self->valuator));
                if (event == FL_PUSH && v == Fl_Valuator_value(&self->valuator)) {
                    offcenter = S / 2;
                    event = FL_DRAG;
                    try_again = 1;
                }
            }
            Fl_Valuator_handle_drag(&self->valuator, Fl_Valuator_clamp(&self->valuator, v));
            return 1;
        }
        case FL_RELEASE:
            Fl_Valuator_handle_release(&self->valuator);
            return 1;
        case FL_KEYBOARD:
            switch (Fl_event_key()) {
                case FL_Up:
                    if (Fl_Valuator_horizontal(&self->valuator)) return 0;
                    Fl_Valuator_handle_push(&self->valuator);
                    Fl_Valuator_handle_drag(&self->valuator, Fl_Valuator_clamp(&self->valuator, Fl_Valuator_increment(&self->valuator, Fl_Valuator_value(&self->valuator), -1)));
                    Fl_Valuator_handle_release(&self->valuator);
                    return 1;
                case FL_Down:
                    if (Fl_Valuator_horizontal(&self->valuator)) return 0;
                    Fl_Valuator_handle_push(&self->valuator);
                    Fl_Valuator_handle_drag(&self->valuator, Fl_Valuator_clamp(&self->valuator, Fl_Valuator_increment(&self->valuator, Fl_Valuator_value(&self->valuator), 1)));
                    Fl_Valuator_handle_release(&self->valuator);
                    return 1;
                case FL_Left:
                    if (!Fl_Valuator_horizontal(&self->valuator)) return 0;
                    Fl_Valuator_handle_push(&self->valuator);
                    Fl_Valuator_handle_drag(&self->valuator, Fl_Valuator_clamp(&self->valuator, Fl_Valuator_increment(&self->valuator, Fl_Valuator_value(&self->valuator), -1)));
                    Fl_Valuator_handle_release(&self->valuator);
                    return 1;
                case FL_Right:
                    if (!Fl_Valuator_horizontal(&self->valuator)) return 0;
                    Fl_Valuator_handle_push(&self->valuator);
                    Fl_Valuator_handle_drag(&self->valuator, Fl_Valuator_clamp(&self->valuator, Fl_Valuator_increment(&self->valuator, Fl_Valuator_value(&self->valuator), 1)));
                    Fl_Valuator_handle_release(&self->valuator);
                    return 1;
                default:
                    return 0;
            }
        case FL_FOCUS:
        case FL_UNFOCUS:
            if (Fl_visible_focus()) { Fl_Widget_redraw(w); return 1; }
            return 0;
        case FL_ENTER:
        case FL_LEAVE:
            return 1;
        default:
            return 0;
    }
}

int Fl_Slider_handle(Fl_Widget *self_w, int event) {
    Fl_Slider *self = (Fl_Slider *)self_w;
    if (event == FL_PUSH && Fl_visible_focus()) {
        Fl_set_focus(self_w);
        Fl_Widget_redraw(self_w);
    }
    return Fl_Slider_handle_in(self, event,
                                self_w->x + fl_box_dx(self_w->box), self_w->y + fl_box_dy(self_w->box),
                                self_w->w - fl_box_dw(self_w->box), self_w->h - fl_box_dh(self_w->box));
}
