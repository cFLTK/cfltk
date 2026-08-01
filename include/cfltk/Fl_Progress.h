/*
 * cfltk - Fl_Progress.h
 *
 * C translation of FLTK 1.3 FL/Fl_Progress.H / src/Fl_Progress.cxx.
 *
 * Original class : Fl_Progress : public Fl_Widget (own draw() only).
 * New C structure : struct Fl_Progress { Fl_Widget widget; float
 *                    value_, minimum_, maximum_; }.
 * Vtbl            : fl_progress_ops (own draw(); handle()/resize() are
 *                    the plain Fl_Widget defaults).
 */
#ifndef CFLTK_FL_PROGRESS_H
#define CFLTK_FL_PROGRESS_H

#include "cfltk/Fl_Widget.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Fl_Progress {
    Fl_Widget widget;
    float value_;
    float minimum_;
    float maximum_;
} Fl_Progress;

extern const Fl_WidgetOps fl_progress_ops;

void Fl_Progress_init(Fl_Progress *self, int x, int y, int w, int h, const char *label);
Fl_Progress *Fl_Progress_new(int x, int y, int w, int h, const char *label);

void Fl_Progress_draw(Fl_Widget *self_w);

static inline float Fl_Progress_maximum(const Fl_Progress *self) { return self->maximum_; }
static inline void Fl_Progress_set_maximum(Fl_Progress *self, float v) { self->maximum_ = v; Fl_Widget_redraw(&self->widget); }
static inline float Fl_Progress_minimum(const Fl_Progress *self) { return self->minimum_; }
static inline void Fl_Progress_set_minimum(Fl_Progress *self, float v) { self->minimum_ = v; Fl_Widget_redraw(&self->widget); }
static inline float Fl_Progress_value(const Fl_Progress *self) { return self->value_; }
static inline void Fl_Progress_set_value(Fl_Progress *self, float v) { self->value_ = v; Fl_Widget_redraw(&self->widget); }

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_PROGRESS_H */
