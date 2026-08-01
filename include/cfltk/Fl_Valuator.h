/*
 * cfltk - Fl_Valuator.h
 *
 * C translation of FLTK 1.3 FL/Fl_Valuator.H.
 *
 * Original class : Fl_Valuator : public Fl_Widget (abstract-ish base for
 *                   every single-value control: sliders, dials, the
 *                   counter, the roller; holds value/range/step and the
 *                   push/drag/release bookkeeping every subclass reuses).
 * New C structure : struct Fl_Valuator { Fl_Widget widget; double
 *                   value_, previous_value_, min_, max_, A; int B; };
 *                   embedded as the first member by every concrete
 *                   valuator (Fl_Slider, Fl_Dial, Fl_Counter, Fl_Roller).
 * Vtbl            : none of its own -- upstream's Fl_Valuator has no
 *                   draw()/handle(), only helpers concrete subclasses
 *                   call from their own. Never instantiated directly.
 * Ownership       : none beyond Fl_Widget's.
 * Known differences: upstream's protected virtual value_damage() (run
 *                   whenever value_ changes, overridden by Fl_Value_Input
 *                   to resync its embedded Fl_Input's text and by
 *                   Fl_Adjuster to no-op) is a plain optional function
 *                   pointer field (`value_damage`) here instead, since
 *                   Fl_Valuator has no vtable of its own to hang a virtual
 *                   off of. NULL means "do what upstream's default does":
 *                   mark FL_DAMAGE_EXPOSE.
 */
#ifndef CFLTK_FL_VALUATOR_H
#define CFLTK_FL_VALUATOR_H

#include "cfltk/Fl_Widget.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FL_VERTICAL   0
#define FL_HORIZONTAL 1

typedef struct Fl_Valuator {
    Fl_Widget widget;
    double value_;
    double previous_value_;
    double min_, max_;
    double A;
    int B;
    /* Optional hook run whenever value_ changes via Fl_Valuator_set_value()/
     * _handle_drag(), in place of the default "just mark FL_DAMAGE_EXPOSE"
     * (upstream: the virtual value_damage()). NULL for every valuator
     * except Fl_Value_Input (needs to resync its embedded Fl_Input's text)
     * and Fl_Adjuster (appearance doesn't depend on value, so it's a no-op). */
    void (*value_damage)(struct Fl_Valuator *self);
} Fl_Valuator;

/* Shared init every concrete valuator's own _init() calls first. */
void Fl_Valuator_init(Fl_Valuator *self, const Fl_WidgetOps *ops, int x, int y, int w, int h, const char *label);

static inline int Fl_Valuator_horizontal(const Fl_Valuator *self) { return self->widget.type & FL_HORIZONTAL; }

static inline void Fl_Valuator_set_bounds(Fl_Valuator *self, double a, double b) { self->min_ = a; self->max_ = b; }
static inline double Fl_Valuator_minimum(const Fl_Valuator *self) { return self->min_; }
static inline void Fl_Valuator_set_minimum(Fl_Valuator *self, double a) { self->min_ = a; }
static inline double Fl_Valuator_maximum(const Fl_Valuator *self) { return self->max_; }
static inline void Fl_Valuator_set_maximum(Fl_Valuator *self, double a) { self->max_ = a; }
static inline void Fl_Valuator_set_range(Fl_Valuator *self, double a, double b) { self->min_ = a; self->max_ = b; }

static inline void Fl_Valuator_set_step_ratio(Fl_Valuator *self, double a, int b) { self->A = a; self->B = b; }
void Fl_Valuator_set_step(Fl_Valuator *self, double s);
static inline double Fl_Valuator_step(const Fl_Valuator *self) { return self->A / self->B; }
void Fl_Valuator_set_precision(Fl_Valuator *self, int digits);

static inline double Fl_Valuator_value(const Fl_Valuator *self) { return self->value_; }
/* Sets the value verbatim (not clamped/rounded); returns non-zero if it
 * actually changed. Use Fl_Valuator_clamp()/_round() first if needed. */
int Fl_Valuator_set_value(Fl_Valuator *self, double v);

/* Formats value() per step()'s implied precision ("%g" if step is 0,
 * otherwise "%.*f" with enough digits to show the step) into buffer,
 * which must have room for at least 128 bytes. Returns the length
 * written, matching snprintf(). */
int Fl_Valuator_format(const Fl_Valuator *self, char *buffer);
double Fl_Valuator_round(const Fl_Valuator *self, double v);
double Fl_Valuator_clamp(const Fl_Valuator *self, double v);
double Fl_Valuator_increment(const Fl_Valuator *self, double v, int n);

/* -------------------------------------------------------------------
 * Internal use: the push/drag/release bookkeeping every concrete
 * valuator's handle() calls into (protected upstream).
 * ---------------------------------------------------------------- */

static inline double Fl_Valuator_previous_value(const Fl_Valuator *self) { return self->previous_value_; }
static inline void Fl_Valuator_handle_push(Fl_Valuator *self) { self->previous_value_ = self->value_; }
double Fl_Valuator_softclamp(const Fl_Valuator *self, double v);
/* Sets value_ directly (bypassing the changed-check in
 * Fl_Valuator_set_value()), marks changed(), redraws, and fires the
 * callback if when() & FL_WHEN_CHANGED. What a drag in progress calls
 * on every mouse-move. */
void Fl_Valuator_handle_drag(Fl_Valuator *self, double v);
/* Fires the callback per when() & FL_WHEN_RELEASE, comparing against
 * the value captured by handle_push(). What FL_RELEASE calls. */
void Fl_Valuator_handle_release(Fl_Valuator *self);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_VALUATOR_H */
