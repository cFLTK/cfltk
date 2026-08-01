/*
 * cfltk - Fl_Value_Input.h
 *
 * C translation of FLTK 1.3 FL/Fl_Value_Input.H.
 *
 * Original class : Fl_Value_Input : public Fl_Valuator (own draw()/
 *                   handle()/resize(); a numeric field with an embedded,
 *                   real Fl_Input for text editing, plus click-and-drag
 *                   adjustment when step() is non-zero. Upstream flags
 *                   its own embedding trick "may be a kludge?": the
 *                   Fl_Input member is *not* added via the normal
 *                   Fl_Group child mechanism -- Fl_Value_Input isn't
 *                   even an Fl_Group -- its parent() pointer is just
 *                   force-set to `this`, relying on Fl_Input needing
 *                   nothing from its parent beyond walking up to find
 *                   the enclosing Fl_Window.)
 * New C structure : struct Fl_Value_Input { Fl_Valuator valuator;
 *                    Fl_Input input; char soft_; }. The embedding
 *                    kludge survives translation unchanged and is safe
 *                    in cfltk for the same structural reason `FL_WIDGET()`
 *                    is: every widget struct (including Fl_Group) starts
 *                    with `Fl_Widget widget` as its first member, so
 *                    `(Fl_Group *)&self->valuator` is a valid pointer for
 *                    any code that only ever dereferences `->widget`
 *                    through it (parent-chain walks for window()/
 *                    visible_r()/active_r(), redraw bubbling). The one
 *                    thing that would NOT be safe -- calling
 *                    Fl_Group_add()/_remove() on that fake pointer, which
 *                    would read/write real Fl_Group fields (children
 *                    array etc.) through a non-Group struct -- is exactly
 *                    what Fl_Input_destroy()'s call into
 *                    Fl_Widget_base_destroy() would do if `input.widget.
 *                    parent` were still set when it runs; Fl_Value_Input_
 *                    destroy() clears it first, mirroring upstream's own
 *                    destructor un-kludge.
 * Vtbl            : fl_value_input_ops.
 * Ownership       : owns `input` as a plain embedded struct (not
 *                   heap-allocated, not a normal Fl_Group child).
 * Known differences: none beyond the embedding mechanics above -- value/
 *                   drag/keyboard/focus semantics are a direct port.
 */
#ifndef CFLTK_FL_VALUE_INPUT_H
#define CFLTK_FL_VALUE_INPUT_H

#include "cfltk/Fl_Valuator.h"
#include "cfltk/Fl_Input.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Fl_Value_Input {
    Fl_Valuator valuator;
    Fl_Input input;
    char soft_;
} Fl_Value_Input;

extern const Fl_WidgetOps fl_value_input_ops;

void Fl_Value_Input_init(Fl_Value_Input *self, int x, int y, int w, int h, const char *label);
Fl_Value_Input *Fl_Value_Input_new(int x, int y, int w, int h, const char *label);
void Fl_Value_Input_destroy(Fl_Widget *self);

void Fl_Value_Input_draw(Fl_Widget *self);
int Fl_Value_Input_handle(Fl_Widget *self, int event);
void Fl_Value_Input_resize(Fl_Widget *self, int x, int y, int w, int h);

static inline void Fl_Value_Input_set_soft(Fl_Value_Input *self, int s) { self->soft_ = (char)s; }
static inline int Fl_Value_Input_soft(const Fl_Value_Input *self) { return self->soft_; }

static inline Fl_Shortcut Fl_Value_Input_shortcut(const Fl_Value_Input *self) { return Fl_Input_shortcut(&self->input); }
static inline void Fl_Value_Input_set_shortcut(Fl_Value_Input *self, Fl_Shortcut s) { Fl_Input_set_shortcut(&self->input, s); }

static inline Fl_Font Fl_Value_Input_textfont(const Fl_Value_Input *self) { return Fl_Input_textfont(&self->input); }
static inline void Fl_Value_Input_set_textfont(Fl_Value_Input *self, Fl_Font f) { Fl_Input_set_textfont(&self->input, f); }
static inline Fl_Fontsize Fl_Value_Input_textsize(const Fl_Value_Input *self) { return Fl_Input_textsize(&self->input); }
static inline void Fl_Value_Input_set_textsize(Fl_Value_Input *self, Fl_Fontsize s) { Fl_Input_set_textsize(&self->input, s); }
static inline Fl_Color Fl_Value_Input_textcolor(const Fl_Value_Input *self) { return Fl_Input_textcolor(&self->input); }
static inline void Fl_Value_Input_set_textcolor(Fl_Value_Input *self, Fl_Color c) { Fl_Input_set_textcolor(&self->input, c); }
static inline Fl_Color Fl_Value_Input_cursor_color(const Fl_Value_Input *self) { return Fl_Input_cursor_color(&self->input); }
static inline void Fl_Value_Input_set_cursor_color(Fl_Value_Input *self, Fl_Color c) { Fl_Input_set_cursor_color(&self->input, c); }

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_VALUE_INPUT_H */
