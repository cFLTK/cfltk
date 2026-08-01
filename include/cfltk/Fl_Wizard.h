/*
 * cfltk - Fl_Wizard.h
 *
 * C translation of FLTK 1.3 FL/Fl_Wizard.H.
 *
 * Original class : Fl_Wizard : public Fl_Group -- shows exactly one
 *                   child ("pane") at a time, switched under program
 *                   control (next()/prev()/value()) rather than by
 *                   clicking tabs like Fl_Tabs. Navigation buttons are
 *                   the caller's responsibility.
 * New C structure : struct Fl_Wizard { Fl_Group group; Fl_Widget
 *                    *value_; }, embedding Fl_Group as its first
 *                    member.
 * Vtbl            : fl_wizard_ops -- draw() is overridden (draws the
 *                    box then only the one visible child); handle()/
 *                    resize() reuse Fl_Group's.
 * Known differences:
 *   - value(Fl_Widget*) doesn't reset the window's mouse cursor to
 *     the default shape on pane switch (upstream:
 *     `window()->cursor(FL_CURSOR_DEFAULT)`, meant to un-stick an
 *     I-beam cursor left over from a text widget on the previous
 *     pane) -- cfltk's Fl_Window has no cursor-shape API yet,
 *     consistent with the same note in Fl_Text_Display.h.
 */
#ifndef CFLTK_FL_WIZARD_H
#define CFLTK_FL_WIZARD_H

#include "cfltk/Fl_Group.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Fl_Wizard {
    Fl_Group group;
    Fl_Widget *value_;
} Fl_Wizard;

extern const Fl_WidgetOps fl_wizard_ops;

void Fl_Wizard_init(Fl_Wizard *self, int x, int y, int w, int h, const char *label);
Fl_Wizard *Fl_Wizard_new(int x, int y, int w, int h, const char *label);

void Fl_Wizard_next(Fl_Wizard *self);
void Fl_Wizard_prev(Fl_Wizard *self);
/* Returns the currently visible child (if more than one child was
 * left visible, e.g. by mistake, all but the first found are hidden
 * as a side effect -- matches upstream exactly). */
Fl_Widget *Fl_Wizard_value(Fl_Wizard *self);
void Fl_Wizard_set_value(Fl_Wizard *self, Fl_Widget *kid);

#ifdef __cplusplus
}
#endif

#endif
