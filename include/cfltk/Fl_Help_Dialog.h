/*
 * cfltk - Fl_Help_Dialog.h
 *
 * C translation of FLTK 1.3 FL/Fl_Help_Dialog.H / src/Fl_Help_Dialog.cxx
 * (a fluid-generated wrapper: a small toolbar of back/forward/smaller/
 * larger/find controls above an Fl_Help_View, with a simple linear
 * back/forward history).
 *
 * Original class : Fl_Help_Dialog (not a widget -- a plain C++ object
 *                   that owns an Fl_Double_Window full of widgets,
 *                   fixed 100-entry back/forward history arrays).
 * New C structure : struct Fl_Help_Dialog { ... same fixed 100-entry
 *                    history arrays ...; Fl_Double_Window *window_;
 *                    Fl_Button *back_, *forward_, *smaller_, *larger_;
 *                    Fl_Input *find_; Fl_Help_View *view_; } --
 *                    heap-allocated children, same reasoning as
 *                    Fl_Spinner.h's "Ownership" note.
 * Known differences: relies on Fl_Help_View's own scope cuts (no
 *                    tables/images/selection -- see Fl_Help_View.h).
 */
#ifndef CFLTK_FL_HELP_DIALOG_H
#define CFLTK_FL_HELP_DIALOG_H

#include "cfltk/Fl_Double_Window.h"
#include "cfltk/Fl_Button.h"
#include "cfltk/Fl_Input.h"
#include "cfltk/Fl_Help_View.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CFLTK_HELP_DIALOG_HISTORY 100

typedef struct Fl_Help_Dialog {
    int index_;
    int max_;
    int line_[CFLTK_HELP_DIALOG_HISTORY];
    char file_[CFLTK_HELP_DIALOG_HISTORY][FL_PATH_MAX];
    int find_pos_;

    Fl_Double_Window *window_;
    Fl_Button *back_;
    Fl_Button *forward_;
    Fl_Button *smaller_;
    Fl_Button *larger_;
    Fl_Input *find_;
    Fl_Help_View *view_;
} Fl_Help_Dialog;

Fl_Help_Dialog *Fl_Help_Dialog_new(void);
void Fl_Help_Dialog_delete(Fl_Help_Dialog *self);

int Fl_Help_Dialog_h(const Fl_Help_Dialog *self);
int Fl_Help_Dialog_w(const Fl_Help_Dialog *self);
int Fl_Help_Dialog_x(const Fl_Help_Dialog *self);
int Fl_Help_Dialog_y(const Fl_Help_Dialog *self);
int Fl_Help_Dialog_visible(const Fl_Help_Dialog *self);

void Fl_Help_Dialog_show(Fl_Help_Dialog *self);
void Fl_Help_Dialog_hide(Fl_Help_Dialog *self);
void Fl_Help_Dialog_position(Fl_Help_Dialog *self, int x, int y);
void Fl_Help_Dialog_resize(Fl_Help_Dialog *self, int x, int y, int w, int h);

void Fl_Help_Dialog_load(Fl_Help_Dialog *self, const char *f);
void Fl_Help_Dialog_set_value(Fl_Help_Dialog *self, const char *f);
const char *Fl_Help_Dialog_value(const Fl_Help_Dialog *self);

void Fl_Help_Dialog_set_textsize(Fl_Help_Dialog *self, Fl_Fontsize s);
Fl_Fontsize Fl_Help_Dialog_textsize(const Fl_Help_Dialog *self);

void Fl_Help_Dialog_set_topline(Fl_Help_Dialog *self, int n);
void Fl_Help_Dialog_set_topline_target(Fl_Help_Dialog *self, const char *n);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_HELP_DIALOG_H */
