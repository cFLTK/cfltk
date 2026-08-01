/*
 * cfltk - Fl_Progress.c
 * See include/cfltk/Fl_Progress.h for the class-conversion notes.
 * Translated from src/Fl_Progress.cxx.
 */
#include <stdlib.h>

#include "cfltk/Fl_Progress.h"
#include "cfltk/fl_draw.h"

const Fl_WidgetOps fl_progress_ops = {
    Fl_Progress_draw,
    NULL, /* handle: Fl_Widget's default (ignores everything) */
    NULL, /* resize: Fl_Widget's default */
    NULL, /* show: Fl_Widget's default */
    NULL, /* hide: Fl_Widget's default */
    Fl_Widget_base_destroy,
    NULL, /* as_group */
    NULL  /* as_window */
};

void Fl_Progress_init(Fl_Progress *self, int x, int y, int w, int h, const char *label) {
    Fl_Widget_init(&self->widget, &fl_progress_ops, x, y, w, h, label);
    Fl_Widget_set_align(&self->widget, FL_ALIGN_INSIDE);
    Fl_Widget_set_box(&self->widget, FL_DOWN_BOX);
    Fl_Widget_set_colors(&self->widget, FL_BACKGROUND2_COLOR, FL_YELLOW);
    self->minimum_ = 0.0f;
    self->maximum_ = 100.0f;
    self->value_ = 0.0f;
}

Fl_Progress *Fl_Progress_new(int x, int y, int w, int h, const char *label) {
    Fl_Progress *self = (Fl_Progress *)malloc(sizeof(Fl_Progress));
    Fl_Progress_init(self, x, y, w, h, label);
    return self;
}

void Fl_Progress_draw(Fl_Widget *self_w) {
    Fl_Progress *self = (Fl_Progress *)self_w;
    int progress;
    int bx, by, bw, bh;
    int tx, tw;

    bx = fl_box_dx(self_w->box);
    by = fl_box_dy(self_w->box);
    bw = fl_box_dw(self_w->box);
    bh = fl_box_dh(self_w->box);

    tx = self_w->x + bx;
    tw = self_w->w - bw;

    if (self->maximum_ > self->minimum_)
        progress = (int)(self_w->w * (self->value_ - self->minimum_) / (self->maximum_ - self->minimum_) + 0.5f);
    else
        progress = 0;

    if (progress > 0) {
        Fl_Color c = Fl_Widget_labelcolor(self_w);
        Fl_Widget_set_labelcolor(self_w, fl_contrast(c, Fl_Widget_selection_color(self_w)));

        fl_push_clip(self_w->x, self_w->y, progress + bx, self_w->h);
        fl_draw_box(self_w->box, self_w->x, self_w->y, self_w->w, self_w->h,
                    Fl_Widget_active_r(self_w) ? Fl_Widget_selection_color(self_w) : fl_inactive(Fl_Widget_selection_color(self_w)));
        Fl_Widget_draw_label_in(self_w, tx, self_w->y + by, tw, self_w->h - bh);
        fl_pop_clip();

        Fl_Widget_set_labelcolor(self_w, c);

        if (progress < self_w->w) {
            fl_push_clip(tx + progress, self_w->y, self_w->w - progress, self_w->h);
            fl_draw_box(self_w->box, self_w->x, self_w->y, self_w->w, self_w->h,
                        Fl_Widget_active_r(self_w) ? Fl_Widget_color(self_w) : fl_inactive(Fl_Widget_color(self_w)));
            Fl_Widget_draw_label_in(self_w, tx, self_w->y + by, tw, self_w->h - bh);
            fl_pop_clip();
        }
    } else {
        fl_draw_box(self_w->box, self_w->x, self_w->y, self_w->w, self_w->h,
                    Fl_Widget_active_r(self_w) ? Fl_Widget_color(self_w) : fl_inactive(Fl_Widget_color(self_w)));
        Fl_Widget_draw_label_in(self_w, tx, self_w->y + by, tw, self_w->h - bh);
    }
}
