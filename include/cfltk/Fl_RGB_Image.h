/*
 * cfltk - Fl_RGB_Image.h
 *
 * C translation of the Fl_RGB_Image half of FLTK 1.3 FL/Fl_Image.H
 * (see Fl_Image.h for the Fl_Image base-class half and the
 * class-conversion notes shared by both).
 *
 * Original class : Fl_RGB_Image : public Fl_Image -- full-color image
 *                   with 1 (gray), 2 (gray+alpha), 3 (RGB), or 4
 *                   (RGBA) bytes per pixel.
 * New C structure : struct Fl_RGB_Image { Fl_Image image; const uchar
 *                    *array; int alloc_array; }, embedding Fl_Image as
 *                    its first member. No `id_`/`mask_` cache fields --
 *                    see Known differences.
 * Vtbl            : fl_rgb_image_ops (Fl_Image_Ops).
 * Known differences:
 *   - No cached/offscreen drawing surface (upstream's `id_`
 *     Fl_Offscreen + `mask_` bitmap, rebuilt lazily on first draw and
 *     reused after). Every draw() call here re-converts and re-blits
 *     directly from `array` via fl_draw_image()/fl_read_image() (see
 *     fl_draw.h's known differences) -- correctness-identical, just
 *     without upstream's per-image caching optimization. uncache() is
 *     consequently a no-op (nothing to free).
 *   - 2- and 4-channel (alpha-bearing) images are composited in
 *     software against the pixels currently on screen at draw time,
 *     the same "manual composite, no accelerated alpha" algorithm
 *     upstream's own Xlib backend falls back to when
 *     fl_can_do_alpha_blending() is false -- cfltk always takes that
 *     path since it has no accelerated-alpha offscreen surface.
 *   - Bilinear scaling (FL_RGB_SCALING_BILINEAR) IS ported in full
 *     (see copy()); this is a correctness feature of the base class,
 *     not related to the caching simplification above.
 */
#ifndef CFLTK_FL_RGB_IMAGE_H
#define CFLTK_FL_RGB_IMAGE_H

#include "cfltk/Fl_Image.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Fl_RGB_Image {
    Fl_Image image;
    const uchar *array;
    int alloc_array; /* non-zero: `array` is owned (malloc'd) and freed on destroy */
} Fl_RGB_Image;

extern const Fl_Image_Ops fl_rgb_image_ops;

/* bits must remain valid as long as the image is used unless the
 * caller sets alloc_array non-zero afterward (matching upstream: the
 * constructor itself always sets alloc_array=0). */
void Fl_RGB_Image_init(Fl_RGB_Image *self, const uchar *bits, int W, int H, int D, int LD);
Fl_RGB_Image *Fl_RGB_Image_new(const uchar *bits, int W, int H, int D, int LD);

static inline int Fl_RGB_Image_w(const Fl_RGB_Image *self) { return self->image.w; }
static inline int Fl_RGB_Image_h(const Fl_RGB_Image *self) { return self->image.h; }
static inline int Fl_RGB_Image_d(const Fl_RGB_Image *self) { return self->image.d; }
static inline const uchar *Fl_RGB_Image_array(const Fl_RGB_Image *self) { return self->array; }
static inline int Fl_RGB_Image_alloc_array(const Fl_RGB_Image *self) { return self->alloc_array; }
static inline void Fl_RGB_Image_set_alloc_array(Fl_RGB_Image *self, int v) { self->alloc_array = v; }

static inline void Fl_RGB_Image_draw(Fl_RGB_Image *self, int X, int Y, int W, int H, int cx, int cy) {
    Fl_Image_draw(&self->image, X, Y, W, H, cx, cy);
}
static inline void Fl_RGB_Image_draw_at(Fl_RGB_Image *self, int X, int Y) {
    Fl_Image_draw_at(&self->image, X, Y);
}
static inline Fl_Image *Fl_RGB_Image_copy(const Fl_RGB_Image *self, int W, int H) {
    return Fl_Image_copy_sized(&self->image, W, H);
}
static inline void Fl_RGB_Image_color_average(Fl_RGB_Image *self, Fl_Color c, float i) {
    Fl_Image_color_average(&self->image, c, i);
}
static inline void Fl_RGB_Image_desaturate(Fl_RGB_Image *self) { Fl_Image_desaturate(&self->image); }

void Fl_RGB_Image_max_size(size_t size);
size_t Fl_RGB_Image_get_max_size(void);

#ifdef __cplusplus
}
#endif

#endif
