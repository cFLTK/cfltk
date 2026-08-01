/*
 * cfltk - Fl_Pack.h
 *
 * C translation of FLTK 1.3 FL/Fl_Pack.H.
 *
 * Original class : Fl_Pack : public Fl_Group -- lays its visible
 *                   children out end-to-end (vertically by default,
 *                   horizontally if type()==HORIZONTAL), packing them
 *                   tight against each other plus an optional fixed
 *                   spacing, then resizes itself to exactly surround
 *                   them. One designated child (resizable(), NULL by
 *                   default unlike plain Fl_Group) absorbs any leftover
 *                   space instead of being packed at its own size.
 * New C structure : struct Fl_Pack { Fl_Group group; int spacing_; },
 *                    embedding Fl_Group as its first member.
 * Vtbl            : fl_pack_ops -- draw() is overridden (the entire
 *                    class is really just this one method); handle()/
 *                    resize() reuse Fl_Group's.
 * Known differences: none -- this is a small, self-contained class
 *                    with no platform dependencies.
 */
#ifndef CFLTK_FL_PACK_H
#define CFLTK_FL_PACK_H

#include "cfltk/Fl_Group.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FL_PACK_VERTICAL 0
#define FL_PACK_HORIZONTAL 1

typedef struct Fl_Pack {
    Fl_Group group;
    int spacing_;
} Fl_Pack;

extern const Fl_WidgetOps fl_pack_ops;

void Fl_Pack_init(Fl_Pack *self, int x, int y, int w, int h, const char *label);
Fl_Pack *Fl_Pack_new(int x, int y, int w, int h, const char *label);

static inline int Fl_Pack_spacing(const Fl_Pack *self) { return self->spacing_; }
static inline void Fl_Pack_set_spacing(Fl_Pack *self, int i) { self->spacing_ = i; }
static inline uchar Fl_Pack_horizontal(const Fl_Pack *self) { return Fl_Widget_type(&self->group.widget); }

#ifdef __cplusplus
}
#endif

#endif
