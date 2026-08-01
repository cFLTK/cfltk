/*
 * cfltk - Fl_Adjuster.h
 *
 * C translation of FLTK 1.3 FL/Fl_Adjuster.H.
 *
 * Original class : Fl_Adjuster : public Fl_Valuator (own draw()/
 *                   handle()/value_damage(); a 3-button "slider" -- the
 *                   widest of the three buttons adjusts by 100*step()
 *                   per pixel dragged, the middle by 10*step(), the
 *                   narrowest by step(); clicking (no drag) nudges by
 *                   10x that button's per-pixel rate, Shift+click nudges
 *                   the other way).
 * New C structure : struct Fl_Adjuster { Fl_Valuator valuator; int
 *                    drag; int ix; int soft_; }.
 * Vtbl            : fl_adjuster_ops.
 * Known differences: the three buttons' fast/medium/slow bitmap glyphs
 *                   (upstream ships them as embedded XBM art --
 *                   fastarrow/mediumarrow/slowarrow) are replaced with
 *                   1/2/3 plain triangles via fl_polygon3(), the same
 *                   substitution already used for Fl_Counter's step
 *                   buttons (see Fl_Counter.h) -- cfltk has no image
 *                   support yet. Button layout, sizing, drag math and
 *                   the step-size-per-button mapping are a direct port.
 */
#ifndef CFLTK_FL_ADJUSTER_H
#define CFLTK_FL_ADJUSTER_H

#include "cfltk/Fl_Valuator.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Fl_Adjuster {
    Fl_Valuator valuator;
    int drag; /* 0 none, 1/2/3 = slow/medium/fast button currently pushed */
    int ix;
    int soft_;
} Fl_Adjuster;

extern const Fl_WidgetOps fl_adjuster_ops;

void Fl_Adjuster_init(Fl_Adjuster *self, int x, int y, int w, int h, const char *label);
Fl_Adjuster *Fl_Adjuster_new(int x, int y, int w, int h, const char *label);

void Fl_Adjuster_draw(Fl_Widget *self);
int Fl_Adjuster_handle(Fl_Widget *self, int event);

static inline void Fl_Adjuster_set_soft(Fl_Adjuster *self, int s) { self->soft_ = s; }
static inline int Fl_Adjuster_soft(const Fl_Adjuster *self) { return self->soft_; }

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_ADJUSTER_H */
