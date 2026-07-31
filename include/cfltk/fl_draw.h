/*
 * cfltk - fl_draw.h
 *
 * C translation of FLTK 1.3 FL/fl_draw.H.
 *
 * Original class : none (free functions forwarding to the single active
 *                   Fl_Graphics_Driver instance -- upstream already uses
 *                   a driver object accessed through a global pointer).
 * New C structure : Fl_Graphics_Driver, a plain vtable struct; the
 *                    platform backend installs one implementation via
 *                    fl_set_graphics_driver() during Fl_context init.
 *                    Every fl_* drawing call below is a thin wrapper that
 *                    forwards to fl_graphics_driver->something(...),
 *                    exactly mirroring the upstream inline forwarders.
 * Ownership       : the driver struct is backend-owned static storage;
 *                    cfltk never allocates or frees it.
 * Known differences:
 *   - No matrix/transform stack (fl_push_matrix family), no complex
 *     polygons, no offscreen/print surfaces yet -- out of scope for the
 *     Linux reference backend's first pass. See docs/DESIGN.md.
 *   - Box drawing keeps upstream's function-pointer-table design
 *     (fl_box_table[boxtype]) unchanged in spirit; see fl_draw_box().
 */
#ifndef CFLTK_FL_DRAW_H
#define CFLTK_FL_DRAW_H

#include <string.h>

#include "cfltk/Enumerations.h"
#include "cfltk/Fl_Widget.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Fl_Region_ *Fl_Region;

typedef struct Fl_Graphics_Driver {
    void (*color_index)(Fl_Color c);
    void (*color_rgb)(uchar r, uchar g, uchar b);
    Fl_Color (*current_color)(void);

    void (*push_clip)(int x, int y, int w, int h);
    void (*push_no_clip)(void);
    void (*pop_clip)(void);
    int  (*not_clipped)(int x, int y, int w, int h);
    int  (*clip_box)(int x, int y, int w, int h, int *X, int *Y, int *W, int *H);

    void (*line_style)(int style, int width, const char *dashes);

    void (*point)(int x, int y);
    void (*line)(int x, int y, int x1, int y1);
    void (*line3)(int x, int y, int x1, int y1, int x2, int y2);
    void (*xyline)(int x, int y, int x1);
    void (*yxline)(int x, int y, int y1);
    void (*rect)(int x, int y, int w, int h);
    void (*rectf)(int x, int y, int w, int h);
    void (*loop3)(int x, int y, int x1, int y1, int x2, int y2);
    void (*polygon3)(int x, int y, int x1, int y1, int x2, int y2);
    void (*arc)(int x, int y, int w, int h, double a1, double a2);
    void (*pie)(int x, int y, int w, int h, double a1, double a2);

    void (*font)(Fl_Font face, Fl_Fontsize size);
    Fl_Font (*current_font)(void);
    Fl_Fontsize (*current_size)(void);
    int (*height)(void);
    int (*descent)(void);
    double (*width)(const char *text, int n);

    void (*draw_text)(const char *str, int n, int x, int y);
} Fl_Graphics_Driver;

/* Installed by the platform backend before any drawing happens. */
void fl_set_graphics_driver(const Fl_Graphics_Driver *driver);
const Fl_Graphics_Driver *fl_graphics_driver(void);

/* ------------------------------------------------------------------ */
/* Color                                                               */
/* ------------------------------------------------------------------ */

static inline void fl_color(Fl_Color c) { fl_graphics_driver()->color_index(c); }
static inline void fl_color_rgb(uchar r, uchar g, uchar b) { fl_graphics_driver()->color_rgb(r, g, b); }
static inline Fl_Color fl_current_color(void) { return fl_graphics_driver()->current_color(); }

Fl_Color fl_inactive(Fl_Color c);
Fl_Color fl_contrast(Fl_Color fg, Fl_Color bg);
Fl_Color fl_color_average(Fl_Color c1, Fl_Color c2, float weight);
static inline Fl_Color fl_lighter(Fl_Color c) { return fl_color_average(c, FL_WHITE, 0.67f); }
static inline Fl_Color fl_darker(Fl_Color c) { return fl_color_average(c, FL_BLACK, 0.67f); }

/* ------------------------------------------------------------------ */
/* Clipping                                                            */
/* ------------------------------------------------------------------ */

static inline void fl_push_clip(int x, int y, int w, int h) { fl_graphics_driver()->push_clip(x, y, w, h); }
static inline void fl_push_no_clip(void) { fl_graphics_driver()->push_no_clip(); }
static inline void fl_pop_clip(void) { fl_graphics_driver()->pop_clip(); }
static inline int fl_not_clipped(int x, int y, int w, int h) { return fl_graphics_driver()->not_clipped(x, y, w, h); }
static inline int fl_clip_box(int x, int y, int w, int h, int *X, int *Y, int *W, int *H) {
    return fl_graphics_driver()->clip_box(x, y, w, h, X, Y, W, H);
}

/* ------------------------------------------------------------------ */
/* Primitives                                                          */
/* ------------------------------------------------------------------ */

enum {
    FL_SOLID       = 0,
    FL_DASH        = 1,
    FL_DOT         = 2,
    FL_DASHDOT     = 3,
    FL_DASHDOTDOT  = 4,

    FL_CAP_FLAT    = 0x100,
    FL_CAP_ROUND   = 0x200,
    FL_CAP_SQUARE  = 0x300,

    FL_JOIN_MITER  = 0x1000,
    FL_JOIN_ROUND  = 0x2000,
    FL_JOIN_BEVEL  = 0x3000
};

static inline void fl_line_style(int style, int width, const char *dashes) {
    fl_graphics_driver()->line_style(style, width, dashes);
}

static inline void fl_point(int x, int y) { fl_graphics_driver()->point(x, y); }
static inline void fl_line(int x, int y, int x1, int y1) { fl_graphics_driver()->line(x, y, x1, y1); }
static inline void fl_line3(int x, int y, int x1, int y1, int x2, int y2) {
    fl_graphics_driver()->line3(x, y, x1, y1, x2, y2);
}
static inline void fl_xyline(int x, int y, int x1) { fl_graphics_driver()->xyline(x, y, x1); }
static inline void fl_yxline(int x, int y, int y1) { fl_graphics_driver()->yxline(x, y, y1); }

static inline void fl_rect(int x, int y, int w, int h) { fl_graphics_driver()->rect(x, y, w, h); }
static inline void fl_rect_c(int x, int y, int w, int h, Fl_Color c) { fl_color(c); fl_rect(x, y, w, h); }
static inline void fl_rectf(int x, int y, int w, int h) { fl_graphics_driver()->rectf(x, y, w, h); }
static inline void fl_rectf_c(int x, int y, int w, int h, Fl_Color c) { fl_color(c); fl_rectf(x, y, w, h); }

static inline void fl_loop3(int x, int y, int x1, int y1, int x2, int y2) {
    fl_graphics_driver()->loop3(x, y, x1, y1, x2, y2);
}
static inline void fl_polygon3(int x, int y, int x1, int y1, int x2, int y2) {
    fl_graphics_driver()->polygon3(x, y, x1, y1, x2, y2);
}
/* a1/a2 in degrees, counterclockwise from 3 o'clock, matching upstream. */
static inline void fl_arc(int x, int y, int w, int h, double a1, double a2) {
    fl_graphics_driver()->arc(x, y, w, h, a1, a2);
}
static inline void fl_pie(int x, int y, int w, int h, double a1, double a2) {
    fl_graphics_driver()->pie(x, y, w, h, a1, a2);
}

/* ------------------------------------------------------------------ */
/* Fonts and text                                                      */
/* ------------------------------------------------------------------ */

static inline void fl_font(Fl_Font face, Fl_Fontsize size) { fl_graphics_driver()->font(face, size); }
static inline Fl_Font fl_font_current(void) { return fl_graphics_driver()->current_font(); }
static inline Fl_Fontsize fl_size(void) { return fl_graphics_driver()->current_size(); }
static inline int fl_height(void) { return fl_graphics_driver()->height(); }
static inline int fl_descent(void) { return fl_graphics_driver()->descent(); }
static inline double fl_width(const char *txt, int n) { return fl_graphics_driver()->width(txt, n); }
static inline double fl_width_str(const char *txt) { return fl_width(txt, txt ? (int)strlen(txt) : 0); }

static inline void fl_draw_text(const char *str, int n, int x, int y) { fl_graphics_driver()->draw_text(str, n, x, y); }
void fl_draw(const char *str, int x, int y);
void fl_measure(const char *str, int *w, int *h, int draw_symbols);

/* ------------------------------------------------------------------ */
/* Box drawing (fl_box_table[boxtype], same spirit as upstream)        */
/* ------------------------------------------------------------------ */

typedef void (Fl_Box_Draw_F)(int x, int y, int w, int h, Fl_Color c);

void fl_draw_box(uchar boxtype, int x, int y, int w, int h, Fl_Color c);
int fl_box_dx(uchar boxtype);
int fl_box_dy(uchar boxtype);
int fl_box_dw(uchar boxtype);
int fl_box_dh(uchar boxtype);
/** True for FL_..._FRAME box types, which draw a border only (no fill). */
int fl_box_is_frame(uchar boxtype);

Fl_Color fl_box_color(Fl_Color c);   /* Fl::box_color(): dims c if the box being drawn is inactive_r() */
void fl_set_box_color(Fl_Color c);   /* Fl::set_box_color(): fl_color(fl_box_color(c)) */
void fl_set_box_drawing_active(int active); /* Fl::draw_box_active() backing store */
int fl_box_drawing_active(void);

/* ------------------------------------------------------------------ */
/* Label drawing/measuring -- free-function replacement for the
 * protected Fl_Label::draw()/measure() methods. */
/* ------------------------------------------------------------------ */

void fl_label_draw(const Fl_Label *label, int x, int y, int w, int h, Fl_Align align);
void fl_label_measure(const Fl_Label *label, int *w, int *h);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_DRAW_H */
