/*
 * cfltk - Fl_Pixmap.h
 *
 * C translation of FLTK 1.3 FL/Fl_Pixmap.H (+ the XPM parsing engine
 * from src/fl_draw_pixmap.cxx that upstream's Fl_Pixmap::draw() and
 * Fl_RGB_Image(const Fl_Pixmap*,bg) both depend on).
 *
 * Original class : Fl_Pixmap : public Fl_Image -- caches and draws
 *                   colormap (XPM-style) images, including binary
 *                   ("None" color) transparency.
 * New C structure : struct Fl_Pixmap { Fl_Image image; int alloc_data; }.
 *                    image.data/image.count hold the XPM char** lines
 *                    exactly as upstream's Fl_Image::data()/count() do
 *                    for a pixmap.
 * Vtbl            : fl_pixmap_ops (Fl_Image_Ops).
 * Known differences:
 *   - No cached server-side Pixmap/mask (upstream's `id_`/`mask_`):
 *     draw() converts the XPM color table to an RGBA buffer (via
 *     fl_convert_pixmap(), ported from fl_draw_pixmap.cxx) and blits
 *     it fresh every call -- correctness-identical, no caching.
 *     uncache() is a no-op.
 *   - fl_convert_pixmap()'s per-pixel alpha is always exactly 0 or 255
 *     (XPM's "None" color is the only transparency FLTK's own XPM
 *     dialect supports), so compositing against the screen is a hard
 *     mask (fully replace or fully keep), not a soft blend -- see
 *     Fl_Pixmap.c.
 *   - fl_convert_pixmap()'s "c word" color-context lookup uses
 *     fl_parse_color() (see fl_colormap.h), which recognizes hex specs
 *     and a small fixed name table, not the full X11 rgb.txt database.
 */
#ifndef CFLTK_FL_PIXMAP_H
#define CFLTK_FL_PIXMAP_H

#include "cfltk/Fl_Image.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Fl_Pixmap {
    Fl_Image image;
    int alloc_data;
} Fl_Pixmap;

extern const Fl_Image_Ops fl_pixmap_ops;

/* D must remain valid as long as the image is used unless alloc_data
 * is set (matching upstream: the constructor always sets it to 0). */
void Fl_Pixmap_init(Fl_Pixmap *self, const char *const *D);
Fl_Pixmap *Fl_Pixmap_new(const char *const *D);

static inline int Fl_Pixmap_w(const Fl_Pixmap *self) { return self->image.w; }
static inline int Fl_Pixmap_h(const Fl_Pixmap *self) { return self->image.h; }
static inline const char *const *Fl_Pixmap_data(const Fl_Pixmap *self) { return self->image.data; }

static inline void Fl_Pixmap_draw(Fl_Pixmap *self, int X, int Y, int W, int H, int cx, int cy) {
    Fl_Image_draw(&self->image, X, Y, W, H, cx, cy);
}
static inline void Fl_Pixmap_draw_at(Fl_Pixmap *self, int X, int Y) { Fl_Image_draw_at(&self->image, X, Y); }
static inline Fl_Image *Fl_Pixmap_copy(const Fl_Pixmap *self, int W, int H) { return Fl_Image_copy_sized(&self->image, W, H); }
static inline void Fl_Pixmap_color_average(Fl_Pixmap *self, Fl_Color c, float i) { Fl_Image_color_average(&self->image, c, i); }
static inline void Fl_Pixmap_desaturate(Fl_Pixmap *self) { Fl_Image_desaturate(&self->image); }

/* Ported from fl_draw_pixmap.cxx: parses just the "W H NCOLORS CPP"
 * header line of XPM data. Returns non-zero and fills the W/H outputs
 * on success. */
int fl_measure_pixmap(const char *const *cdata, int *W, int *H);

/* Ported from fl_draw_pixmap.cxx: decodes XPM data (color table +
 * pixel grid) into a WxHx4 (RGBA) buffer `out` (caller-allocated,
 * W*H*4 bytes), using `bg` for the color of fully transparent ("None")
 * pixels (their alpha byte is still 0, so callers compositing against
 * the screen -- see Fl_Pixmap_draw()'s known differences -- get the
 * expected hard transparency; `bg`'s RGB only matters if the buffer
 * is used without alpha, e.g. to build an Fl_RGB_Image). Returns
 * non-zero on success. */
int fl_convert_pixmap(const char *const *cdata, uchar *out, Fl_Color bg);

#ifdef __cplusplus
}
#endif

#endif
