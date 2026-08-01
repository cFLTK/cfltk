/*
 * cfltk - Fl_Dial.h
 *
 * C translation of FLTK 1.3 FL/Fl_Dial.H.
 *
 * Original class : Fl_Dial : public Fl_Valuator (own draw()/handle(); a
 *                   circular knob, type() selects a dot indicator, a
 *                   line/needle indicator, or a filled pie-slice meter).
 * New C structure : struct Fl_Dial { Fl_Valuator valuator; short a1,
 *                    a2; }; reused as-is by Fl_Fill_Dial/Fl_Line_Dial
 *                    (constructor-only type() variants).
 * Vtbl            : fl_dial_ops.
 * Known differences: the dot/line indicator is drawn as a plain
 *                   trig-computed dot/needle from the hub instead of
 *                   upstream's small rotated polygon shapes (which rely
 *                   on a push/translate/scale/rotate transform matrix
 *                   stack cfltk's drawing API doesn't have -- see
 *                   docs/DESIGN.md). The value<->angle mapping and drag
 *                   interaction are translated exactly; only the
 *                   indicator's pixel art differs. FL_FILL_DIAL (the pie
 *                   meter) has no such difference: it's a near-direct
 *                   translation using fl_pie(), which cfltk already has.
 */
#ifndef CFLTK_FL_DIAL_H
#define CFLTK_FL_DIAL_H

#include "cfltk/Fl_Valuator.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FL_NORMAL_DIAL 0
#define FL_LINE_DIAL   1
#define FL_FILL_DIAL   2

typedef struct Fl_Dial {
    Fl_Valuator valuator;
    short a1, a2;
} Fl_Dial;

extern const Fl_WidgetOps fl_dial_ops;

void Fl_Dial_init(Fl_Dial *self, int x, int y, int w, int h, const char *label);
Fl_Dial *Fl_Dial_new(int x, int y, int w, int h, const char *label);

void Fl_Dial_draw(Fl_Widget *self);
int Fl_Dial_handle(Fl_Widget *self, int event);

static inline short Fl_Dial_angle1(const Fl_Dial *self) { return self->a1; }
static inline void Fl_Dial_set_angle1(Fl_Dial *self, short a) { self->a1 = a; }
static inline short Fl_Dial_angle2(const Fl_Dial *self) { return self->a2; }
static inline void Fl_Dial_set_angle2(Fl_Dial *self, short a) { self->a2 = a; }
static inline void Fl_Dial_set_angles(Fl_Dial *self, short a, short b) { self->a1 = a; self->a2 = b; }

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_DIAL_H */
