/*
 * cfltk - Fl_Spinner.h
 *
 * C translation of FLTK 1.3 FL/Fl_Spinner.H (a header-only class
 * upstream, plus its one out-of-line constructor in src/Fl_Group.cxx).
 *
 * Original class : Fl_Spinner : public Fl_Group (owns an Fl_Input plus
 *                   two Fl_Repeat_Button children, embedded by value).
 * New C structure : struct Fl_Spinner { Fl_Group group; ...; Fl_Input
 *                   *input_; Fl_Button *up_button_, *down_button_; }.
 * Ownership       : input_/up_button_/down_button_ are heap-allocated
 *                   (via Fl_Input_new()/Fl_Repeat_Button_new()) and
 *                   auto-added as real children during
 *                   Fl_Spinner_init() (Fl_Group_current() is this
 *                   group at that point, same auto-add mechanism every
 *                   other widget uses) -- NOT embedded by value the way
 *                   upstream's C++ object embeds them. cfltk's group
 *                   teardown (Fl_Group_clear(), see Fl_Group.c)
 *                   unconditionally free()s every child, which would
 *                   corrupt the heap if a child's storage were embedded
 *                   inside the parent instead of its own malloc'd
 *                   block; heap-allocating them here makes destruction
 *                   "just work" via the same path as any other Group's
 *                   children, at the cost of 3 extra small allocations
 *                   per spinner. No functional difference from
 *                   upstream.
 * Vtbl            : draw()/destroy() are inherited unchanged from
 *                   Fl_Group (no visible box of its own -- the input
 *                   field and buttons draw their own); handle()/
 *                   resize() are overridden.
 */
#ifndef CFLTK_FL_SPINNER_H
#define CFLTK_FL_SPINNER_H

#include "cfltk/Fl_Group.h"
#include "cfltk/Fl_Input.h"
#include "cfltk/Fl_Button.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Fl_Spinner {
    Fl_Group group;
    double value_;
    double minimum_;
    double maximum_;
    double step_;
    const char *format_;
    Fl_Input *input_;
    Fl_Button *up_button_;
    Fl_Button *down_button_;
} Fl_Spinner;

extern const Fl_WidgetOps fl_spinner_ops;

void Fl_Spinner_init(Fl_Spinner *self, int x, int y, int w, int h, const char *label);
Fl_Spinner *Fl_Spinner_new(int x, int y, int w, int h, const char *label);

static inline const char *Fl_Spinner_format(const Fl_Spinner *self) { return self->format_; }
void Fl_Spinner_set_format(Fl_Spinner *self, const char *f);

static inline double Fl_Spinner_maximum(const Fl_Spinner *self) { return self->maximum_; }
static inline void Fl_Spinner_set_maximum(Fl_Spinner *self, double m) { self->maximum_ = m; }
static inline double Fl_Spinner_minimum(const Fl_Spinner *self) { return self->minimum_; }
static inline void Fl_Spinner_set_minimum(Fl_Spinner *self, double m) { self->minimum_ = m; }
static inline void Fl_Spinner_range(Fl_Spinner *self, double a, double b) {
    self->minimum_ = a;
    self->maximum_ = b;
}

static inline double Fl_Spinner_step(const Fl_Spinner *self) { return self->step_; }
void Fl_Spinner_set_step(Fl_Spinner *self, double s);

static inline Fl_Color Fl_Spinner_textcolor(const Fl_Spinner *self) { return Fl_Input_textcolor(self->input_); }
static inline void Fl_Spinner_set_textcolor(Fl_Spinner *self, Fl_Color c) { Fl_Input_set_textcolor(self->input_, c); }
static inline Fl_Font Fl_Spinner_textfont(const Fl_Spinner *self) { return Fl_Input_textfont(self->input_); }
static inline void Fl_Spinner_set_textfont(Fl_Spinner *self, Fl_Font f) { Fl_Input_set_textfont(self->input_, f); }
static inline Fl_Fontsize Fl_Spinner_textsize(const Fl_Spinner *self) { return Fl_Input_textsize(self->input_); }
static inline void Fl_Spinner_set_textsize(Fl_Spinner *self, Fl_Fontsize s) { Fl_Input_set_textsize(self->input_, s); }

/** The input field's numeric representation: FL_INT_INPUT or FL_FLOAT_INPUT (Fl_Input.h). Also updates format(). */
static inline unsigned char Fl_Spinner_type(const Fl_Spinner *self) { return Fl_Input_input_type(self->input_); }
void Fl_Spinner_set_type(Fl_Spinner *self, unsigned char v);

static inline double Fl_Spinner_value(const Fl_Spinner *self) { return self->value_; }
void Fl_Spinner_set_value(Fl_Spinner *self, double v);

static inline Fl_Color Fl_Spinner_color(const Fl_Spinner *self) { return Fl_Widget_color(&self->input_->widget); }
static inline void Fl_Spinner_set_color(Fl_Spinner *self, Fl_Color v) { Fl_Widget_set_color(&self->input_->widget, v); }
static inline Fl_Color Fl_Spinner_selection_color(const Fl_Spinner *self) { return Fl_Widget_selection_color(&self->input_->widget); }
static inline void Fl_Spinner_set_selection_color(Fl_Spinner *self, Fl_Color v) { Fl_Widget_set_selection_color(&self->input_->widget, v); }

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_SPINNER_H */
