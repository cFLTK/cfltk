/*
 * cfltk - Fl_Valuator.c
 * See include/cfltk/Fl_Valuator.h for the class-conversion notes.
 * Translated from src/Fl_Valuator.cxx.
 */
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "cfltk/Fl_Valuator.h"
#include "cfltk/Fl.h"

#define EPSILON 4.66e-10

void Fl_Valuator_init(Fl_Valuator *self, const Fl_WidgetOps *ops, int x, int y, int w, int h, const char *label) {
    Fl_Widget_init(&self->widget, ops, x, y, w, h, label);
    Fl_Widget_set_align(&self->widget, FL_ALIGN_BOTTOM);
    Fl_Widget_set_when(&self->widget, FL_WHEN_CHANGED);
    self->value_ = 0.0;
    self->previous_value_ = 1.0;
    self->min_ = 0.0;
    self->max_ = 1.0;
    self->A = 0.0;
    self->B = 1;
}

void Fl_Valuator_set_step(Fl_Valuator *self, double s) {
    double a;
    int b;
    if (s < 0) s = -s;
    a = rint(s);
    b = 1;
    while (fabs(s - a / b) > EPSILON && b <= (0x7fffffff / 10)) {
        b *= 10;
        a = rint(s * b);
    }
    self->A = a;
    self->B = b;
}

void Fl_Valuator_set_precision(Fl_Valuator *self, int digits) {
    if (digits > 9) digits = 9;
    else if (digits < 0) digits = 0;
    self->A = 1.0;
    self->B = 1;
    while (digits--) self->B *= 10;
}

int Fl_Valuator_set_value(Fl_Valuator *self, double v) {
    Fl_Widget_clear_changed(&self->widget);
    if (v == self->value_) return 0;
    self->value_ = v;
    Fl_Widget_set_damage(&self->widget, FL_DAMAGE_EXPOSE);
    return 1;
}

double Fl_Valuator_softclamp(const Fl_Valuator *self, double v) {
    int which = self->min_ <= self->max_;
    double p = self->previous_value_;
    if ((v < self->min_) == which && p != self->min_ && (p < self->min_) != which) return self->min_;
    if ((v > self->max_) == which && p != self->max_ && (p > self->max_) != which) return self->max_;
    return v;
}

void Fl_Valuator_handle_drag(Fl_Valuator *self, double v) {
    if (v != self->value_) {
        self->value_ = v;
        Fl_Widget_set_damage(&self->widget, FL_DAMAGE_EXPOSE);
        Fl_Widget_set_changed(&self->widget);
        if (Fl_Widget_when(&self->widget) & FL_WHEN_CHANGED) Fl_Widget_do_callback(&self->widget);
    }
}

void Fl_Valuator_handle_release(Fl_Valuator *self) {
    if (Fl_Widget_when(&self->widget) & FL_WHEN_RELEASE) {
        Fl_Widget_clear_changed(&self->widget);
        if (self->value_ != self->previous_value_ || (Fl_Widget_when(&self->widget) & FL_WHEN_NOT_CHANGED))
            Fl_Widget_do_callback(&self->widget);
    }
}

double Fl_Valuator_round(const Fl_Valuator *self, double v) {
    if (self->A) return rint(v * self->B / self->A) * self->A / self->B;
    return v;
}

double Fl_Valuator_clamp(const Fl_Valuator *self, double v) {
    int ascending = self->min_ <= self->max_;
    if ((v < self->min_) == ascending) return self->min_;
    if ((v > self->max_) == ascending) return self->max_;
    return v;
}

double Fl_Valuator_increment(const Fl_Valuator *self, double v, int n) {
    if (!self->A) return v + n * (self->max_ - self->min_) / 100;
    if (self->min_ > self->max_) n = -n;
    return (rint(v * self->B / self->A) + n) * self->A / self->B;
}

int Fl_Valuator_format(const Fl_Valuator *self, char *buffer) {
    double v = self->value_;
    int i, c;
    char temp[32];

    if (!self->A || !self->B) return snprintf(buffer, 128, "%g", v);

    snprintf(temp, sizeof(temp), "%.12f", self->A / self->B);
    for (i = (int)strlen(temp) - 1; i > 0; i--)
        if (temp[i] != '0') break;
    for (c = 0; i > 0; i--, c++)
        if (!isdigit((unsigned char)temp[i])) break;

    return snprintf(buffer, 128, "%.*f", c, v);
}
