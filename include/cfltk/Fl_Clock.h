/*
 * cfltk - Fl_Clock.h
 *
 * C translation of FLTK 1.3 FL/Fl_Clock.H (+ FL/Fl_Round_Clock.H) /
 * src/Fl_Clock.cxx.
 *
 * Original class : Fl_Clock_Output : public Fl_Widget (displays a
 *                   program-supplied time, never updates itself);
 *                   Fl_Clock : public Fl_Clock_Output (adds a 1-second
 *                   Fl::add_timeout() ticker via handle()'s FL_SHOW/
 *                   FL_HIDE, removed again in its destructor);
 *                   Fl_Round_Clock : public Fl_Clock (type(FL_ROUND_
 *                   CLOCK), box(FL_NO_BOX) -- constructor-only).
 * New C structure : struct Fl_Clock_Output { Fl_Widget widget; int
 *                    hour_, minute_, second_; unsigned long value_; };
 *                    Fl_Clock/Fl_Round_Clock reuse it (own ops table
 *                    for the ticking behavior), same pattern as
 *                    Fl_Hold_Browser reusing struct Fl_Browser.
 * Vtbl            : fl_clock_output_ops (draw() only, handle()/destroy()
 *                    are Fl_Widget's defaults); fl_clock_ops (adds the
 *                    FL_SHOW/FL_HIDE-driven ticker to handle(), and a
 *                    destroy() that removes any still-pending timeout).
 */
#ifndef CFLTK_FL_CLOCK_H
#define CFLTK_FL_CLOCK_H

#include "cfltk/Fl_Widget.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FL_SQUARE_CLOCK  0
#define FL_ROUND_CLOCK   1
#define FL_ANALOG_CLOCK  FL_SQUARE_CLOCK
#define FL_DIGITAL_CLOCK FL_SQUARE_CLOCK /* not yet implemented, matches upstream */

typedef struct Fl_Clock_Output {
    Fl_Widget widget;
    int hour_;
    int minute_;
    int second_;
    unsigned long value_;
} Fl_Clock_Output;

extern const Fl_WidgetOps fl_clock_output_ops;
extern const Fl_WidgetOps fl_clock_ops;

void Fl_Clock_Output_init(Fl_Clock_Output *self, int x, int y, int w, int h, const char *label);
Fl_Clock_Output *Fl_Clock_Output_new(int x, int y, int w, int h, const char *label);

/** Sets the displayed time to `v` seconds since the Unix epoch (localtime). */
void Fl_Clock_Output_set_value(Fl_Clock_Output *self, unsigned long v);
void Fl_Clock_Output_set_hms(Fl_Clock_Output *self, int h, int m, int s);
static inline unsigned long Fl_Clock_Output_value(const Fl_Clock_Output *self) { return self->value_; }
static inline int Fl_Clock_Output_hour(const Fl_Clock_Output *self) { return self->hour_; }
static inline int Fl_Clock_Output_minute(const Fl_Clock_Output *self) { return self->minute_; }
static inline int Fl_Clock_Output_second(const Fl_Clock_Output *self) { return self->second_; }

void Fl_Clock_Output_draw(Fl_Widget *self_w);

/* Fl_Clock: adds the 1-second ticker; `t` is FL_SQUARE_CLOCK or FL_ROUND_CLOCK. */
void Fl_Clock_init(Fl_Clock_Output *self, uchar t, int x, int y, int w, int h, const char *label);
Fl_Clock_Output *Fl_Clock_new(int x, int y, int w, int h, const char *label);
Fl_Clock_Output *Fl_Clock_new_with_type(uchar t, int x, int y, int w, int h, const char *label);

Fl_Clock_Output *Fl_Round_Clock_new(int x, int y, int w, int h, const char *label);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_CLOCK_H */
