/*
 * cfltk - Fl_Bitmap.h
 *
 * C translation of FLTK 1.3 FL/Fl_Bitmap.H.
 *
 * Original class : Fl_Bitmap : public Fl_Image -- a 1-bit-per-pixel
 *                   mask image (XBM-style data), drawn in the widget's
 *                   current color (set bits paint, clear bits are
 *                   transparent).
 * New C structure : struct Fl_Bitmap { Fl_Image image; const uchar
 *                    *array; int alloc_array; }.
 * Vtbl            : fl_bitmap_ops (Fl_Image_Ops).
 * Known differences:
 *   - No cached server-side Pixmap (upstream's `id_`): every draw()
 *     builds and immediately frees a throwaway X Pixmap via
 *     fl_draw_bitmask() (see fl_draw.h) -- correctness-identical,
 *     without the caching optimization. uncache() is a no-op.
 */
#ifndef CFLTK_FL_BITMAP_H
#define CFLTK_FL_BITMAP_H

#include "cfltk/Fl_Image.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Fl_Bitmap {
    Fl_Image image;
    const uchar *array;
    int alloc_array;
} Fl_Bitmap;

extern const Fl_Image_Ops fl_bitmap_ops;

void Fl_Bitmap_init(Fl_Bitmap *self, const uchar *bits, int W, int H);
Fl_Bitmap *Fl_Bitmap_new(const uchar *bits, int W, int H);

static inline int Fl_Bitmap_w(const Fl_Bitmap *self) { return self->image.w; }
static inline int Fl_Bitmap_h(const Fl_Bitmap *self) { return self->image.h; }
static inline const uchar *Fl_Bitmap_array(const Fl_Bitmap *self) { return self->array; }

static inline void Fl_Bitmap_draw(Fl_Bitmap *self, int X, int Y, int W, int H, int cx, int cy) {
    Fl_Image_draw(&self->image, X, Y, W, H, cx, cy);
}
static inline void Fl_Bitmap_draw_at(Fl_Bitmap *self, int X, int Y) { Fl_Image_draw_at(&self->image, X, Y); }
static inline Fl_Image *Fl_Bitmap_copy(const Fl_Bitmap *self, int W, int H) { return Fl_Image_copy_sized(&self->image, W, H); }

#ifdef __cplusplus
}
#endif

#endif
