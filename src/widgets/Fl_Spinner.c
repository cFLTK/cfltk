/*
 * cfltk - Fl_Spinner.c
 * See include/cfltk/Fl_Spinner.h for the class-conversion notes.
 * Translated from FL/Fl_Spinner.H + the Fl_Spinner::Fl_Spinner()
 * constructor in src/Fl_Group.cxx.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cfltk/Fl_Spinner.h"
#include "cfltk/Fl_Repeat_Button.h"
#include "cfltk/Fl.h"

static void spinner_update(Fl_Spinner *self) {
    char s[255];
    const char *format = self->format_;

    if (format[0] == '%' && format[1] == '.' && format[2] == '*') {
        /* Precision-argument form (set by Fl_Spinner_set_type(FL_FLOAT_INPUT),
         * format "%.*f"): derive the precision from how many significant
         * decimal digits step_ has, matching upstream's own simplified
         * (if a little ugly) approach. */
        int c = 0;
        char temp[64], *sp;
        snprintf(temp, sizeof(temp), "%.12f", self->step_);
        sp = temp + strlen(temp) - 1;
        while (sp > temp && *sp == '0') sp--;
        while (sp > temp && (*sp >= '0' && *sp <= '9')) { sp--; c++; }
        snprintf(s, sizeof(s), format, c, self->value_);
    } else {
        snprintf(s, sizeof(s), format, self->value_);
    }
    Fl_Input_set_value_str(self->input_, s);
}

static void sb_cb(Fl_Widget *w, void *data) {
    Fl_Spinner *sb = (Fl_Spinner *)data;
    double v;

    if (w == &sb->input_->widget) {
        v = atof(Fl_Input_value(sb->input_));
        if (v < sb->minimum_) { sb->value_ = sb->minimum_; spinner_update(sb); }
        else if (v > sb->maximum_) { sb->value_ = sb->maximum_; spinner_update(sb); }
        else sb->value_ = v;
    } else if (w == &sb->up_button_->widget) {
        v = sb->value_ + sb->step_;
        sb->value_ = (v > sb->maximum_) ? sb->minimum_ : v;
        spinner_update(sb);
    } else if (w == &sb->down_button_->widget) {
        v = sb->value_ - sb->step_;
        sb->value_ = (v < sb->minimum_) ? sb->maximum_ : v;
        spinner_update(sb);
    }

    Fl_Widget_set_changed(&sb->group.widget);
    Fl_Widget_do_callback(&sb->group.widget);
}

static int Fl_Spinner_handle(Fl_Widget *self_w, int event) {
    Fl_Spinner *self = (Fl_Spinner *)self_w;

    switch (event) {
        case FL_KEYDOWN:
        case FL_SHORTCUT:
            if (Fl_event_key() == FL_Up) { Fl_Widget_do_callback(&self->up_button_->widget); return 1; }
            else if (Fl_event_key() == FL_Down) { Fl_Widget_do_callback(&self->down_button_->widget); return 1; }
            return 0;

        case FL_FOCUS:
            return Fl_Widget_take_focus(&self->input_->widget) ? 1 : 0;
    }

    return Fl_Group_handle(self_w, event);
}

static void Fl_Spinner_resize(Fl_Widget *self_w, int X, int Y, int W, int H) {
    Fl_Spinner *self = (Fl_Spinner *)self_w;
    Fl_Group_resize(self_w, X, Y, W, H);
    Fl_Widget_resize(&self->input_->widget, X, Y, W - H / 2 - 2, H);
    Fl_Widget_resize(&self->up_button_->widget, X + W - H / 2 - 2, Y, H / 2 + 2, H / 2);
    Fl_Widget_resize(&self->down_button_->widget, X + W - H / 2 - 2, Y + H - H / 2, H / 2 + 2, H / 2);
}

const Fl_WidgetOps fl_spinner_ops = {
    Fl_Group_draw,
    Fl_Spinner_handle,
    Fl_Spinner_resize,
    Fl_Widget_default_show,
    Fl_Widget_default_hide,
    Fl_Group_destroy,
    Fl_Group_as_group,
    NULL
};

void Fl_Spinner_init(Fl_Spinner *self, int x, int y, int w, int h, const char *label) {
    Fl_Group_init(&self->group, x, y, w, h, label);
    self->group.widget.ops = &fl_spinner_ops;

    self->input_ = Fl_Input_new(x, y, w - h / 2 - 2, h, NULL);
    self->up_button_ = Fl_Repeat_Button_new(x + w - h / 2 - 2, y, h / 2 + 2, h / 2, "@-42<");
    self->down_button_ = Fl_Repeat_Button_new(x + w - h / 2 - 2, y + h - h / 2, h / 2 + 2, h / 2, "@-42>");

    Fl_Group_end(&self->group);

    self->value_ = 1.0;
    self->minimum_ = 1.0;
    self->maximum_ = 100.0;
    self->step_ = 1.0;
    self->format_ = "%g";

    Fl_Widget_set_align(&self->group.widget, FL_ALIGN_LEFT);

    Fl_Input_set_value_str(self->input_, "1");
    Fl_Input_set_input_type(self->input_, FL_INT_INPUT);
    Fl_Widget_set_when(&self->input_->widget, FL_WHEN_ENTER_KEY | FL_WHEN_RELEASE);
    Fl_Widget_set_callback(&self->input_->widget, sb_cb, self);

    Fl_Widget_set_callback(&self->up_button_->widget, sb_cb, self);
    Fl_Widget_set_callback(&self->down_button_->widget, sb_cb, self);
}

Fl_Spinner *Fl_Spinner_new(int x, int y, int w, int h, const char *label) {
    Fl_Spinner *self = (Fl_Spinner *)malloc(sizeof(Fl_Spinner));
    Fl_Spinner_init(self, x, y, w, h, label);
    return self;
}

void Fl_Spinner_set_format(Fl_Spinner *self, const char *f) {
    self->format_ = f;
    spinner_update(self);
}

void Fl_Spinner_set_step(Fl_Spinner *self, double s) {
    self->step_ = s;
    Fl_Input_set_input_type(self->input_, (s != (int)s) ? FL_FLOAT_INPUT : FL_INT_INPUT);
    spinner_update(self);
}

void Fl_Spinner_set_type(Fl_Spinner *self, unsigned char v) {
    Fl_Spinner_set_format(self, (v == FL_FLOAT_INPUT) ? "%.*f" : "%.0f");
    Fl_Input_set_input_type(self->input_, v);
}

void Fl_Spinner_set_value(Fl_Spinner *self, double v) {
    self->value_ = v;
    spinner_update(self);
}
