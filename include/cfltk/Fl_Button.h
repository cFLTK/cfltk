/*
 * cfltk - Fl_Button.h
 *
 * C translation of FLTK 1.3 FL/Fl_Button.H.
 *
 * Original class : Fl_Button : public Fl_Widget
 * New C structure : struct Fl_Button { Fl_Widget widget; int shortcut_;
 *                    char value_, oldval; uchar down_box_; };
 * Inheritance     : Fl_Button IS-A Fl_Widget through embedding. This is
 *                    also the base every FLTK button subclass
 *                    (Fl_Toggle_Button, Fl_Radio_Button, Fl_Light_Button,
 *                    Fl_Round_Button, Fl_Check_Button, Fl_Return_Button,
 *                    Fl_Repeat_Button) builds on by setting type()/
 *                    down_box() rather than overriding draw()/handle();
 *                    cfltk widgets for those follow the same pattern
 *                    once ported (see docs/DESIGN.md).
 * Vtbl            : fl_button_ops overrides draw() and handle(); resize/
 *                    show/hide fall back to Fl_Widget's defaults.
 * Ownership       : none beyond Fl_Widget's.
 * Known differences:
 *   - label_shortcut()/test_shortcut() decode a single byte, not full
 *     UTF-8, so only ASCII '&x' label shortcuts work for now.
 */
#ifndef CFLTK_FL_BUTTON_H
#define CFLTK_FL_BUTTON_H

#include "cfltk/Fl_Widget.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Values for type(). FL_RADIO_BUTTON reuses FL_RESERVED_TYPE exactly as
 * upstream does, to stay numerically compatible. */
#define FL_NORMAL_BUTTON  0
#define FL_TOGGLE_BUTTON  1
#define FL_RADIO_BUTTON   (FL_RESERVED_TYPE + 2)
#define FL_HIDDEN_BUTTON  3

typedef struct Fl_Button {
    Fl_Widget widget;
    int shortcut_;
    char value_;
    char oldval;
    uchar down_box_;
} Fl_Button;

extern const Fl_WidgetOps fl_button_ops;

void Fl_Button_init(Fl_Button *self, int x, int y, int w, int h, const char *label);
Fl_Button *Fl_Button_new(int x, int y, int w, int h, const char *label);

void Fl_Button_draw(Fl_Widget *self);
int Fl_Button_handle(Fl_Widget *self, int event);

/* Returns 1 if calling this changed value(), 0 if it was already v. */
int Fl_Button_set_value(Fl_Button *self, int v);
static inline char Fl_Button_value(const Fl_Button *self) { return self->value_; }

static inline int Fl_Button_set(Fl_Button *self) { return Fl_Button_set_value(self, 1); }
static inline int Fl_Button_clear(Fl_Button *self) { return Fl_Button_set_value(self, 0); }

/* Sets this radio button on and clears every sibling in the same parent
 * group whose type() is FL_RADIO_BUTTON. Only meaningful on buttons
 * with type() == FL_RADIO_BUTTON, exactly like upstream. */
void Fl_Button_setonly(Fl_Button *self);

/* Briefly (~150ms, via Fl_add_timeout) shows the button in its pressed
 * state then releases it -- what a FL_NORMAL_BUTTON does in response to
 * a keyboard activation (Space while focused, or Enter on a
 * Fl_Return_Button). Exposed (protected upstream) because
 * Fl_Return_Button's handle() calls it directly. */
void Fl_Button_simulate_key_action(Fl_Button *self);

static inline Fl_Shortcut Fl_Button_shortcut(const Fl_Button *self) { return (Fl_Shortcut)self->shortcut_; }
static inline void Fl_Button_set_shortcut(Fl_Button *self, Fl_Shortcut s) { self->shortcut_ = (int)s; }

static inline uchar Fl_Button_down_box(const Fl_Button *self) { return self->down_box_; }
static inline void Fl_Button_set_down_box(Fl_Button *self, uchar b) { self->down_box_ = b; }

/* Back-compatibility aliases for selection_color(), as upstream keeps. */
static inline Fl_Color Fl_Button_down_color(const Fl_Button *self) { return Fl_Widget_selection_color(&self->widget); }
static inline void Fl_Button_set_down_color(Fl_Button *self, Fl_Color c) { Fl_Widget_set_selection_color(&self->widget, c); }

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_BUTTON_H */
