/*
 * cfltk - Fl_Counter.h
 *
 * C translation of FLTK 1.3 FL/Fl_Counter.H.
 *
 * Original class : Fl_Counter : public Fl_Valuator (own draw()/handle();
 *                   a numeric field flanked by step buttons, auto-
 *                   repeating while held via a timer).
 * New C structure : struct Fl_Counter { Fl_Valuator valuator; Fl_Font
 *                    textfont_; Fl_Fontsize textsize_; Fl_Color
 *                    textcolor_; double lstep_; uchar mouseobj; };
 *                    reused as-is by Fl_Simple_Counter (constructor-only:
 *                    type(FL_SIMPLE_COUNTER)).
 * Vtbl            : fl_counter_ops.
 * Known differences: the step-button arrows are drawn as plain
 *                   triangles via fl_polygon3() instead of upstream's
 *                   "@-4<"/"@-4>" fl_draw_symbol() glyphs -- cfltk
 *                   hasn't ported the '@'-string symbol mini-language
 *                   fl_draw_symbol() depends on. Same arrow count/
 *                   direction/position, different pixel art.
 */
#ifndef CFLTK_FL_COUNTER_H
#define CFLTK_FL_COUNTER_H

#include "cfltk/Fl_Valuator.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FL_NORMAL_COUNTER 0
#define FL_SIMPLE_COUNTER 1

typedef struct Fl_Counter {
    Fl_Valuator valuator;
    Fl_Font textfont_;
    Fl_Fontsize textsize_;
    Fl_Color textcolor_;
    double lstep_;
    uchar mouseobj; /* 0 none, 1/4 = large step buttons, 2/3 = small step buttons */
} Fl_Counter;

extern const Fl_WidgetOps fl_counter_ops;

void Fl_Counter_init(Fl_Counter *self, int x, int y, int w, int h, const char *label);
Fl_Counter *Fl_Counter_new(int x, int y, int w, int h, const char *label);
void Fl_Counter_destroy(Fl_Widget *self);

void Fl_Counter_draw(Fl_Widget *self);
int Fl_Counter_handle(Fl_Widget *self, int event);

static inline void Fl_Counter_set_lstep(Fl_Counter *self, double a) { self->lstep_ = a; }
static inline double Fl_Counter_lstep(const Fl_Counter *self) { return self->lstep_; }
static inline void Fl_Counter_set_step2(Fl_Counter *self, double a, double b) { Fl_Valuator_set_step(&self->valuator, a); self->lstep_ = b; }

static inline Fl_Font Fl_Counter_textfont(const Fl_Counter *self) { return self->textfont_; }
static inline void Fl_Counter_set_textfont(Fl_Counter *self, Fl_Font f) { self->textfont_ = f; }
static inline Fl_Fontsize Fl_Counter_textsize(const Fl_Counter *self) { return self->textsize_; }
static inline void Fl_Counter_set_textsize(Fl_Counter *self, Fl_Fontsize s) { self->textsize_ = s; }
static inline Fl_Color Fl_Counter_textcolor(const Fl_Counter *self) { return self->textcolor_; }
static inline void Fl_Counter_set_textcolor(Fl_Counter *self, Fl_Color c) { self->textcolor_ = c; }

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_COUNTER_H */
