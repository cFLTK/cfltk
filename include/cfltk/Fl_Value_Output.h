/*
 * cfltk - Fl_Value_Output.h
 *
 * C translation of FLTK 1.3 FL/Fl_Value_Output.H.
 *
 * Original class : Fl_Value_Output : public Fl_Valuator (own draw()/
 *                   handle(); a read-only-looking numeric field, no text
 *                   editor -- displays value() as text and, if step() is
 *                   non-zero, lets the user click-drag left/right to
 *                   adjust it).
 * New C structure : struct Fl_Value_Output { Fl_Valuator valuator;
 *                    Fl_Font textfont_; Fl_Fontsize textsize_; uchar
 *                    soft_; Fl_Color textcolor_; }.
 * Vtbl            : fl_value_output_ops.
 * Ownership       : none beyond Fl_Valuator's.
 */
#ifndef CFLTK_FL_VALUE_OUTPUT_H
#define CFLTK_FL_VALUE_OUTPUT_H

#include "cfltk/Fl_Valuator.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Fl_Value_Output {
    Fl_Valuator valuator;
    Fl_Font textfont_;
    Fl_Fontsize textsize_;
    uchar soft_;
    Fl_Color textcolor_;
} Fl_Value_Output;

extern const Fl_WidgetOps fl_value_output_ops;

void Fl_Value_Output_init(Fl_Value_Output *self, int x, int y, int w, int h, const char *label);
Fl_Value_Output *Fl_Value_Output_new(int x, int y, int w, int h, const char *label);

void Fl_Value_Output_draw(Fl_Widget *self);
int Fl_Value_Output_handle(Fl_Widget *self, int event);

static inline void Fl_Value_Output_set_soft(Fl_Value_Output *self, int s) { self->soft_ = (uchar)s; }
static inline int Fl_Value_Output_soft(const Fl_Value_Output *self) { return self->soft_; }

static inline Fl_Font Fl_Value_Output_textfont(const Fl_Value_Output *self) { return self->textfont_; }
static inline void Fl_Value_Output_set_textfont(Fl_Value_Output *self, Fl_Font f) { self->textfont_ = f; }
static inline Fl_Fontsize Fl_Value_Output_textsize(const Fl_Value_Output *self) { return self->textsize_; }
static inline void Fl_Value_Output_set_textsize(Fl_Value_Output *self, Fl_Fontsize s) { self->textsize_ = s; }
static inline Fl_Color Fl_Value_Output_textcolor(const Fl_Value_Output *self) { return self->textcolor_; }
static inline void Fl_Value_Output_set_textcolor(Fl_Value_Output *self, Fl_Color c) { self->textcolor_ = c; }

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_VALUE_OUTPUT_H */
