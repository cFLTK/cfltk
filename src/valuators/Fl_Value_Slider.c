/*
 * cfltk - Fl_Value_Slider.c
 * See include/cfltk/Fl_Value_Slider.h for the class-conversion notes.
 * Translated from src/Fl_Value_Slider.cxx.
 */
#include <stdlib.h>

#include "cfltk/Fl_Value_Slider.h"
#include "cfltk/Fl.h"
#include "cfltk/fl_draw.h"

const Fl_WidgetOps fl_value_slider_ops = {
    Fl_Value_Slider_draw,
    Fl_Value_Slider_handle,
    NULL, NULL, NULL,
    Fl_Widget_base_destroy,
    NULL, NULL
};

void Fl_Value_Slider_init(Fl_Value_Slider *self, int x, int y, int w, int h, const char *label) {
    Fl_Slider_init(&self->slider, x, y, w, h, label);
    self->slider.valuator.widget.ops = &fl_value_slider_ops;
    Fl_Valuator_set_step_ratio(&self->slider.valuator, 1, 100);
    self->textfont_ = FL_HELVETICA;
    self->textsize_ = 10;
    self->textcolor_ = FL_FOREGROUND_COLOR;
}

Fl_Value_Slider *Fl_Value_Slider_new(int x, int y, int w, int h, const char *label) {
    Fl_Value_Slider *self = (Fl_Value_Slider *)malloc(sizeof(Fl_Value_Slider));
    Fl_Value_Slider_init(self, x, y, w, h, label);
    return self;
}

void Fl_Value_Slider_draw(Fl_Widget *self_w) {
    Fl_Value_Slider *self = (Fl_Value_Slider *)self_w;
    int sxx = self_w->x, syy = self_w->y, sww = self_w->w, shh = self_w->h;
    int bxx = self_w->x, byy = self_w->y, bww = self_w->w, bhh = self_w->h;
    char buf[128];

    if (Fl_Valuator_horizontal(&self->slider.valuator)) {
        bww = 35; sxx += 35; sww -= 35;
    } else {
        syy += 25; bhh = 25; shh -= 25;
    }

    if (self_w->damage & FL_DAMAGE_ALL) fl_draw_box(self_w->box, sxx, syy, sww, shh, self_w->color);
    Fl_Slider_draw_in(&self->slider, sxx + fl_box_dx(self_w->box), syy + fl_box_dy(self_w->box),
                       sww - fl_box_dw(self_w->box), shh - fl_box_dh(self_w->box));

    fl_draw_box(self_w->box, bxx, byy, bww, bhh, self_w->color);
    Fl_Valuator_format(&self->slider.valuator, buf);
    fl_font(self->textfont_, self->textsize_);
    fl_color(Fl_Widget_active_r(self_w) ? self->textcolor_ : fl_inactive(self->textcolor_));
    {
        Fl_Label l;
        l.value = buf; l.image = NULL; l.deimage = NULL; l.type = FL_NORMAL_LABEL;
        l.font = self->textfont_; l.size = self->textsize_; l.color = Fl_Widget_active_r(self_w) ? self->textcolor_ : fl_inactive(self->textcolor_);
        l.align = FL_ALIGN_CENTER;
        fl_label_draw(&l, bxx, byy, bww, bhh, FL_ALIGN_CLIP);
    }
}

int Fl_Value_Slider_handle(Fl_Widget *self_w, int event) {
    Fl_Value_Slider *self = (Fl_Value_Slider *)self_w;
    int sxx = self_w->x, syy = self_w->y, sww = self_w->w, shh = self_w->h;

    if (event == FL_PUSH && Fl_visible_focus()) {
        Fl_set_focus(self_w);
        Fl_Widget_redraw(self_w);
    }
    if (Fl_Valuator_horizontal(&self->slider.valuator)) { sxx += 35; sww -= 35; }
    else { syy += 25; shh -= 25; }

    return Fl_Slider_handle_in(&self->slider, event,
                                sxx + fl_box_dx(self_w->box), syy + fl_box_dy(self_w->box),
                                sww - fl_box_dw(self_w->box), shh - fl_box_dh(self_w->box));
}
