/*
 * cfltk - Fl_Value_Slider.h
 *
 * C translation of FLTK 1.3 FL/Fl_Value_Slider.H.
 *
 * Original class : Fl_Value_Slider : public Fl_Slider (own draw()/
 *                   handle(); reserves a strip of the widget to show
 *                   the numeric value and draws the slider in what's
 *                   left).
 * New C structure : struct Fl_Value_Slider { Fl_Slider slider; Fl_Font
 *                    textfont_; Fl_Fontsize textsize_; Fl_Color
 *                    textcolor_; }; reused as-is by Fl_Hor_Value_Slider
 *                    (constructor-only: type(FL_HOR_SLIDER)).
 * Vtbl            : fl_value_slider_ops.
 */
#ifndef CFLTK_FL_VALUE_SLIDER_H
#define CFLTK_FL_VALUE_SLIDER_H

#include "cfltk/Fl_Slider.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Fl_Value_Slider {
    Fl_Slider slider;
    Fl_Font textfont_;
    Fl_Fontsize textsize_;
    Fl_Color textcolor_;
} Fl_Value_Slider;

extern const Fl_WidgetOps fl_value_slider_ops;

void Fl_Value_Slider_init(Fl_Value_Slider *self, int x, int y, int w, int h, const char *label);
Fl_Value_Slider *Fl_Value_Slider_new(int x, int y, int w, int h, const char *label);

void Fl_Value_Slider_draw(Fl_Widget *self);
int Fl_Value_Slider_handle(Fl_Widget *self, int event);

static inline Fl_Font Fl_Value_Slider_textfont(const Fl_Value_Slider *self) { return self->textfont_; }
static inline void Fl_Value_Slider_set_textfont(Fl_Value_Slider *self, Fl_Font f) { self->textfont_ = f; }
static inline Fl_Fontsize Fl_Value_Slider_textsize(const Fl_Value_Slider *self) { return self->textsize_; }
static inline void Fl_Value_Slider_set_textsize(Fl_Value_Slider *self, Fl_Fontsize s) { self->textsize_ = s; }
static inline Fl_Color Fl_Value_Slider_textcolor(const Fl_Value_Slider *self) { return self->textcolor_; }
static inline void Fl_Value_Slider_set_textcolor(Fl_Value_Slider *self, Fl_Color c) { self->textcolor_ = c; }

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_VALUE_SLIDER_H */
