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
 *   - No offscreen/print surfaces yet -- out of scope for the Linux
 *     reference backend's first pass. See docs/DESIGN.md.
 *   - draw_image()/read_image() (added for Fl_RGB_Image) always blit
 *     straight to the current drawable -- there is no cached/offscreen
 *     image surface (upstream's per-image `id_`/`Fl_Offscreen` cache).
 *     Every Fl_RGB_Image::draw() call re-blits from its source array.
 *     See docs/DESIGN.md.
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

    /* Raw pixel blit/read-back, matching upstream's free functions
     * fl_draw_image()/fl_read_image() (themselves thin driver
     * forwarders). buf for draw_image is row-major top-to-bottom,
     * `d` bytes per pixel (1=gray, 3=RGB -- the only depths cfltk's
     * Fl_RGB_Image ever hands the driver directly; 2/4-channel
     * images with alpha are pre-composited to depth 3 by
     * Fl_RGB_Image itself before calling fl_draw_image(), the same
     * "manual composite, no accelerated alpha" path upstream's own
     * Xlib backend falls back to -- see docs/DESIGN.md), `ld` is the
     * byte stride between rows (0 means w*d). read_image fills buf
     * (already allocated by the caller) with `d`-byte-per-pixel data
     * (1 or 3) read back from the current drawable at (x,y,w,h); used
     * only for software alpha compositing (see docs/DESIGN.md), not
     * exposed as a general screenshot API yet. */
    void (*draw_image)(const unsigned char *buf, int x, int y, int w, int h, int d, int ld);
    void (*read_image)(unsigned char *buf, int x, int y, int w, int h, int d);

    /* Draws a 1-bit-per-pixel mask (Fl_Bitmap's XBM-style data: LSB
     * first, each row padded out to a whole byte -- (bmp_w+7)/8 bytes
     * per row where bmp_w/bmp_h is the FULL bitmap's size) in the
     * current color: set bits paint, clear bits are left untouched
     * (transparent), matching upstream's XSetStipple()/FillStippled
     * behavior. (x,y,w,h) is the on-screen rectangle to fill; (cx,cy)
     * is the offset into the bitmap of that rectangle's top-left
     * corner (for cropped/scrolled drawing), and bmp_w/bmp_h are the
     * full bitmap's own dimensions (needed to compute the stipple
     * origin/wrap). */
    void (*draw_bitmask)(const unsigned char *bits, int bmp_w, int bmp_h, int cx, int cy, int x, int y, int w, int h);

    /* Arbitrary-N-point fill/stroke, backing the portable vertex/matrix
     * drawing layer below (fl_begin_polygon()/fl_vertex()/...) --
     * unlike line3/loop3/polygon3 (hardcoded to exactly 3 points, used
     * by the older fl_line()/fl_loop()/fl_polygon() convenience calls
     * still used directly by some box types), these take a point count.
     * `closed` for draw_polyline: 0 = open polyline (XDrawLines-style),
     * 1 = closed loop (last point connects back to the first). */
    void (*fill_polygon)(const int *xs, const int *ys, int n);
    void (*draw_polyline)(const int *xs, const int *ys, int n, int closed);

    /* Real per-glyph ink-extent measurement (the actual painted pixel
     * bounding box, which can differ from the advance-width-based
     * width()/height() above - e.g. an italic "f" or a glyph with
     * overhang/underhang) in the current font, matching upstream's
     * fl_text_extents(). (*dx,*dy) is the ink box's top-left corner
     * relative to the text's pen-origin (usually small negative
     * numbers), (*w,*h) its size. Appended at the end of the vtable
     * (not inserted earlier) since every struct literal initializing
     * one of these is positional, not designated. */
    void (*text_extents)(const char *text, int n, int *dx, int *dy, int *w, int *h);
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

/* Registers (idempotently, by name - repeat calls with the same name
 * return the same slot) a system font family beyond the 12 builtin
 * FL_HELVETICA/FL_COURIER/FL_TIMES faces, for embedders that need to
 * select an arbitrary installed font by name (e.g. matching a CSS
 * font-family request) instead of being limited to the 3 generic
 * builtin families. Returns the "regular" Fl_Font for that family; the
 * bold/italic/bold-italic variants are `result | FL_BOLD`,
 * `result | FL_ITALIC`, `result | (FL_BOLD|FL_ITALIC)` respectively,
 * exactly like the builtin FL_HELVETICA/etc. slots (see
 * Enumerations.h). Matches upstream FLTK's own free-font-numbering
 * convention (FL_FREE_FONT). The actual font is resolved via
 * fontconfig/Xft at draw time, which already falls back gracefully to
 * a default if the named family isn't installed - so this never fails
 * and never needs a paired "does this font exist" query. */
Fl_Font Fl_set_font_family(const char *name);

/* Real fontconfig existence check for a family name: returns nonzero
 * only if fontconfig's own matching resolves `name` to itself (not a
 * generic substitution/fallback to some other installed family).
 * Unlike Fl_set_font_family() (which always "succeeds" by design, via
 * Xft/fontconfig's automatic substitution), this can genuinely say "no,
 * that font isn't installed" - useful for CSS font-family fallback
 * lists, where the caller needs to know whether to keep trying the next
 * candidate in the list. */
int Fl_font_family_exists(const char *name);
static inline Fl_Fontsize fl_size(void) { return fl_graphics_driver()->current_size(); }
static inline int fl_height(void) { return fl_graphics_driver()->height(); }
static inline int fl_descent(void) { return fl_graphics_driver()->descent(); }
static inline double fl_width(const char *txt, int n) { return fl_graphics_driver()->width(txt, n); }
static inline double fl_width_str(const char *txt) { return fl_width(txt, txt ? (int)strlen(txt) : 0); }

static inline void fl_draw_text(const char *str, int n, int x, int y) { fl_graphics_driver()->draw_text(str, n, x, y); }
void fl_draw(const char *str, int x, int y);
void fl_measure(const char *str, int *w, int *h, int draw_symbols);

/* Real per-glyph ink-extent measurement in the current font - see
 * Fl_Graphics_Driver.text_extents' own doc comment above for the
 * (dx,dy,w,h) semantics. Matches upstream's fl_text_extents(). */
static inline void fl_text_extents(const char *txt, int n, int *dx, int *dy, int *w, int *h) {
    fl_graphics_driver()->text_extents(txt, n, dx, dy, w, h);
}
static inline void fl_text_extents_str(const char *txt, int *dx, int *dy, int *w, int *h) {
    fl_text_extents(txt, txt ? (int)strlen(txt) : 0, dx, dy, w, h);
}

/* ------------------------------------------------------------------ */
/* Raw image blit/read-back (see Fl_Graphics_Driver::draw_image/       */
/* read_image above).                                                  */
/* ------------------------------------------------------------------ */

static inline void fl_draw_image(const unsigned char *buf, int x, int y, int w, int h, int d, int ld) {
    fl_graphics_driver()->draw_image(buf, x, y, w, h, d, ld);
}
/* Reads back w*h pixels of `d`-byte-per-pixel data (1 or 3) from the
 * current drawable at (x,y) into buf, which must be at least w*h*d
 * bytes. Unlike upstream's fl_read_image(), this never allocates --
 * cfltk has no client that needs the allocate-if-NULL convenience yet,
 * and the only caller so far (Fl_RGB_Image's software alpha compositor)
 * always has a buffer ready. */
static inline void fl_read_image(unsigned char *buf, int x, int y, int w, int h, int d) {
    fl_graphics_driver()->read_image(buf, x, y, w, h, d);
}
static inline void fl_draw_bitmask(const unsigned char *bits, int bmp_w, int bmp_h, int cx, int cy, int x, int y, int w, int h) {
    fl_graphics_driver()->draw_bitmask(bits, bmp_w, bmp_h, cx, cy, x, y, w, h);
}

/* ------------------------------------------------------------------ */
/* Portable vertex/matrix drawing (new; ported from src/fl_vertex.cxx  */
/* and the portable half of src/fl_arc.cxx). A small 2D affine         */
/* transform stack plus a begin/vertex/end path-building API, used by  */
/* fl_draw_symbol() (see fl_symbols.h) to draw the '@'-prefixed label   */
/* glyphs (arrows, etc.) at any position/size/rotation from a single   */
/* small set of vertices in a fixed -1..1 coordinate system. Portable  */
/* (backend-independent) code living in fl_draw.c, built on the new    */
/* fill_polygon/draw_polyline driver primitives above plus the         */
/* existing arc/pie ones.                                              */
/*                                                                      */
/* Known difference: fl_begin_complex_polygon()/fl_gap() do not        */
/* actually support multiple contours/holes -- end_complex_polygon()   */
/* fills the same way end_polygon() does. None of the symbols ported   */
/* in fl_symbols.c need a hole (upstream's own symbol set never calls  */
/* fl_gap() either), so this is unexercised rather than narrowed.      */
/* ------------------------------------------------------------------ */

void fl_push_matrix(void);
void fl_pop_matrix(void);
void fl_mult_matrix(double a, double b, double c, double d, double x, double y);
void fl_translate(double x, double y);
void fl_scale(double x, double y);
void fl_rotate(double d); /* degrees */

void fl_begin_points(void);
void fl_begin_line(void);
void fl_begin_loop(void);
void fl_begin_polygon(void);
void fl_begin_complex_polygon(void);
void fl_vertex(double x, double y);
void fl_circle(double x, double y, double r); /* adds to the current path if one is open, else draws standalone */
void fl_gap(void); /* no-op, see above */
void fl_end_points(void);
void fl_end_line(void);
void fl_end_loop(void);
void fl_end_polygon(void);
void fl_end_complex_polygon(void);

double fl_transform_x(double x, double y);
double fl_transform_y(double x, double y);

/* '@'-prefixed label glyphs (src/fl_symbols.cxx), built on the vertex/
 * matrix layer above. name excludes the leading '@'. Known difference:
 * "returnarrow" is not registered -- Fl_Return_Button already has its
 * own private, working arrow decoration (fl_return_arrow() static
 * helper in Fl_Return_Button.c); exposing it under this name too is
 * out of scope. See docs/DESIGN.md. */
int fl_add_symbol(const char *name, void (*drawit)(Fl_Color), int scalable);
int fl_draw_symbol(const char *label, int x, int y, int w, int h, Fl_Color col);

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

/* The raw draw function backing a boxtype - lets a caller registering a
 * new custom boxtype reuse an existing one's drawing function (e.g.
 * "same as FL_DOWN_BOX, just with different insets"), matching how
 * upstream's own Fl::set_boxtype() is commonly used. */
Fl_Box_Draw_F *fl_box_fn(uchar boxtype);

/* Registers a custom boxtype at slot `new_boxtype` (caller picks a
 * number >= FL_FREE_BOXTYPE, matching upstream's Fl::set_boxtype()
 * exactly) with the given draw function and insets. Once registered,
 * `new_boxtype` behaves exactly like any builtin boxtype through
 * fl_draw_box()/fl_box_dx()/etc. - no separate "is this custom"
 * distinction anywhere else in the API. */
void fl_set_boxtype(uchar new_boxtype, Fl_Box_Draw_F *fn, uchar dx, uchar dy, uchar dw, uchar dh);

Fl_Color fl_box_color(Fl_Color c);   /* Fl::box_color(): dims c if the box being drawn is inactive_r() */
void fl_set_box_color(Fl_Color c);   /* Fl::set_box_color(): fl_color(fl_box_color(c)) */
void fl_set_box_drawing_active(int active); /* Fl::draw_box_active() backing store */
int fl_box_drawing_active(void);

/* ------------------------------------------------------------------ */
/* Label drawing/measuring -- free-function replacement for the
 * protected Fl_Label::draw()/measure() methods. */
/* ------------------------------------------------------------------ */

/* When set, fl_label_draw()/fl_label_measure() interpret a single '&' in
 * the label as a mnemonic marker: the marker is removed from the
 * displayed text and the following character is underlined ("&&"
 * collapses to one literal '&'). Matches upstream's extern
 * fl_draw_shortcut; callers set it around one label draw/measure call
 * and clear it after (see Fl_Widget_draw_label_at() in Fl_Widget.c and
 * Fl_Menu_Item_draw()/_measure() in Fl_Menu_Item.c for the two places
 * that manage it). Known difference: the underlined character is
 * assumed to be a single byte (ASCII), consistent with the ASCII-only
 * shortcut limitation noted elsewhere (Fl_Widget.h, Fl_Button.h).
 */
extern int fl_draw_shortcut;

void fl_label_draw(const Fl_Label *label, int x, int y, int w, int h, Fl_Align align);
void fl_label_measure(const Fl_Label *label, int *w, int *h);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_DRAW_H */
