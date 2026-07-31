/*
 * cfltk - Fl_Input.h
 *
 * C translation of FLTK 1.3 FL/Fl_Input_.H and FL/Fl_Input.H, merged
 * into one struct/vtable/file.
 *
 * Original class : Fl_Input_ : public Fl_Widget (all editing state and
 *                   logic; upstream keeps it as a separate "virtual base
 *                   class" with no draw()/handle() specifically so
 *                   people can subclass it with different key bindings)
 *                   and Fl_Input : public Fl_Input_ (adds draw()/
 *                   handle()/handle_key(), i.e. the only concrete,
 *                   instantiable class upstream actually ships).
 * New C structure : struct Fl_Input { Fl_Widget widget; ...editing
 *                   fields... }; merged because cfltk has no client that
 *                   needs the "same fields, different keymap" extension
 *                   point Fl_Input_ exists for -- if one shows up,
 *                   splitting fl_input_ops's handle() out is a small,
 *                   localized change (the fields already model exactly
 *                   what Fl_Input_ held). See docs/DESIGN.md for the
 *                   "subclasses that add no state reuse the struct
 *                   outright" pattern this follows in spirit, just
 *                   collapsed one step further since here the *base*
 *                   class was the one with no independent existence in
 *                   practice.
 * Vtbl            : fl_input_ops (own draw/handle/resize).
 * Ownership       : owns `buffer` (always a private copy -- see Known
 *                   differences). Frees it in destroy().
 * Known differences:
 *   - No zero-copy static_value() optimization: Fl_Input_set_value()
 *     always copies into the owned buffer (static_value()'s "alias the
 *     caller's string until the user edits it" trick is a memory/time
 *     optimization for rapidly-refreshed read-only displays, not a
 *     behavioral guarantee anything depends on).
 *   - No undo/redo (Fl_Input_undo() is not implemented).
 *   - No word-wrap for FL_MULTILINE_INPUT|FL_INPUT_WRAP; wrap() is
 *     accepted but has no effect. Multiline text still displays fully,
 *     just without wrapping -- horizontal scroll takes over instead.
 *   - No control-character '^X' expansion or tab-to-spaces expansion in
 *     display; raw bytes are handed to the font renderer as-is.
 *   - Cursor motion and replace() do not snap to UTF-8 character
 *     boundaries (see Fl_Widget.h's shortcut-decoding note for the same
 *     limitation elsewhere); ASCII text is unaffected.
 *   - Clipboard is in-process only (Fl_copy()/Fl_paste() in Fl.h).
 *   - No IME/composition support (Fl::compose()).
 */
#ifndef CFLTK_FL_INPUT_H
#define CFLTK_FL_INPUT_H

#include <string.h>

#include "cfltk/Fl_Widget.h"

#ifdef __cplusplus
extern "C" {
#endif

/* type() values (FL/Fl_Input_.H) */
#define FL_NORMAL_INPUT          0
#define FL_FLOAT_INPUT           1
#define FL_INT_INPUT             2
#define FL_HIDDEN_INPUT          3
#define FL_MULTILINE_INPUT       4
#define FL_SECRET_INPUT          5
#define FL_INPUT_TYPE            7
#define FL_INPUT_READONLY        8
#define FL_NORMAL_OUTPUT         (FL_NORMAL_INPUT | FL_INPUT_READONLY)
#define FL_MULTILINE_OUTPUT      (FL_MULTILINE_INPUT | FL_INPUT_READONLY)
#define FL_INPUT_WRAP            16
#define FL_MULTILINE_INPUT_WRAP  (FL_MULTILINE_INPUT | FL_INPUT_WRAP)
#define FL_MULTILINE_OUTPUT_WRAP (FL_MULTILINE_INPUT | FL_INPUT_READONLY | FL_INPUT_WRAP)

typedef struct Fl_Input {
    Fl_Widget widget;

    char *buffer;      /* owned, nul-terminated, capacity == bufsize */
    int size_;          /* bytes in use, excluding the nul */
    int bufsize;

    int position_;
    int mark_;

    int xscroll_, yscroll_;
    int maximum_size_;
    int shortcut_;

    Fl_Font textfont_;
    Fl_Fontsize textsize_;
    Fl_Color textcolor_;
    Fl_Color cursor_color_;

    int tab_nav_;
} Fl_Input;

extern const Fl_WidgetOps fl_input_ops;

void Fl_Input_init(Fl_Input *self, int x, int y, int w, int h, const char *label);
Fl_Input *Fl_Input_new(int x, int y, int w, int h, const char *label);

void Fl_Input_draw(Fl_Widget *self);
int Fl_Input_handle(Fl_Widget *self, int event);
void Fl_Input_resize(Fl_Widget *self, int x, int y, int w, int h);
void Fl_Input_destroy(Fl_Widget *self);

/* -------------------------------------------------------------------
 * Value
 * ---------------------------------------------------------------- */

/* Copies str (len bytes, or strlen(str) if len==0) into the widget and
 * moves position()/mark() to the end. Returns non-zero if the value
 * actually changed. */
int Fl_Input_set_value(Fl_Input *self, const char *str, int len);
static inline int Fl_Input_set_value_str(Fl_Input *self, const char *str) {
    return Fl_Input_set_value(self, str, str ? (int)strlen(str) : 0);
}
static inline const char *Fl_Input_value(const Fl_Input *self) { return self->buffer; }
static inline int Fl_Input_value_length(const Fl_Input *self) { return self->size_; }

/* -------------------------------------------------------------------
 * Editing
 * ---------------------------------------------------------------- */

/* Deletes [b,e) (order-independent) and inserts text (ilen bytes, or
 * strlen(text) if ilen==0 and text is non-NULL); moves position()/
 * mark() to the end of the insertion. Returns 0 if nothing changed. */
int Fl_Input_replace(Fl_Input *self, int b, int e, const char *text, int ilen);
static inline int Fl_Input_cut(Fl_Input *self) { return Fl_Input_replace(self, self->position_, self->mark_, NULL, 0); }
static inline int Fl_Input_cut_n(Fl_Input *self, int n) { return Fl_Input_replace(self, self->position_, self->position_ + n, NULL, 0); }
static inline int Fl_Input_cut_range(Fl_Input *self, int a, int b) { return Fl_Input_replace(self, a, b, NULL, 0); }
static inline int Fl_Input_insert(Fl_Input *self, const char *t, int l) {
    return Fl_Input_replace(self, self->position_, self->mark_, t, l);
}

/* Copies the current selection to clipboard 0 (mouse-style) or 1
 * (Ctrl-C-style); see Fl_copy() in Fl.h. Returns 0 if nothing selected. */
int Fl_Input_copy(Fl_Input *self, int clipboard);

static inline int Fl_Input_position(const Fl_Input *self) { return self->position_; }
static inline int Fl_Input_mark(const Fl_Input *self) { return self->mark_; }
/* Sets cursor (p) and selection anchor (m); bounds-checked. Returns 0 if
 * neither changed. */
int Fl_Input_set_position_mark(Fl_Input *self, int p, int m);
static inline int Fl_Input_set_position(Fl_Input *self, int p) { return Fl_Input_set_position_mark(self, p, p); }
static inline int Fl_Input_set_mark(Fl_Input *self, int m) { return Fl_Input_set_position_mark(self, self->position_, m); }

int Fl_Input_word_start(const Fl_Input *self, int i);
int Fl_Input_word_end(const Fl_Input *self, int i);
int Fl_Input_line_start(const Fl_Input *self, int i);
int Fl_Input_line_end(const Fl_Input *self, int i);

/* -------------------------------------------------------------------
 * Type / mode
 * ---------------------------------------------------------------- */

static inline int Fl_Input_input_type(const Fl_Input *self) { return self->widget.type & FL_INPUT_TYPE; }
static inline void Fl_Input_set_input_type(Fl_Input *self, int t) {
    self->widget.type = (uchar)(t | (self->widget.type & FL_INPUT_READONLY));
}
static inline int Fl_Input_readonly(const Fl_Input *self) { return self->widget.type & FL_INPUT_READONLY; }
static inline void Fl_Input_set_readonly(Fl_Input *self, int b) {
    if (b) self->widget.type |= FL_INPUT_READONLY;
    else self->widget.type = (uchar)(self->widget.type & ~FL_INPUT_READONLY);
}
static inline int Fl_Input_wrap(const Fl_Input *self) { return self->widget.type & FL_INPUT_WRAP; }
static inline void Fl_Input_set_wrap(Fl_Input *self, int b) {
    if (b) self->widget.type |= FL_INPUT_WRAP;
    else self->widget.type = (uchar)(self->widget.type & ~FL_INPUT_WRAP);
}
static inline void Fl_Input_set_tab_nav(Fl_Input *self, int v) { self->tab_nav_ = v; }
static inline int Fl_Input_tab_nav(const Fl_Input *self) { return self->tab_nav_; }

/* -------------------------------------------------------------------
 * Appearance
 * ---------------------------------------------------------------- */

static inline int Fl_Input_maximum_size(const Fl_Input *self) { return self->maximum_size_; }
static inline void Fl_Input_set_maximum_size(Fl_Input *self, int m) { self->maximum_size_ = m; }

static inline Fl_Shortcut Fl_Input_shortcut(const Fl_Input *self) { return (Fl_Shortcut)self->shortcut_; }
static inline void Fl_Input_set_shortcut(Fl_Input *self, Fl_Shortcut s) { self->shortcut_ = (int)s; }

static inline Fl_Font Fl_Input_textfont(const Fl_Input *self) { return self->textfont_; }
static inline void Fl_Input_set_textfont(Fl_Input *self, Fl_Font f) { self->textfont_ = f; }
static inline Fl_Fontsize Fl_Input_textsize(const Fl_Input *self) { return self->textsize_; }
static inline void Fl_Input_set_textsize(Fl_Input *self, Fl_Fontsize s) { self->textsize_ = s; }
static inline Fl_Color Fl_Input_textcolor(const Fl_Input *self) { return self->textcolor_; }
static inline void Fl_Input_set_textcolor(Fl_Input *self, Fl_Color c) { self->textcolor_ = c; }
static inline Fl_Color Fl_Input_cursor_color(const Fl_Input *self) { return self->cursor_color_; }
static inline void Fl_Input_set_cursor_color(Fl_Input *self, Fl_Color c) { self->cursor_color_ = c; }

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_INPUT_H */
