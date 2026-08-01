/*
 * cfltk - Fl_Input_Choice.h
 *
 * C translation of FLTK 1.3 FL/Fl_Input_Choice.H (+ the out-of-line
 * constructor in src/Fl_Group.cxx).
 *
 * Original class : Fl_Input_Choice : public Fl_Group (an Fl_Input plus
 *                   a private "InputMenuButton" -- an Fl_Menu_Button
 *                   subclass with its own tiny-triangle draw() --
 *                   embedded by value).
 * New C structure : struct Fl_Input_Choice { Fl_Group group; Fl_Input
 *                    *inp_; Fl_Menu_ *menu_; }. inp_/menu_ are heap-
 *                    allocated and added as ordinary children (same
 *                    reasoning as Fl_Spinner.h's "Ownership" note --
 *                    cfltk's group teardown always free()s every
 *                    child, incompatible with embedded-by-value
 *                    storage) instead of upstream's embedded-by-value
 *                    InputMenuButton/Fl_Input members.
 * Vtbl            : draw() is inherited unchanged from Fl_Group (no
 *                    box of its own -- the input field draws its own);
 *                    resize() is overridden. The private menu button's
 *                    own draw() override (the small triangle instead
 *                    of a normal down-arrow) lives in the .c file,
 *                    same as upstream's private nested class.
 */
#ifndef CFLTK_FL_INPUT_CHOICE_H
#define CFLTK_FL_INPUT_CHOICE_H

#include "cfltk/Fl_Group.h"
#include "cfltk/Fl_Input.h"
#include "cfltk/Fl_Menu_Button.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Fl_Input_Choice {
    Fl_Group group;
    Fl_Input *inp_;
    Fl_Menu_ *menu_;
} Fl_Input_Choice;

extern const Fl_WidgetOps fl_input_choice_ops;

void Fl_Input_Choice_init(Fl_Input_Choice *self, int x, int y, int w, int h, const char *label);
Fl_Input_Choice *Fl_Input_Choice_new(int x, int y, int w, int h, const char *label);

/** Adds one flat item to the menu (see Fl_Menu_.h's "Known differences" -- no "File/Open" path parsing). */
static inline void Fl_Input_Choice_add(Fl_Input_Choice *self, const char *s) { Fl_Menu_add(self->menu_, s, 0, NULL, NULL, 0); }
/** Combined changed() state of the input field and the choice itself. */
int Fl_Input_Choice_changed(const Fl_Input_Choice *self);
void Fl_Input_Choice_clear_changed(Fl_Input_Choice *self);
void Fl_Input_Choice_set_changed(Fl_Input_Choice *self);
static inline void Fl_Input_Choice_clear(Fl_Input_Choice *self) { Fl_Menu_clear(self->menu_); }

static inline uchar Fl_Input_Choice_down_box(const Fl_Input_Choice *self) { return Fl_Menu_down_box(self->menu_); }
static inline void Fl_Input_Choice_set_down_box(Fl_Input_Choice *self, uchar b) { Fl_Menu_set_down_box(self->menu_, b); }
static inline const Fl_Menu_Item *Fl_Input_Choice_menu(Fl_Input_Choice *self) { return Fl_Menu_menu(self->menu_); }
static inline void Fl_Input_Choice_set_menu(Fl_Input_Choice *self, const Fl_Menu_Item *m) { Fl_Menu_set_menu(self->menu_, m); }

static inline Fl_Color Fl_Input_Choice_textcolor(const Fl_Input_Choice *self) { return Fl_Input_textcolor(self->inp_); }
static inline void Fl_Input_Choice_set_textcolor(Fl_Input_Choice *self, Fl_Color c) { Fl_Input_set_textcolor(self->inp_, c); }
static inline Fl_Font Fl_Input_Choice_textfont(const Fl_Input_Choice *self) { return Fl_Input_textfont(self->inp_); }
static inline void Fl_Input_Choice_set_textfont(Fl_Input_Choice *self, Fl_Font f) { Fl_Input_set_textfont(self->inp_, f); }
static inline Fl_Fontsize Fl_Input_Choice_textsize(const Fl_Input_Choice *self) { return Fl_Input_textsize(self->inp_); }
static inline void Fl_Input_Choice_set_textsize(Fl_Input_Choice *self, Fl_Fontsize s) { Fl_Input_set_textsize(self->inp_, s); }

static inline const char *Fl_Input_Choice_value(const Fl_Input_Choice *self) { return Fl_Input_value(self->inp_); }
/** Sets the input field's text; does not affect the menu selection. */
static inline void Fl_Input_Choice_set_value(Fl_Input_Choice *self, const char *val) { Fl_Input_set_value_str(self->inp_, val); }
/** Selects item #val in the menu and loads its label into the input field. */
void Fl_Input_Choice_set_value_index(Fl_Input_Choice *self, int val);

static inline Fl_Menu_ *Fl_Input_Choice_menubutton(Fl_Input_Choice *self) { return self->menu_; }
static inline Fl_Input *Fl_Input_Choice_input(Fl_Input_Choice *self) { return self->inp_; }

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_INPUT_CHOICE_H */
