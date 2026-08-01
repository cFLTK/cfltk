/*
 * cfltk - Fl_Color_Chooser.h
 *
 * C translation of FLTK 1.3 FL/Fl_Color_Chooser.H / src/Fl_Color_Chooser.cxx.
 *
 * Original class : Fl_Color_Chooser : public Fl_Group, composed of two
 *                   internal-only custom widgets (Flcc_HueBox: the
 *                   hue/saturation square; Flcc_ValueBox: the vertical
 *                   brightness slider), an Fl_Choice (rgb/byte/hex/hsv
 *                   mode), and three Flcc_Value_Input fields (a plain
 *                   Fl_Value_Input with a virtual format() override for
 *                   hex display) -- all embedded by value upstream.
 *                   Plus the free function fl_color_chooser(), which
 *                   pops up a small modal-ish dialog wrapping one.
 * New C structure : struct Fl_Color_Chooser { Fl_Group group; ...
 *                    heap-allocated child pointers ...; double hue_,
 *                    saturation_, value_, r_, g_, b_; }. Children are
 *                    heap-allocated and added as ordinary group
 *                    children instead of embedded by value (same
 *                    reasoning as Fl_Spinner.h's "Ownership" note).
 *                    Flcc_Value_Input's format() override becomes a
 *                    custom `value_damage` hook (Fl_Valuator.h's
 *                    existing extension point for exactly this need)
 *                    installed on rvalue/gvalue/bvalue.
 * Known differences:
 *   - The hue box / value box gradients are built into a full RGB
 *     buffer each redraw and blitted with fl_draw_image() (the raw-
 *     buffer form) instead of upstream's per-scanline streaming
 *     callback (fl_draw_image(callback,...)), which cfltk's
 *     Fl_Graphics_Driver doesn't have. Correctness-identical; the
 *     buffer is at most ~115x115x3 bytes, not a real cost.
 *   - fl_color_chooser()'s dialog is not truly modal (same known
 *     difference as fl_ask.h's dialogs -- no Fl::modal() event-
 *     redirection stack) and is not draggable-resizable via a window
 *     manager resize (Fl_Window::size_range() isn't implemented) --
 *     neither affects its actual use as a blocking "pick a color and
 *     come back" call.
 */
#ifndef CFLTK_FL_COLOR_CHOOSER_H
#define CFLTK_FL_COLOR_CHOOSER_H

#include "cfltk/Fl_Group.h"
#include "cfltk/Fl_Box.h"
#include "cfltk/Fl_Menu_.h"
#include "cfltk/Fl_Choice.h"
#include "cfltk/Fl_Value_Input.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    FL_COLOR_CHOOSER_M_RGB = 0,
    FL_COLOR_CHOOSER_M_BYTE = 1,
    FL_COLOR_CHOOSER_M_HEX = 2,
    FL_COLOR_CHOOSER_M_HSV = 3
};

/* Internal-use-only widgets, exposed only because C has no private
 * members -- do not construct these directly. */
typedef struct Flcc_HueBox {
    Fl_Widget widget;
    int px, py;
} Flcc_HueBox;

typedef struct Flcc_ValueBox {
    Fl_Widget widget;
    int py;
} Flcc_ValueBox;

typedef struct Fl_Color_Chooser {
    Fl_Group group;
    Flcc_HueBox *huebox;
    Flcc_ValueBox *valuebox;
    Fl_Menu_ *choice;
    Fl_Value_Input *rvalue, *gvalue, *bvalue;
    Fl_Box *resize_box;
    double hue_, saturation_, value_;
    double r_, g_, b_;
} Fl_Color_Chooser;

extern const Fl_WidgetOps fl_color_chooser_ops;

/* Recommended size is 200x95. Color is initialized to black. */
void Fl_Color_Chooser_init(Fl_Color_Chooser *self, int x, int y, int w, int h, const char *label);
Fl_Color_Chooser *Fl_Color_Chooser_new(int x, int y, int w, int h, const char *label);

/** rgb(0), byte(1), hex(2), or hsv(3) -- see the FL_COLOR_CHOOSER_M_* enum. */
static inline int Fl_Color_Chooser_mode(const Fl_Color_Chooser *self) { return Fl_Choice_value(self->choice); }
void Fl_Color_Chooser_set_mode(Fl_Color_Chooser *self, int new_mode);

static inline double Fl_Color_Chooser_hue(const Fl_Color_Chooser *self) { return self->hue_; }
static inline double Fl_Color_Chooser_saturation(const Fl_Color_Chooser *self) { return self->saturation_; }
static inline double Fl_Color_Chooser_value(const Fl_Color_Chooser *self) { return self->value_; }
static inline double Fl_Color_Chooser_r(const Fl_Color_Chooser *self) { return self->r_; }
static inline double Fl_Color_Chooser_g(const Fl_Color_Chooser *self) { return self->g_; }
static inline double Fl_Color_Chooser_b(const Fl_Color_Chooser *self) { return self->b_; }

/* Clamped (H modulo 6). Does not fire the callback. Returns non-zero
 * if the value actually changed. */
int Fl_Color_Chooser_hsv(Fl_Color_Chooser *self, double h, double s, double v);
/* Not clamped (out-of-range values produce "psychedelic" hue-box
 * effects, matching upstream exactly). Does not fire the callback. */
int Fl_Color_Chooser_rgb(Fl_Color_Chooser *self, double r, double g, double b);

void Fl_Color_Chooser_hsv2rgb(double h, double s, double v, double *r, double *g, double *b);
void Fl_Color_Chooser_rgb2hsv(double r, double g, double b, double *h, double *s, double *v);

/* Pops up a small dialog to pick an RGB color; r/g/b are read as the
 * initial color and written back on OK. cmode of -1 uses rgb mode
 * (matches Fl_Color_Chooser_set_mode()'s FL_COLOR_CHOOSER_M_* values).
 * Returns non-zero if the user picked OK. */
int fl_color_chooser_d(const char *name, double *r, double *g, double *b, int cmode);
/* Same, with r/g/b in 0-255. */
int fl_color_chooser_u(const char *name, unsigned char *r, unsigned char *g, unsigned char *b, int cmode);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_COLOR_CHOOSER_H */
