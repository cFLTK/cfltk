/*
 * cfltk - Fl_Slider.h
 *
 * C translation of FLTK 1.3 FL/Fl_Slider.H.
 *
 * Original class : Fl_Slider : public Fl_Valuator (own draw()/handle();
 *                   type() selects vertical/horizontal and plain/fill/
 *                   "nice" knob rendering).
 * New C structure : struct Fl_Slider { Fl_Valuator valuator; float
 *                    slider_size_; uchar slider_; }; reused as-is by
 *                    Fl_Fill_Slider/Fl_Hor_Slider/Fl_Hor_Fill_Slider/
 *                    Fl_Nice_Slider/Fl_Hor_Nice_Slider (all
 *                    constructor-only type()/box() variants, same
 *                    "subclasses that add no fields reuse the struct"
 *                    pattern as the button family) and as the base
 *                    struct for Fl_Value_Slider/Fl_Scrollbar (which do
 *                    add fields).
 * Vtbl            : fl_slider_ops.
 * Known differences: no "gtk+" scheme gripper decoration (see
 *                    docs/DESIGN.md's "no color schemes" note).
 */
#ifndef CFLTK_FL_SLIDER_H
#define CFLTK_FL_SLIDER_H

#include "cfltk/Fl_Valuator.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FL_VERT_SLIDER      0
#define FL_HOR_SLIDER       1
#define FL_VERT_FILL_SLIDER 2
#define FL_HOR_FILL_SLIDER  3
#define FL_VERT_NICE_SLIDER 4
#define FL_HOR_NICE_SLIDER  5

typedef struct Fl_Slider {
    Fl_Valuator valuator;
    float slider_size_;
    uchar slider_;
} Fl_Slider;

extern const Fl_WidgetOps fl_slider_ops;

void Fl_Slider_init(Fl_Slider *self, int x, int y, int w, int h, const char *label);
void Fl_Slider_init_typed(Fl_Slider *self, uchar type, int x, int y, int w, int h, const char *label);
Fl_Slider *Fl_Slider_new(int x, int y, int w, int h, const char *label);
Fl_Slider *Fl_Slider_new_typed(uchar type, int x, int y, int w, int h, const char *label);

void Fl_Slider_draw(Fl_Widget *self);
int Fl_Slider_handle(Fl_Widget *self, int event);
/* Draws/handles within an arbitrary sub-rectangle (protected upstream);
 * exposed so Fl_Value_Slider/Fl_Scrollbar can reuse them to draw the
 * slider track in less than their full widget area. */
void Fl_Slider_draw_in(Fl_Slider *self, int x, int y, int w, int h);
int Fl_Slider_handle_in(Fl_Slider *self, int event, int x, int y, int w, int h);

int Fl_Slider_scrollvalue(Fl_Slider *self, int pos, int size, int first, int total);
void Fl_Slider_set_bounds(Fl_Slider *self, double a, double b);

static inline float Fl_Slider_slider_size(const Fl_Slider *self) { return self->slider_size_; }
void Fl_Slider_set_slider_size(Fl_Slider *self, double v);
static inline uchar Fl_Slider_slider(const Fl_Slider *self) { return self->slider_; }
static inline void Fl_Slider_set_slider(Fl_Slider *self, uchar boxtype) { self->slider_ = boxtype; }

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_SLIDER_H */
