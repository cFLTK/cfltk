/*
 * cfltk - Fl_Value_Output.c
 * See include/cfltk/Fl_Value_Output.h for the class-conversion notes.
 * Translated from src/Fl_Value_Output.cxx.
 */
#include <stdlib.h>

#include "cfltk/Fl_Value_Output.h"
#include "cfltk/Fl.h"
#include "cfltk/fl_draw.h"

const Fl_WidgetOps fl_value_output_ops = {
    Fl_Value_Output_draw,
    Fl_Value_Output_handle,
    NULL, NULL, NULL,
    Fl_Widget_base_destroy,
    NULL, NULL
};

void Fl_Value_Output_init(Fl_Value_Output *self, int x, int y, int w, int h, const char *label) {
    Fl_Valuator_init(&self->valuator, &fl_value_output_ops, x, y, w, h, label);
    self->valuator.widget.box = FL_NO_BOX;
    Fl_Widget_set_align(&self->valuator.widget, FL_ALIGN_LEFT);
    self->textfont_ = FL_HELVETICA;
    self->textsize_ = FL_NORMAL_SIZE;
    self->textcolor_ = FL_FOREGROUND_COLOR;
    self->soft_ = 0;
}

Fl_Value_Output *Fl_Value_Output_new(int x, int y, int w, int h, const char *label) {
    Fl_Value_Output *self = (Fl_Value_Output *)malloc(sizeof(Fl_Value_Output));
    Fl_Value_Output_init(self, x, y, w, h, label);
    return self;
}

void Fl_Value_Output_draw(Fl_Widget *self_w) {
    Fl_Value_Output *self = (Fl_Value_Output *)self_w;
    uchar b = self_w->box ? self_w->box : FL_DOWN_BOX;
    int X = self_w->x + fl_box_dx(b);
    int Y = self_w->y + fl_box_dy(b);
    int W = self_w->w - fl_box_dw(b);
    int H = self_w->h - fl_box_dh(b);
    char buf[128];
    Fl_Label l;

    if (self_w->damage & (uchar)~FL_DAMAGE_CHILD) {
        fl_draw_box(b, self_w->x, self_w->y, self_w->w, self_w->h, self_w->color);
    } else {
        fl_color(self_w->color);
        fl_rectf(X, Y, W, H);
    }

    Fl_Valuator_format(&self->valuator, buf);
    l.value = buf; l.image = NULL; l.deimage = NULL; l.type = FL_NORMAL_LABEL;
    l.font = self->textfont_; l.size = self->textsize_;
    l.color = Fl_Widget_active_r(self_w) ? self->textcolor_ : fl_inactive(self->textcolor_);
    l.align = FL_ALIGN_LEFT;
    fl_label_draw(&l, X, Y, W, H, FL_ALIGN_LEFT);
}

int Fl_Value_Output_handle(Fl_Widget *self_w, int event) {
    Fl_Value_Output *self = (Fl_Value_Output *)self_w;
    Fl_Valuator *v = &self->valuator;
    double vv;
    int delta;
    int mx = Fl_event_x();
    static int ix, drag;

    if (!Fl_Valuator_step(v)) return 0;

    switch (event) {
        case FL_PUSH:
            ix = mx;
            drag = Fl_event_button();
            Fl_Valuator_handle_push(v);
            return 1;
        case FL_DRAG:
            delta = Fl_event_x() - ix;
            if (delta > 5) delta -= 5;
            else if (delta < -5) delta += 5;
            else delta = 0;
            switch (drag) {
                case 3: vv = Fl_Valuator_increment(v, Fl_Valuator_previous_value(v), delta * 100); break;
                case 2: vv = Fl_Valuator_increment(v, Fl_Valuator_previous_value(v), delta * 10); break;
                default: vv = Fl_Valuator_increment(v, Fl_Valuator_previous_value(v), delta); break;
            }
            vv = Fl_Valuator_round(v, vv);
            Fl_Valuator_handle_drag(v, self->soft_ ? Fl_Valuator_softclamp(v, vv) : Fl_Valuator_clamp(v, vv));
            return 1;
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
