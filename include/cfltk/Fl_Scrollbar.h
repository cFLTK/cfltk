/*
 * cfltk - Fl_Scrollbar.h
 *
 * C translation of FLTK 1.3 FL/Fl_Scrollbar.H.
 *
 * Original class : Fl_Scrollbar : public Fl_Slider (own draw()/handle();
 *                   adds arrow buttons at each end that step by
 *                   linesize(), auto-repeating while held via a timer).
 * New C structure : struct Fl_Scrollbar { Fl_Slider slider; int
 *                    linesize_, pushed_; }.
 * Vtbl            : fl_scrollbar_ops.
 * Known differences: no "gtk+" scheme arrow-glyph variant (see
 *                    docs/DESIGN.md's "no color schemes" note) -- always
 *                    draws the plain-scheme triangle arrows.
 */
#ifndef CFLTK_FL_SCROLLBAR_H
#define CFLTK_FL_SCROLLBAR_H

#include "cfltk/Fl_Slider.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Fl_Scrollbar {
    Fl_Slider slider;
    int linesize_;
    int pushed_; /* 0 none, 1/2 = arrow buttons, 5/6 = page-jump areas, 8 = on the knob */
} Fl_Scrollbar;

extern const Fl_WidgetOps fl_scrollbar_ops;

void Fl_Scrollbar_init(Fl_Scrollbar *self, int x, int y, int w, int h, const char *label);
Fl_Scrollbar *Fl_Scrollbar_new(int x, int y, int w, int h, const char *label);
void Fl_Scrollbar_destroy(Fl_Widget *self);

void Fl_Scrollbar_draw(Fl_Widget *self);
int Fl_Scrollbar_handle(Fl_Widget *self, int event);

static inline int Fl_Scrollbar_value(const Fl_Scrollbar *self) { return (int)Fl_Valuator_value(&self->slider.valuator); }
static inline int Fl_Scrollbar_set_value(Fl_Scrollbar *self, int p) { return Fl_Valuator_set_value(&self->slider.valuator, (double)p); }
static inline int Fl_Scrollbar_set_value_range(Fl_Scrollbar *self, int pos, int size, int first, int total) {
    return Fl_Slider_scrollvalue(&self->slider, pos, size, first, total);
}

static inline int Fl_Scrollbar_linesize(const Fl_Scrollbar *self) { return self->linesize_; }
static inline void Fl_Scrollbar_set_linesize(Fl_Scrollbar *self, int v) { self->linesize_ = v; }

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_SCROLLBAR_H */
