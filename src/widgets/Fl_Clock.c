/*
 * cfltk - Fl_Clock.c
 * See include/cfltk/Fl_Clock.h for the class-conversion notes.
 * Translated from src/Fl_Clock.cxx.
 */
#include <stdlib.h>
#include <time.h>

#include "cfltk/Fl_Clock.h"
#include "cfltk/Fl.h"
#include "cfltk/fl_draw.h"

/* Original clock display written by Paul Haeberli at SGI; modifications
 * by Mark Overmars for Forms, further changes by Bill Spitzak for FLTK. */
static const float hourhand[4][2] = { { -0.5f, 0 }, { 0, 1.5f }, { 0.5f, 0 }, { 0, -7.0f } };
static const float minhand[4][2] = { { -0.5f, 0 }, { 0, 1.5f }, { 0.5f, 0 }, { 0, -11.5f } };
static const float sechand[4][2] = { { -0.1f, 0 }, { 0, 2.0f }, { 0.1f, 0 }, { 0, -11.5f } };

static void drawhand(double ang, const float v[4][2], Fl_Color fill, Fl_Color line) {
    int i;
    fl_push_matrix();
    fl_rotate(ang);
    fl_color(fill);
    fl_begin_polygon();
    for (i = 0; i < 4; i++) fl_vertex(v[i][0], v[i][1]);
    fl_end_polygon();
    fl_color(line);
    fl_begin_loop();
    for (i = 0; i < 4; i++) fl_vertex(v[i][0], v[i][1]);
    fl_end_loop();
    fl_pop_matrix();
}

static void drawhands(const Fl_Clock_Output *self, Fl_Color fill, Fl_Color line) {
    if (!Fl_Widget_active_r(&self->widget)) {
        fill = fl_inactive(fill);
        line = fl_inactive(line);
    }
    drawhand(-360 * (self->hour_ + self->minute_ / 60.0) / 12, hourhand, fill, line);
    drawhand(-360 * (self->minute_ + self->second_ / 60.0) / 60, minhand, fill, line);
    drawhand(-360 * (self->second_ / 60.0), sechand, fill, line);
}

static void clock_rect(double x, double y, double w, double h) {
    double r = x + w, t = y + h;
    fl_begin_polygon();
    fl_vertex(x, y);
    fl_vertex(r, y);
    fl_vertex(r, t);
    fl_vertex(x, t);
    fl_end_polygon();
}

static void draw_at(const Fl_Clock_Output *self, int X, int Y, int W, int H) {
    const Fl_Widget *self_w = &self->widget;
    int active = Fl_Widget_active_r(self_w);
    Fl_Color box_color = (Fl_Widget_type(self_w) == FL_ROUND_CLOCK) ? FL_GRAY : Fl_Widget_color(self_w);
    Fl_Color shadow_color = fl_color_average(box_color, FL_BLACK, 0.5f);
    int i;

    fl_draw_box(Fl_Widget_box(self_w), X, Y, W, H, box_color);
    fl_push_matrix();
    fl_translate(X + W / 2.0 - .5, Y + H / 2.0 - .5);
    fl_scale((W - 1) / 28.0, (H - 1) / 28.0);
    if (Fl_Widget_type(self_w) == FL_ROUND_CLOCK) {
        fl_color(active ? Fl_Widget_color(self_w) : fl_inactive(Fl_Widget_color(self_w)));
        fl_begin_polygon(); fl_circle(0, 0, 14); fl_end_polygon();
        fl_color(active ? FL_FOREGROUND_COLOR : fl_inactive(FL_FOREGROUND_COLOR));
        fl_begin_loop(); fl_circle(0, 0, 14); fl_end_loop();
    }
    /* shadows */
    fl_push_matrix();
    fl_translate(0.60, 0.60);
    drawhands(self, shadow_color, shadow_color);
    fl_pop_matrix();
    /* tick marks */
    fl_push_matrix();
    fl_color(active ? FL_FOREGROUND_COLOR : fl_inactive(FL_FOREGROUND_COLOR));
    for (i = 0; i < 12; i++) {
        if (i == 6) clock_rect(-0.5, 9, 1, 2);
        else if (i == 3 || i == 0 || i == 9) clock_rect(-0.5, 9.5, 1, 1);
        else clock_rect(-0.25, 9.5, .5, 1);
        fl_rotate(-30);
    }
    fl_pop_matrix();
    /* hands */
    drawhands(self, Fl_Widget_selection_color(self_w), FL_FOREGROUND_COLOR);
    fl_pop_matrix();
}

void Fl_Clock_Output_draw(Fl_Widget *self_w) {
    Fl_Clock_Output *self = (Fl_Clock_Output *)self_w;
    draw_at(self, self_w->x, self_w->y, self_w->w, self_w->h);
    Fl_Widget_draw_label(self_w);
}

void Fl_Clock_Output_set_hms(Fl_Clock_Output *self, int h, int m, int s) {
    if (h != self->hour_ || m != self->minute_ || s != self->second_) {
        self->hour_ = h;
        self->minute_ = m;
        self->second_ = s;
        self->value_ = (unsigned long)((h * 60 + m) * 60 + s);
        Fl_Widget_set_damage(&self->widget, FL_DAMAGE_CHILD);
    }
}

void Fl_Clock_Output_set_value(Fl_Clock_Output *self, unsigned long v) {
    struct tm *timeofday;
    time_t vv = (time_t)v;
    self->value_ = v;
    timeofday = localtime(&vv);
    Fl_Clock_Output_set_hms(self, timeofday->tm_hour, timeofday->tm_min, timeofday->tm_sec);
}

const Fl_WidgetOps fl_clock_output_ops = {
    Fl_Clock_Output_draw,
    NULL, /* handle: Fl_Widget's default */
    NULL, /* resize: Fl_Widget's default */
    NULL, /* show: Fl_Widget's default */
    NULL, /* hide: Fl_Widget's default */
    Fl_Widget_base_destroy,
    NULL,
    NULL
};

void Fl_Clock_Output_init(Fl_Clock_Output *self, int x, int y, int w, int h, const char *label) {
    Fl_Widget_init(&self->widget, &fl_clock_output_ops, x, y, w, h, label);
    Fl_Widget_set_box(&self->widget, FL_UP_BOX);
    Fl_Widget_set_selection_color(&self->widget, fl_gray_ramp(5));
    Fl_Widget_set_align(&self->widget, FL_ALIGN_BOTTOM);
    self->hour_ = 0;
    self->minute_ = 0;
    self->second_ = 0;
    self->value_ = 0;
}

Fl_Clock_Output *Fl_Clock_Output_new(int x, int y, int w, int h, const char *label) {
    Fl_Clock_Output *self = (Fl_Clock_Output *)malloc(sizeof(Fl_Clock_Output));
    Fl_Clock_Output_init(self, x, y, w, h, label);
    return self;
}

/* -------------------------------------------------------------------
 * Fl_Clock: adds a 1-second ticker while shown.
 * ---------------------------------------------------------------- */

static void tick(void *v) {
    Fl_Clock_Output_set_value((Fl_Clock_Output *)v, (unsigned long)time(NULL));
    Fl_add_timeout(1.0, tick, v);
}

static int Fl_Clock_handle(Fl_Widget *self_w, int event) {
    switch (event) {
        case FL_SHOW: tick(self_w); break;
        case FL_HIDE: Fl_remove_timeout(tick, self_w); break;
    }
    return Fl_Widget_default_handle(self_w, event);
}

static void Fl_Clock_destroy(Fl_Widget *self_w) {
    Fl_remove_timeout(tick, self_w);
    Fl_Widget_base_destroy(self_w);
}

const Fl_WidgetOps fl_clock_ops = {
    Fl_Clock_Output_draw,
    Fl_Clock_handle,
    NULL,
    NULL,
    NULL,
    Fl_Clock_destroy,
    NULL,
    NULL
};

void Fl_Clock_init(Fl_Clock_Output *self, uchar t, int x, int y, int w, int h, const char *label) {
    Fl_Clock_Output_init(self, x, y, w, h, label);
    self->widget.ops = &fl_clock_ops;
    Fl_Widget_set_type(&self->widget, t);
    Fl_Widget_set_box(&self->widget, t == FL_ROUND_CLOCK ? FL_NO_BOX : FL_UP_BOX);
}

Fl_Clock_Output *Fl_Clock_new(int x, int y, int w, int h, const char *label) {
    Fl_Clock_Output *self = (Fl_Clock_Output *)malloc(sizeof(Fl_Clock_Output));
    Fl_Clock_Output_init(self, x, y, w, h, label);
    self->widget.ops = &fl_clock_ops;
    return self;
}

Fl_Clock_Output *Fl_Clock_new_with_type(uchar t, int x, int y, int w, int h, const char *label) {
    Fl_Clock_Output *self = (Fl_Clock_Output *)malloc(sizeof(Fl_Clock_Output));
    Fl_Clock_init(self, t, x, y, w, h, label);
    return self;
}

Fl_Clock_Output *Fl_Round_Clock_new(int x, int y, int w, int h, const char *label) {
    Fl_Clock_Output *self = Fl_Clock_new_with_type(FL_ROUND_CLOCK, x, y, w, h, label);
    Fl_Widget_set_box(&self->widget, FL_NO_BOX);
    return self;
}
