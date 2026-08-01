/*
 * cfltk - Fl_Color_Chooser.c
 * See include/cfltk/Fl_Color_Chooser.h for the class-conversion notes.
 * Translated from src/Fl_Color_Chooser.cxx.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "cfltk/Fl_Color_Chooser.h"
#include "cfltk/Fl.h"
#include "cfltk/Fl_Window.h"
#include "cfltk/Fl_Return_Button.h"
#include "cfltk/fl_ask.h"
#include "cfltk/fl_draw.h"

#ifndef CFLTK_CC_PI
#define CFLTK_CC_PI 3.14159265358979323846
#endif

/* -------------------------------------------------------------------
 * HSV <-> RGB
 * ---------------------------------------------------------------- */

void Fl_Color_Chooser_hsv2rgb(double H, double S, double V, double *R, double *G, double *B) {
    if (S < 5.0e-6) {
        *R = *G = *B = V;
    } else {
        int i = (int)H;
        double f = H - (double)i;
        double p1 = V * (1.0 - S);
        double p2 = V * (1.0 - S * f);
        double p3 = V * (1.0 - S * (1.0 - f));
        switch (i) {
            case 0: *R = V; *G = p3; *B = p1; break;
            case 1: *R = p2; *G = V; *B = p1; break;
            case 2: *R = p1; *G = V; *B = p3; break;
            case 3: *R = p1; *G = p2; *B = V; break;
            case 4: *R = p3; *G = p1; *B = V; break;
            case 5: *R = V; *G = p1; *B = p2; break;
            default: break;
        }
    }
}

void Fl_Color_Chooser_rgb2hsv(double R, double G, double B, double *H, double *S, double *V) {
    double maxv = R > G ? R : G; if (B > maxv) maxv = B;
    *V = maxv;
    if (maxv > 0) {
        double minv = R < G ? R : G; if (B < minv) minv = B;
        *S = 1.0 - minv / maxv;
        if (maxv > minv) {
            if (maxv == R) { *H = (G - B) / (maxv - minv); if (*H < 0) *H += 6.0; }
            else if (maxv == G) *H = 2.0 + (B - R) / (maxv - minv);
            else *H = 4.0 + (R - G) / (maxv - minv);
        }
    }
}

/* -------------------------------------------------------------------
 * Mode menu / Flcc_Value_Input's hex-formatting hook
 * ---------------------------------------------------------------- */

static const Fl_Menu_Item mode_menu[] = {
    { "rgb" },
    { "byte" },
    { "hex" },
    { "hsv" },
    { 0 }
};

static void flcc_value_input_damage_cb(Fl_Valuator *v) {
    Fl_Value_Input *self = (Fl_Value_Input *)v;
    Fl_Color_Chooser *c = (Fl_Color_Chooser *)Fl_Widget_parent(&v->widget);
    char buf[128];
    if (c && Fl_Color_Chooser_mode(c) == FL_COLOR_CHOOSER_M_HEX) {
        snprintf(buf, sizeof(buf), "0x%02X", (int)Fl_Valuator_value(v));
    } else {
        Fl_Valuator_format(v, buf);
    }
    Fl_Input_set_value_str(&self->input, buf);
    Fl_Input_set_mark(&self->input, Fl_Input_position(&self->input));
}

static void set_valuators(Fl_Color_Chooser *c) {
    switch (Fl_Color_Chooser_mode(c)) {
        case FL_COLOR_CHOOSER_M_RGB:
            Fl_Valuator_set_range(&c->rvalue->valuator, 0, 1); Fl_Valuator_set_step_ratio(&c->rvalue->valuator, 1, 1000); Fl_Valuator_set_value(&c->rvalue->valuator, c->r_);
            Fl_Valuator_set_range(&c->gvalue->valuator, 0, 1); Fl_Valuator_set_step_ratio(&c->gvalue->valuator, 1, 1000); Fl_Valuator_set_value(&c->gvalue->valuator, c->g_);
            Fl_Valuator_set_range(&c->bvalue->valuator, 0, 1); Fl_Valuator_set_step_ratio(&c->bvalue->valuator, 1, 1000); Fl_Valuator_set_value(&c->bvalue->valuator, c->b_);
            break;
        case FL_COLOR_CHOOSER_M_BYTE:
        case FL_COLOR_CHOOSER_M_HEX:
            Fl_Valuator_set_range(&c->rvalue->valuator, 0, 255); Fl_Valuator_set_step(&c->rvalue->valuator, 1); Fl_Valuator_set_value(&c->rvalue->valuator, (int)(255 * c->r_ + .5));
            Fl_Valuator_set_range(&c->gvalue->valuator, 0, 255); Fl_Valuator_set_step(&c->gvalue->valuator, 1); Fl_Valuator_set_value(&c->gvalue->valuator, (int)(255 * c->g_ + .5));
            Fl_Valuator_set_range(&c->bvalue->valuator, 0, 255); Fl_Valuator_set_step(&c->bvalue->valuator, 1); Fl_Valuator_set_value(&c->bvalue->valuator, (int)(255 * c->b_ + .5));
            break;
        case FL_COLOR_CHOOSER_M_HSV:
            Fl_Valuator_set_range(&c->rvalue->valuator, 0, 6); Fl_Valuator_set_step_ratio(&c->rvalue->valuator, 1, 1000); Fl_Valuator_set_value(&c->rvalue->valuator, c->hue_);
            Fl_Valuator_set_range(&c->gvalue->valuator, 0, 1); Fl_Valuator_set_step_ratio(&c->gvalue->valuator, 1, 1000); Fl_Valuator_set_value(&c->gvalue->valuator, c->saturation_);
            Fl_Valuator_set_range(&c->bvalue->valuator, 0, 1); Fl_Valuator_set_step_ratio(&c->bvalue->valuator, 1, 1000); Fl_Valuator_set_value(&c->bvalue->valuator, c->value_);
            break;
        default: break;
    }
}

/* -------------------------------------------------------------------
 * Public rgb()/hsv() setters
 * ---------------------------------------------------------------- */

int Fl_Color_Chooser_rgb(Fl_Color_Chooser *c, double R, double G, double B) {
    double ph, ps, pv;
    if (R == c->r_ && G == c->g_ && B == c->b_) return 0;
    c->r_ = R; c->g_ = G; c->b_ = B;
    ph = c->hue_; ps = c->saturation_; pv = c->value_;
    Fl_Color_Chooser_rgb2hsv(R, G, B, &c->hue_, &c->saturation_, &c->value_);
    set_valuators(c);
    Fl_Widget_set_changed(&c->group.widget);
    if (c->value_ != pv) {
        Fl_Widget_set_damage(&c->huebox->widget, FL_DAMAGE_SCROLL);
        Fl_Widget_set_damage(&c->valuebox->widget, FL_DAMAGE_EXPOSE);
    }
    if (c->hue_ != ph || c->saturation_ != ps) {
        Fl_Widget_set_damage(&c->huebox->widget, FL_DAMAGE_EXPOSE);
        Fl_Widget_set_damage(&c->valuebox->widget, FL_DAMAGE_SCROLL);
    }
    return 1;
}

int Fl_Color_Chooser_hsv(Fl_Color_Chooser *c, double H, double S, double V) {
    double ph, ps, pv;
    H = fmod(H, 6.0); if (H < 0.0) H += 6.0;
    if (S < 0.0) S = 0.0; else if (S > 1.0) S = 1.0;
    if (V < 0.0) V = 0.0; else if (V > 1.0) V = 1.0;
    if (H == c->hue_ && S == c->saturation_ && V == c->value_) return 0;
    ph = c->hue_; ps = c->saturation_; pv = c->value_;
    c->hue_ = H; c->saturation_ = S; c->value_ = V;
    if (c->value_ != pv) {
        Fl_Widget_set_damage(&c->huebox->widget, FL_DAMAGE_SCROLL);
        Fl_Widget_set_damage(&c->valuebox->widget, FL_DAMAGE_EXPOSE);
    }
    if (c->hue_ != ph || c->saturation_ != ps) {
        Fl_Widget_set_damage(&c->huebox->widget, FL_DAMAGE_EXPOSE);
        Fl_Widget_set_damage(&c->valuebox->widget, FL_DAMAGE_SCROLL);
    }
    Fl_Color_Chooser_hsv2rgb(H, S, V, &c->r_, &c->g_, &c->b_);
    set_valuators(c);
    Fl_Widget_set_changed(&c->group.widget);
    return 1;
}

void Fl_Color_Chooser_set_mode(Fl_Color_Chooser *self, int new_mode) {
    Fl_Choice_set_value(self->choice, new_mode);
    Fl_Widget_do_callback(&self->choice->widget);
}

/* -------------------------------------------------------------------
 * Flcc_HueBox: the hue/saturation square (a filled circle, matching
 * upstream's #define CIRCLE 1 default).
 * ---------------------------------------------------------------- */

static void tohs(double x, double y, double *h, double *s) {
    x = 2 * x - 1;
    y = 1 - 2 * y;
    *s = sqrt(x * x + y * y); if (*s > 1.0) *s = 1.0;
    *h = (3.0 / CFLTK_CC_PI) * atan2(y, x);
    if (*h < 0) *h += 6.0;
}

static int flcc_huebox_handle_key(Flcc_HueBox *self, int key) {
    Fl_Widget *self_w = &self->widget;
    Fl_Color_Chooser *c = (Fl_Color_Chooser *)Fl_Widget_parent(self_w);
    int w1 = self_w->w - fl_box_dw(self_w->box) - 6;
    int h1 = self_w->h - fl_box_dh(self_w->box) - 6;
    int X = (int)(.5 * (cos(Fl_Color_Chooser_hue(c) * (CFLTK_CC_PI / 3.0)) * Fl_Color_Chooser_saturation(c) + 1) * w1);
    int Y = (int)(.5 * (1 - sin(Fl_Color_Chooser_hue(c) * (CFLTK_CC_PI / 3.0)) * Fl_Color_Chooser_saturation(c)) * h1);
    double Xf, Yf, H, S;

    switch (key) {
        case FL_Up: Y -= 3; break;
        case FL_Down: Y += 3; break;
        case FL_Left: X -= 3; break;
        case FL_Right: X += 3; break;
        default: return 0;
    }

    Xf = (double)X / (double)w1;
    Yf = (double)Y / (double)h1;
    tohs(Xf, Yf, &H, &S);
    if (Fl_Color_Chooser_hsv(c, H, S, Fl_Color_Chooser_value(c))) Fl_Widget_do_callback(&c->group.widget);
    return 1;
}

static int flcc_huebox_handle(Fl_Widget *self_w, int e) {
    static double ih, is;
    Flcc_HueBox *self = (Flcc_HueBox *)self_w;
    Fl_Color_Chooser *c = (Fl_Color_Chooser *)Fl_Widget_parent(self_w);

    switch (e) {
        case FL_PUSH:
            if (Fl_visible_focus()) { Fl_set_focus(self_w); Fl_Widget_redraw(self_w); }
            ih = Fl_Color_Chooser_hue(c);
            is = Fl_Color_Chooser_saturation(c);
            /* fall through */
        case FL_DRAG: {
            double Xf, Yf, H, S;
            Xf = (Fl_event_x() - self_w->x - fl_box_dx(self_w->box)) / (double)(self_w->w - fl_box_dw(self_w->box));
            Yf = (Fl_event_y() - self_w->y - fl_box_dy(self_w->box)) / (double)(self_w->h - fl_box_dh(self_w->box));
            tohs(Xf, Yf, &H, &S);
            if (fabs(H - ih) < 3 * 6.0 / self_w->w) H = ih;
            if (fabs(S - is) < 3 * 1.0 / self_w->h) S = is;
            if (Fl_event_state_of(FL_CTRL)) H = ih;
            if (Fl_Color_Chooser_hsv(c, H, S, Fl_Color_Chooser_value(c))) Fl_Widget_do_callback(&c->group.widget);
            return 1;
        }
        case FL_FOCUS:
        case FL_UNFOCUS:
            if (Fl_visible_focus()) { Fl_Widget_redraw(self_w); return 1; }
            return 1;
        case FL_KEYBOARD:
            return flcc_huebox_handle_key(self, Fl_event_key());
        default:
            return 0;
    }
}

static void flcc_huebox_draw(Fl_Widget *self_w) {
    Flcc_HueBox *self = (Flcc_HueBox *)self_w;
    Fl_Color_Chooser *c = (Fl_Color_Chooser *)Fl_Widget_parent(self_w);
    int x1 = self_w->x + fl_box_dx(self_w->box);
    int y1 = self_w->y + fl_box_dy(self_w->box);
    int w1 = self_w->w - fl_box_dw(self_w->box);
    int h1 = self_w->h - fl_box_dh(self_w->box);
    int X, Y;
    unsigned char *buf;
    int px, py;
    double V;

    if (Fl_Widget_damage(self_w) & FL_DAMAGE_ALL)
        fl_draw_box(self_w->box, self_w->x, self_w->y, self_w->w, self_w->h, self_w->color);

    V = Fl_Color_Chooser_value(c); /* UPDATE_HUE_BOX: darkens with value() */
    buf = (unsigned char *)malloc((size_t)w1 * (size_t)h1 * 3);
    for (py = 0; py < h1; py++) {
        double Yf = (double)py / h1;
        for (px = 0; px < w1; px++) {
            double Xf = (double)px / w1;
            double H, S, r = 0, g = 0, b = 0;
            unsigned char *p = buf + ((size_t)py * w1 + px) * 3;
            tohs(Xf, Yf, &H, &S);
            Fl_Color_Chooser_hsv2rgb(H, S, V, &r, &g, &b);
            p[0] = (unsigned char)(255 * r + .5);
            p[1] = (unsigned char)(255 * g + .5);
            p[2] = (unsigned char)(255 * b + .5);
        }
    }
    if (Fl_Widget_damage(self_w) == FL_DAMAGE_EXPOSE) fl_push_clip(x1 + self->px, y1 + self->py, 6, 6);
    fl_draw_image(buf, x1, y1, w1, h1, 3, 0);
    if (Fl_Widget_damage(self_w) == FL_DAMAGE_EXPOSE) fl_pop_clip();
    free(buf);

    X = (int)(.5 * (cos(Fl_Color_Chooser_hue(c) * (CFLTK_CC_PI / 3.0)) * Fl_Color_Chooser_saturation(c) + 1) * (w1 - 6));
    Y = (int)(.5 * (1 - sin(Fl_Color_Chooser_hue(c) * (CFLTK_CC_PI / 3.0)) * Fl_Color_Chooser_saturation(c)) * (h1 - 6));
    if (X < 0) X = 0; else if (X > w1 - 6) X = w1 - 6;
    if (Y < 0) Y = 0; else if (Y > h1 - 6) Y = h1 - 6;
    fl_draw_box(FL_UP_BOX, x1 + X, y1 + Y, 6, 6, Fl_focus() == self_w ? FL_FOREGROUND_COLOR : FL_GRAY);
    self->px = X; self->py = Y;
}

static Flcc_HueBox *flcc_huebox_new(int x, int y, int w, int h) {
    static const Fl_WidgetOps ops = { flcc_huebox_draw, flcc_huebox_handle, NULL, NULL, NULL, Fl_Widget_base_destroy, NULL, NULL };
    Flcc_HueBox *self = (Flcc_HueBox *)malloc(sizeof(Flcc_HueBox));
    Fl_Widget_init(&self->widget, &ops, x, y, w, h, NULL);
    self->px = self->py = 0;
    return self;
}

/* -------------------------------------------------------------------
 * Flcc_ValueBox: the vertical brightness slider.
 * ---------------------------------------------------------------- */

static int flcc_valuebox_handle_key(Flcc_ValueBox *self, int key) {
    Fl_Widget *self_w = &self->widget;
    Fl_Color_Chooser *c = (Fl_Color_Chooser *)Fl_Widget_parent(self_w);
    int h1 = self_w->h - fl_box_dh(self_w->box) - 6;
    int Y = (int)((1 - Fl_Color_Chooser_value(c)) * h1);
    double Yf;

    if (Y < 0) Y = 0; else if (Y > h1) Y = h1;
    switch (key) {
        case FL_Up: Y -= 3; break;
        case FL_Down: Y += 3; break;
        default: return 0;
    }
    Yf = 1 - ((double)Y / (double)h1);
    if (Fl_Color_Chooser_hsv(c, Fl_Color_Chooser_hue(c), Fl_Color_Chooser_saturation(c), Yf)) Fl_Widget_do_callback(&c->group.widget);
    return 1;
}

static int flcc_valuebox_handle(Fl_Widget *self_w, int e) {
    static double iv;
    Flcc_ValueBox *self = (Flcc_ValueBox *)self_w;
    Fl_Color_Chooser *c = (Fl_Color_Chooser *)Fl_Widget_parent(self_w);

    switch (e) {
        case FL_PUSH:
            if (Fl_visible_focus()) { Fl_set_focus(self_w); Fl_Widget_redraw(self_w); }
            iv = Fl_Color_Chooser_value(c);
            /* fall through */
        case FL_DRAG: {
            double Yf = 1 - (Fl_event_y() - self_w->y - fl_box_dy(self_w->box)) / (double)(self_w->h - fl_box_dh(self_w->box));
            if (fabs(Yf - iv) < (3 * 1.0 / self_w->h)) Yf = iv;
            if (Fl_Color_Chooser_hsv(c, Fl_Color_Chooser_hue(c), Fl_Color_Chooser_saturation(c), Yf)) Fl_Widget_do_callback(&c->group.widget);
            return 1;
        }
        case FL_FOCUS:
        case FL_UNFOCUS:
            if (Fl_visible_focus()) { Fl_Widget_redraw(self_w); return 1; }
            return 1;
        case FL_KEYBOARD:
            return flcc_valuebox_handle_key(self, Fl_event_key());
        default:
            return 0;
    }
}

static void flcc_valuebox_draw(Fl_Widget *self_w) {
    Flcc_ValueBox *self = (Flcc_ValueBox *)self_w;
    Fl_Color_Chooser *c = (Fl_Color_Chooser *)Fl_Widget_parent(self_w);
    int x1 = self_w->x + fl_box_dx(self_w->box);
    int y1 = self_w->y + fl_box_dy(self_w->box);
    int w1 = self_w->w - fl_box_dw(self_w->box);
    int h1 = self_w->h - fl_box_dh(self_w->box);
    double tr, tg, tb;
    unsigned char *buf;
    int px, py, Y;

    if (Fl_Widget_damage(self_w) & FL_DAMAGE_ALL)
        fl_draw_box(self_w->box, self_w->x, self_w->y, self_w->w, self_w->h, self_w->color);

    Fl_Color_Chooser_hsv2rgb(Fl_Color_Chooser_hue(c), Fl_Color_Chooser_saturation(c), 1.0, &tr, &tg, &tb);

    buf = (unsigned char *)malloc((size_t)w1 * (size_t)h1 * 3);
    for (py = 0; py < h1; py++) {
        double Yf = 255 * (1.0 - (double)py / h1);
        unsigned char r = (unsigned char)(tr * Yf + .5);
        unsigned char g = (unsigned char)(tg * Yf + .5);
        unsigned char b = (unsigned char)(tb * Yf + .5);
        for (px = 0; px < w1; px++) {
            unsigned char *p = buf + ((size_t)py * w1 + px) * 3;
            p[0] = r; p[1] = g; p[2] = b;
        }
    }
    if (Fl_Widget_damage(self_w) == FL_DAMAGE_EXPOSE) fl_push_clip(x1, y1 + self->py, w1, 6);
    fl_draw_image(buf, x1, y1, w1, h1, 3, 0);
    if (Fl_Widget_damage(self_w) == FL_DAMAGE_EXPOSE) fl_pop_clip();
    free(buf);

    Y = (int)((1 - Fl_Color_Chooser_value(c)) * (h1 - 6));
    if (Y < 0) Y = 0; else if (Y > h1 - 6) Y = h1 - 6;
    fl_draw_box(FL_UP_BOX, x1, y1 + Y, w1, 6, Fl_focus() == self_w ? FL_FOREGROUND_COLOR : FL_GRAY);
    self->py = Y;
}

static Flcc_ValueBox *flcc_valuebox_new(int x, int y, int w, int h) {
    static const Fl_WidgetOps ops = { flcc_valuebox_draw, flcc_valuebox_handle, NULL, NULL, NULL, Fl_Widget_base_destroy, NULL, NULL };
    Flcc_ValueBox *self = (Flcc_ValueBox *)malloc(sizeof(Flcc_ValueBox));
    Fl_Widget_init(&self->widget, &ops, x, y, w, h, NULL);
    self->py = 0;
    return self;
}

/* -------------------------------------------------------------------
 * rvalue/gvalue/bvalue and the mode Fl_Choice callbacks
 * ---------------------------------------------------------------- */

static void rgb_cb(Fl_Widget *o, void *data) {
    Fl_Color_Chooser *c = (Fl_Color_Chooser *)Fl_Widget_parent(o);
    double R, G, B;
    (void)data;
    R = Fl_Valuator_value(&c->rvalue->valuator);
    G = Fl_Valuator_value(&c->gvalue->valuator);
    B = Fl_Valuator_value(&c->bvalue->valuator);
    if (Fl_Color_Chooser_mode(c) == FL_COLOR_CHOOSER_M_HSV) {
        if (Fl_Color_Chooser_hsv(c, R, G, B)) Fl_Widget_do_callback(&c->group.widget);
        return;
    }
    if (Fl_Color_Chooser_mode(c) != FL_COLOR_CHOOSER_M_RGB) { R /= 255; G /= 255; B /= 255; }
    if (Fl_Color_Chooser_rgb(c, R, G, B)) Fl_Widget_do_callback(&c->group.widget);
}

static void mode_cb(Fl_Widget *o, void *data) {
    Fl_Color_Chooser *c = (Fl_Color_Chooser *)Fl_Widget_parent(o);
    (void)data;
    /* force a redraw even if the underlying value is unchanged */
    Fl_Valuator_set_value(&c->rvalue->valuator, -1);
    Fl_Valuator_set_value(&c->gvalue->valuator, -1);
    Fl_Valuator_set_value(&c->bvalue->valuator, -1);
    set_valuators(c);
}

/* -------------------------------------------------------------------
 * Fl_Color_Chooser
 * ---------------------------------------------------------------- */

const Fl_WidgetOps fl_color_chooser_ops = {
    Fl_Group_draw,
    Fl_Group_handle,
    Fl_Group_resize,
    NULL, NULL,
    Fl_Group_destroy,
    Fl_Group_as_group,
    NULL
};

void Fl_Color_Chooser_init(Fl_Color_Chooser *self, int x, int y, int w, int h, const char *label) {
    Fl_Widget *self_w = &self->group.widget;

    /* Built at its upstream-native 195x115 layout, then resized into
     * the caller's requested box via the group's own proportional
     * resize (resizable(resize_box)) -- matches upstream exactly. */
    Fl_Group_init(&self->group, 0, 0, 195, 115, label);
    self_w->ops = &fl_color_chooser_ops;

    self->huebox = flcc_huebox_new(0, 0, 115, 115);
    self->valuebox = flcc_valuebox_new(115, 0, 20, 115);
    self->choice = Fl_Choice_new(140, 0, 55, 25, NULL);
    self->rvalue = Fl_Value_Input_new(140, 30, 55, 25, NULL);
    self->gvalue = Fl_Value_Input_new(140, 60, 55, 25, NULL);
    self->bvalue = Fl_Value_Input_new(140, 90, 55, 25, NULL);
    self->resize_box = Fl_Box_new(0, 0, 115, 115, NULL);

    Fl_Group_end(&self->group);
    Fl_Group_set_resizable(&self->group, &self->resize_box->widget);
    Fl_Widget_resize(self_w, x, y, w, h);

    self->r_ = self->g_ = self->b_ = 0;
    self->hue_ = 0.0;
    self->saturation_ = 0.0;
    self->value_ = 0.0;

    Fl_Widget_set_box(&self->huebox->widget, FL_DOWN_FRAME);
    Fl_Widget_set_box(&self->valuebox->widget, FL_DOWN_FRAME);
    Fl_Menu_set_menu(self->choice, mode_menu);
    set_valuators(self);

    self->rvalue->valuator.value_damage = flcc_value_input_damage_cb;
    self->gvalue->valuator.value_damage = flcc_value_input_damage_cb;
    self->bvalue->valuator.value_damage = flcc_value_input_damage_cb;
    Fl_Widget_set_callback(&self->rvalue->valuator.widget, rgb_cb, NULL);
    Fl_Widget_set_callback(&self->gvalue->valuator.widget, rgb_cb, NULL);
    Fl_Widget_set_callback(&self->bvalue->valuator.widget, rgb_cb, NULL);
    Fl_Widget_set_callback(&self->choice->widget, mode_cb, NULL);
    Fl_Widget_set_box(&self->choice->widget, FL_THIN_UP_BOX);
    Fl_Menu_set_textfont(self->choice, FL_HELVETICA_BOLD_ITALIC);
}

Fl_Color_Chooser *Fl_Color_Chooser_new(int x, int y, int w, int h, const char *label) {
    Fl_Color_Chooser *self = (Fl_Color_Chooser *)malloc(sizeof(Fl_Color_Chooser));
    Fl_Color_Chooser_init(self, x, y, w, h, label);
    return self;
}

/* -------------------------------------------------------------------
 * fl_color_chooser(): the pop-up dialog
 * ---------------------------------------------------------------- */

typedef struct ColorChip {
    Fl_Widget widget;
    unsigned char r, g, b;
} ColorChip;

static void colorchip_draw(Fl_Widget *self_w) {
    ColorChip *self = (ColorChip *)self_w;
    if (Fl_Widget_damage(self_w) & FL_DAMAGE_ALL)
        fl_draw_box(self_w->box, self_w->x, self_w->y, self_w->w, self_w->h, self_w->color);
    fl_color_rgb(self->r, self->g, self->b);
    fl_rectf(self_w->x + fl_box_dx(self_w->box), self_w->y + fl_box_dy(self_w->box),
             self_w->w - fl_box_dw(self_w->box), self_w->h - fl_box_dh(self_w->box));
}

static ColorChip *colorchip_new(int x, int y, int w, int h) {
    static const Fl_WidgetOps ops = { colorchip_draw, NULL, NULL, NULL, NULL, Fl_Widget_base_destroy, NULL, NULL };
    ColorChip *self = (ColorChip *)malloc(sizeof(ColorChip));
    Fl_Widget_init(&self->widget, &ops, x, y, w, h, NULL);
    Fl_Widget_set_box(&self->widget, FL_ENGRAVED_FRAME);
    self->r = self->g = self->b = 0;
    return self;
}

static void chooser_cb(Fl_Widget *o, void *vv) {
    Fl_Color_Chooser *c = (Fl_Color_Chooser *)o;
    ColorChip *v = (ColorChip *)vv;
    v->r = (unsigned char)(255 * Fl_Color_Chooser_r(c) + .5);
    v->g = (unsigned char)(255 * Fl_Color_Chooser_g(c) + .5);
    v->b = (unsigned char)(255 * Fl_Color_Chooser_b(c) + .5);
    Fl_Widget_set_damage(&v->widget, FL_DAMAGE_EXPOSE);
}

static void cc_ok_cb(Fl_Widget *o, void *p) {
    *((int *)p) = 1;
    Fl_Widget_hide(FL_WIDGET(Fl_Widget_window(o)));
}

static void cc_cancel_cb(Fl_Widget *o, void *p) {
    *((int *)p) = 0;
    if (Fl_Widget_window(o)) Fl_Widget_hide(FL_WIDGET(Fl_Widget_window(o)));
    else Fl_Widget_hide(o);
}

int fl_color_chooser_d(const char *name, double *r, double *g, double *b, int cmode) {
    int ret = 0;
    Fl_Window *window;
    Fl_Color_Chooser *chooser;
    ColorChip *ok_color, *cancel_color;
    Fl_Button *ok_button, *cancel_button;

    window = Fl_Window_new(0, 0, 215, 200, name);
    Fl_Widget_set_callback(FL_WIDGET(window), cc_cancel_cb, &ret);

    chooser = Fl_Color_Chooser_new(10, 10, 195, 115, NULL);
    ok_color = colorchip_new(10, 130, 95, 25);
    ok_button = Fl_Return_Button_new(10, 165, 95, 25, fl_ok);
    Fl_Widget_set_callback(&ok_button->widget, cc_ok_cb, &ret);
    cancel_color = colorchip_new(110, 130, 95, 25);
    cancel_color->r = ok_color->r = (unsigned char)(255 * (*r) + .5);
    cancel_color->g = ok_color->g = (unsigned char)(255 * (*g) + .5);
    cancel_color->b = ok_color->b = (unsigned char)(255 * (*b) + .5);
    cancel_button = Fl_Button_new(110, 165, 95, 25, fl_cancel);
    Fl_Widget_set_callback(&cancel_button->widget, cc_cancel_cb, &ret);

    Fl_Group_set_resizable(&window->group, &chooser->group.widget);
    Fl_Color_Chooser_rgb(chooser, *r, *g, *b);
    Fl_Widget_set_callback(&chooser->group.widget, chooser_cb, ok_color);
    if (cmode != -1) Fl_Color_Chooser_set_mode(chooser, cmode);

    Fl_Group_end(&window->group);
    Fl_Window_hotspot(window, FL_WIDGET(window)->w / 2, FL_WIDGET(window)->h / 2, 0);
    Fl_Widget_show(FL_WIDGET(window));
    while (Fl_Window_shown(window)) Fl_wait();
    if (ret) {
        *r = Fl_Color_Chooser_r(chooser);
        *g = Fl_Color_Chooser_g(chooser);
        *b = Fl_Color_Chooser_b(chooser);
    }
    Fl_Widget_delete(FL_WIDGET(window));
    return ret;
}

int fl_color_chooser_u(const char *name, unsigned char *r, unsigned char *g, unsigned char *b, int cmode) {
    double dr = *r / 255.0, dg = *g / 255.0, db = *b / 255.0;
    if (fl_color_chooser_d(name, &dr, &dg, &db, cmode)) {
        *r = (unsigned char)(255 * dr + .5);
        *g = (unsigned char)(255 * dg + .5);
        *b = (unsigned char)(255 * db + .5);
        return 1;
    }
    return 0;
}
