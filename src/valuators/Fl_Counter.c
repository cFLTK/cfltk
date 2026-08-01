/*
 * cfltk - Fl_Counter.c
 * See include/cfltk/Fl_Counter.h for the class-conversion notes.
 * Translated from src/Fl_Counter.cxx (arrow rendering simplified, see
 * header).
 */
#include <stdlib.h>

#include "cfltk/Fl_Counter.h"
#include "cfltk/Fl.h"
#include "cfltk/fl_draw.h"

#define INITIAL_REPEAT 0.5
#define REPEAT 0.1

const Fl_WidgetOps fl_counter_ops = {
    Fl_Counter_draw,
    Fl_Counter_handle,
    NULL, NULL, NULL,
    Fl_Counter_destroy,
    NULL, NULL
};

void Fl_Counter_init(Fl_Counter *self, int x, int y, int w, int h, const char *label) {
    Fl_Valuator_init(&self->valuator, &fl_counter_ops, x, y, w, h, label);
    self->valuator.widget.box = FL_UP_BOX;
    Fl_Widget_set_selection_color(&self->valuator.widget, FL_INACTIVE_COLOR);
    Fl_Widget_set_align(&self->valuator.widget, FL_ALIGN_BOTTOM);
    Fl_Valuator_set_bounds(&self->valuator, -1000000.0, 1000000.0);
    Fl_Valuator_set_step_ratio(&self->valuator, 1, 10);
    self->lstep_ = 1.0;
    self->mouseobj = 0;
    self->textfont_ = FL_HELVETICA;
    self->textsize_ = FL_NORMAL_SIZE;
    self->textcolor_ = FL_FOREGROUND_COLOR;
}

Fl_Counter *Fl_Counter_new(int x, int y, int w, int h, const char *label) {
    Fl_Counter *self = (Fl_Counter *)malloc(sizeof(Fl_Counter));
    Fl_Counter_init(self, x, y, w, h, label);
    return self;
}

static void repeat_callback(void *v);

void Fl_Counter_destroy(Fl_Widget *self_w) {
    Fl_Counter *self = (Fl_Counter *)self_w;
    Fl_remove_timeout(repeat_callback, self);
    Fl_Widget_base_destroy(self_w);
}

/* count=1 draws a single triangle, count=2 draws two side by side
 * (upstream's "<<"/">>" double-arrow buttons); dir<0 points left. */
static void draw_arrow(int x, int y, int w, int h, int dir, int count, Fl_Color col) {
    int i;
    int step = w / (count + 1);
    fl_color(col);
    for (i = 0; i < count; i++) {
        int cx = x + step * (i + 1);
        int cy = y + h / 2;
        int aw = step > 10 ? 5 : step / 2;
        int ah = h / 4 > 5 ? 5 : h / 4;
        if (ah < 2) ah = 2;
        if (dir < 0) fl_polygon3(cx + aw, cy - ah, cx + aw, cy + ah, cx - aw, cy);
        else fl_polygon3(cx - aw, cy - ah, cx - aw, cy + ah, cx + aw, cy);
    }
}

void Fl_Counter_draw(Fl_Widget *self_w) {
    Fl_Counter *self = (Fl_Counter *)self_w;
    Fl_Valuator *v = &self->valuator;
    int i;
    uchar boxtype[5];
    Fl_Color selcolor;
    int xx[5], ww[5];
    char str[128];

    boxtype[0] = self_w->box;
    if (boxtype[0] == FL_UP_BOX) boxtype[0] = FL_DOWN_BOX;
    if (boxtype[0] == FL_THIN_UP_BOX) boxtype[0] = FL_THIN_DOWN_BOX;
    for (i = 1; i < 5; i++) boxtype[i] = (self->mouseobj == (uchar)i) ? fl_down(self_w->box) : self_w->box;

    if (Fl_Widget_type(self_w) == FL_NORMAL_COUNTER) {
        int W = self_w->w * 15 / 100;
        xx[1] = self_w->x; ww[1] = W;
        xx[2] = self_w->x + W; ww[2] = W;
        xx[0] = self_w->x + 2 * W; ww[0] = self_w->w - 4 * W;
        xx[3] = self_w->x + self_w->w - 2 * W; ww[3] = W;
        xx[4] = self_w->x + self_w->w - W; ww[4] = W;
    } else {
        int W = self_w->w * 20 / 100;
        xx[1] = 0; ww[1] = 0;
        xx[2] = self_w->x; ww[2] = W;
        xx[0] = self_w->x + W; ww[0] = self_w->w - 2 * W;
        xx[3] = self_w->x + self_w->w - W; ww[3] = W;
        xx[4] = 0; ww[4] = 0;
    }

    fl_draw_box(boxtype[0], xx[0], self_w->y, ww[0], self_w->h, FL_BACKGROUND2_COLOR);
    fl_font(self->textfont_, self->textsize_);
    fl_color(Fl_Widget_active_r(self_w) ? self->textcolor_ : fl_inactive(self->textcolor_));
    Fl_Valuator_format(v, str);
    {
        Fl_Label l;
        l.value = str; l.image = NULL; l.deimage = NULL; l.type = FL_NORMAL_LABEL;
        l.font = self->textfont_; l.size = self->textsize_;
        l.color = Fl_Widget_active_r(self_w) ? self->textcolor_ : fl_inactive(self->textcolor_);
        l.align = FL_ALIGN_CENTER;
        fl_label_draw(&l, xx[0], self_w->y, ww[0], self_w->h, FL_ALIGN_CENTER);
    }
    if (Fl_focus() == self_w) Fl_Widget_draw_focus(self_w, boxtype[0], xx[0], self_w->y, ww[0], self_w->h);
    if (!(self_w->damage & FL_DAMAGE_ALL)) return;

    selcolor = Fl_Widget_active_r(self_w) ? Fl_Widget_labelcolor(self_w) : fl_inactive(Fl_Widget_labelcolor(self_w));

    if (Fl_Widget_type(self_w) == FL_NORMAL_COUNTER) {
        fl_draw_box(boxtype[1], xx[1], self_w->y, ww[1], self_w->h, self_w->color);
        draw_arrow(xx[1], self_w->y, ww[1], self_w->h, -1, 2, selcolor);
    }
    fl_draw_box(boxtype[2], xx[2], self_w->y, ww[2], self_w->h, self_w->color);
    draw_arrow(xx[2], self_w->y, ww[2], self_w->h, -1, 1, selcolor);
    fl_draw_box(boxtype[3], xx[3], self_w->y, ww[3], self_w->h, self_w->color);
    draw_arrow(xx[3], self_w->y, ww[3], self_w->h, 1, 1, selcolor);
    if (Fl_Widget_type(self_w) == FL_NORMAL_COUNTER) {
        fl_draw_box(boxtype[4], xx[4], self_w->y, ww[4], self_w->h, self_w->color);
        draw_arrow(xx[4], self_w->y, ww[4], self_w->h, 1, 2, selcolor);
    }
}

static void increment_cb(Fl_Counter *self) {
    Fl_Valuator *v = &self->valuator;
    double val = Fl_Valuator_value(v);
    if (!self->mouseobj) return;
    switch (self->mouseobj) {
        case 1: val -= self->lstep_; break;
        case 2: val = Fl_Valuator_increment(v, val, -1); break;
        case 3: val = Fl_Valuator_increment(v, val, 1); break;
        case 4: val += self->lstep_; break;
        default: break;
    }
    Fl_Valuator_handle_drag(v, Fl_Valuator_clamp(v, Fl_Valuator_round(v, val)));
}

static void repeat_callback(void *vptr) {
    Fl_Counter *self = (Fl_Counter *)vptr;
    if (self->mouseobj) {
        Fl_add_timeout(REPEAT, repeat_callback, self);
        increment_cb(self);
    }
}

static int calc_mouseobj(Fl_Counter *self) {
    Fl_Widget *self_w = &self->valuator.widget;
    if (Fl_Widget_type(self_w) == FL_NORMAL_COUNTER) {
        int W = self_w->w * 15 / 100;
        if (Fl_event_inside_rect(self_w->x, self_w->y, W, self_w->h)) return 1;
        if (Fl_event_inside_rect(self_w->x + W, self_w->y, W, self_w->h)) return 2;
        if (Fl_event_inside_rect(self_w->x + self_w->w - 2 * W, self_w->y, W, self_w->h)) return 3;
        if (Fl_event_inside_rect(self_w->x + self_w->w - W, self_w->y, W, self_w->h)) return 4;
    } else {
        int W = self_w->w * 20 / 100;
        if (Fl_event_inside_rect(self_w->x, self_w->y, W, self_w->h)) return 2;
        if (Fl_event_inside_rect(self_w->x + self_w->w - W, self_w->y, W, self_w->h)) return 3;
    }
    return -1;
}

int Fl_Counter_handle(Fl_Widget *self_w, int event) {
    Fl_Counter *self = (Fl_Counter *)self_w;
    Fl_Valuator *v = &self->valuator;
    int i;

    switch (event) {
        case FL_RELEASE:
            if (self->mouseobj) {
                Fl_remove_timeout(repeat_callback, self);
                self->mouseobj = 0;
                Fl_Widget_redraw(self_w);
            }
            Fl_Valuator_handle_release(v);
            return 1;
        case FL_PUSH:
            if (Fl_visible_focus()) Fl_set_focus(self_w);
            Fl_Valuator_handle_push(v);
            /* fallthrough */
        case FL_DRAG:
            i = calc_mouseobj(self);
            if (i != self->mouseobj) {
                Fl_remove_timeout(repeat_callback, self);
                self->mouseobj = (uchar)(i < 0 ? 0 : i);
                if (i >= 0) Fl_add_timeout(INITIAL_REPEAT, repeat_callback, self);
                increment_cb(self);
                Fl_Widget_redraw(self_w);
            }
            return 1;
        case FL_KEYBOARD:
            switch (Fl_event_key()) {
                case FL_Left:
                    Fl_Valuator_handle_drag(v, Fl_Valuator_clamp(v, Fl_Valuator_increment(v, Fl_Valuator_value(v), -1)));
                    return 1;
                case FL_Right:
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
