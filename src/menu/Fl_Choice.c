/*
 * cfltk - Fl_Choice.c
 * See include/cfltk/Fl_Choice.h for the class-conversion notes.
 * Translated from src/Fl_Choice.cxx (default-scheme rendering path only).
 */
#include <stdlib.h>

#include "cfltk/Fl_Choice.h"
#include "cfltk/Fl.h"
#include "cfltk/fl_draw.h"

const Fl_WidgetOps fl_choice_ops = {
    Fl_Choice_draw,
    Fl_Choice_handle,
    NULL, NULL, NULL,
    Fl_Menu__destroy,
    NULL, NULL
};

void Fl_Choice_init(Fl_Menu_ *self, int x, int y, int w, int h, const char *label) {
    Fl_Menu__init(self, &fl_choice_ops, x, y, w, h, label);
    Fl_Widget_set_align(&self->widget, FL_ALIGN_LEFT);
    Fl_Widget_set_when(&self->widget, FL_WHEN_RELEASE);
    self->textfont_ = FL_HELVETICA;
    self->widget.box = FL_FLAT_BOX;
    self->down_box_ = FL_BORDER_BOX;
}

Fl_Menu_ *Fl_Choice_new(int x, int y, int w, int h, const char *label) {
    Fl_Menu_ *self = (Fl_Menu_ *)malloc(sizeof(Fl_Menu_));
    Fl_Choice_init(self, x, y, w, h, label);
    return self;
}

int Fl_Choice_set_value_item(Fl_Menu_ *self, const Fl_Menu_Item *v) {
    if (!Fl_Menu_set_value(self, v)) return 0;
    Fl_Widget_redraw(&self->widget);
    return 1;
}

int Fl_Choice_set_value(Fl_Menu_ *self, int v) {
    if (v == -1) return Fl_Choice_set_value_item(self, NULL);
    if (v < 0 || v >= Fl_Menu_size(self) - 1) return 0;
    return Fl_Choice_set_value_item(self, self->menu_ + v);
}

void Fl_Choice_draw(Fl_Widget *self_w) {
    Fl_Menu_ *self = (Fl_Menu_ *)self_w;
    uchar btype = FL_DOWN_BOX;
    int dx = fl_box_dx(btype), dy = fl_box_dy(btype);
    int H = self_w->h - 2 * dy;
    int W = (H > 20) ? 20 : H;
    int X = self_w->x + self_w->w - W - dx;
    int Y = self_w->y + dy;
    int w1 = (W - 4) / 3;
    int x1, y1;

    if (w1 < 1) w1 = 1;
    x1 = X + (W - 2 * w1 - 1) / 2;
    y1 = Y + (H - w1 - 1) / 2;

    if (fl_contrast(self->textcolor_, FL_BACKGROUND2_COLOR) == self->textcolor_)
        fl_draw_box(btype, self_w->x, self_w->y, self_w->w, self_w->h, FL_BACKGROUND2_COLOR);
    else
        fl_draw_box(btype, self_w->x, self_w->y, self_w->w, self_w->h, fl_lighter(self_w->color));

    fl_draw_box(FL_UP_BOX, X, Y, W, H, self_w->color);
    fl_color(Fl_Widget_active_r(self_w) ? Fl_Widget_labelcolor(self_w) : fl_inactive(Fl_Widget_labelcolor(self_w)));
    fl_polygon3(x1, y1, x1 + w1, y1 + w1, x1 + 2 * w1, y1);

    W += 2 * dx;

    if (self->value_) {
        Fl_Label l;
        int xx = self_w->x + dx, yy = self_w->y + dy + 1, ww = self_w->w - W, hh = H - 2;
        int active = Fl_Menu_Item_active(self->value_);

        l.value = self->value_->text;
        l.image = NULL;
        l.deimage = NULL;
        l.type = self->value_->labeltype_;
        l.font = self->value_->labelfont_ ? self->value_->labelfont_ : self->textfont_;
        l.size = self->value_->labelsize_ ? self->value_->labelsize_ : self->textsize_;
        l.color = self->value_->labelcolor_ ? self->value_->labelcolor_ : self->textcolor_;
        if (!active) l.color = fl_inactive(l.color);

        fl_push_clip(xx, yy, ww, hh);
        fl_label_draw(&l, xx + 3, yy, ww > 6 ? ww - 6 : 0, hh, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
        if (Fl_focus() == self_w) Fl_Widget_draw_focus(self_w, self_w->box, xx, yy, ww, hh);
        fl_pop_clip();
    }

    Fl_Widget_draw_label(self_w);
}

int Fl_Choice_handle(Fl_Widget *self_w, int e) {
    Fl_Menu_ *self = (Fl_Menu_ *)self_w;
    const Fl_Menu_Item *v;

    if (!self->menu_ || !self->menu_->text) return 0;

    switch (e) {
        case FL_ENTER:
        case FL_LEAVE:
            return 1;

        case FL_KEYBOARD:
            if (Fl_event_key() != ' ' || Fl_event_state_of(FL_SHIFT | FL_CTRL | FL_ALT | FL_META)) return 0;
            /* fallthrough */
        case FL_PUSH:
            if (Fl_visible_focus()) Fl_set_focus(self_w);
            v = Fl_Menu_Item_pulldown(self->menu_, self_w->x, self_w->y, self_w->w, self_w->h, self, self->value_, 0);
            if (!v || Fl_Menu_Item_submenu(v)) return 1;
            if (v != self->value_) Fl_Widget_redraw(self_w);
            Fl_Menu_picked(self, v);
            return 1;

        case FL_SHORTCUT:
            if (Fl_Widget_test_shortcut(self_w)) {
                v = Fl_Menu_Item_pulldown(self->menu_, self_w->x, self_w->y, self_w->w, self_w->h, self, self->value_, 0);
                if (!v || Fl_Menu_Item_submenu(v)) return 1;
                if (v != self->value_) Fl_Widget_redraw(self_w);
                Fl_Menu_picked(self, v);
                return 1;
            }
            v = Fl_Menu_Item_test_shortcut(self->menu_);
            if (!v) return 0;
            if (v != self->value_) Fl_Widget_redraw(self_w);
            Fl_Menu_picked(self, v);
            return 1;

        case FL_FOCUS:
        case FL_UNFOCUS:
            if (Fl_visible_focus()) {
                Fl_Widget_redraw(self_w);
                return 1;
            }
            return 0;

        default:
            return 0;
    }
}
