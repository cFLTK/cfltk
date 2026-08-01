/*
 * cfltk - Fl_Image.h
 *
 * C translation of FLTK 1.3 FL/Fl_Image.H (base class portion; the
 * Fl_RGB_Image half of that same upstream header lives in
 * Fl_RGB_Image.h so each cfltk header maps to one class).
 *
 * Original class : Fl_Image -- abstract base class for cached, drawable
 *                   images (bitmaps, pixmaps, RGB(A) images, and every
 *                   file-format loader that derives from those).
 * New C structure : struct Fl_Image { const Fl_Image_Ops *ops; int w,
 *                    h, d, ld, count; const char *const *data; }. No
 *                    inheritance chain to embed -- every concrete image
 *                    type (Fl_RGB_Image, Fl_Pixmap, Fl_Bitmap, and any
 *                    future loader) embeds this struct as its first
 *                    member, exactly like Fl_Widget subclasses embed
 *                    Fl_Widget.
 * Vtbl            : Fl_Image_Ops -- draw/copy/color_average/desaturate/
 *                    uncache/destroy, one per concrete image type,
 *                    mirroring Fl_WidgetOps's role for widgets. The
 *                    base fl_image_ops instance implements upstream's
 *                    base-class bodies (draw() -> draw_empty(), copy()
 *                    returns a same-size empty Fl_Image, color_average/
 *                    desaturate/uncache are no-ops).
 * Ownership       : an Fl_Image never owns a widget; a widget's
 *                    label.image/deimage are plain unowned pointers
 *                    (see Fl_Widget_set_image() in Fl_Widget.h) --
 *                    matching upstream, where nothing ref-counts or
 *                    deletes an image on a widget's behalf (that is
 *                    what Fl_Shared_Image's caching layer is for, not
 *                    yet ported -- see docs/DESIGN.md).
 * Known differences:
 *   - No Fl_Menu_Item image-label support (upstream's
 *     Fl_Image::label(Fl_Menu_Item*), wired through Fl::set_labeltype()'s
 *     pluggable labeltype-callback registry). cfltk's menu items draw
 *     plain text labels only; the registry Fl_Image::labeltype()/
 *     measure() would need to plug into doesn't exist yet. Image labels
 *     on ordinary Fl_Widgets (the far more common case) work fully via
 *     Fl_Widget_set_image()/fl_label_draw().
 *   - No cached/offscreen drawing surface (see fl_draw.h's known
 *     differences) -- every draw() re-blits from the source pixel
 *     array; there is no per-image `id_`/`Fl_Offscreen` cache field.
 */
#ifndef CFLTK_FL_IMAGE_H
#define CFLTK_FL_IMAGE_H

#include "cfltk/Fl_Widget.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FL_IMAGE_ERR_NO_IMAGE    (-1)
#define FL_IMAGE_ERR_FILE_ACCESS (-2)
#define FL_IMAGE_ERR_FORMAT      (-3)

/* Fl_RGB_Scaling */
#define FL_RGB_SCALING_NEAREST  0
#define FL_RGB_SCALING_BILINEAR 1

typedef struct Fl_Image_Ops {
    void (*draw)(Fl_Image *self, int X, int Y, int W, int H, int cx, int cy);
    Fl_Image *(*copy)(const Fl_Image *self, int W, int H);
    void (*color_average)(Fl_Image *self, Fl_Color c, float i);
    void (*desaturate)(Fl_Image *self);
    void (*uncache)(Fl_Image *self);
    /* Frees any resources the concrete type owns (e.g. Fl_RGB_Image's
     * pixel array if alloc_array is set). Never frees `self` itself --
     * matches the destroy()-then-free() split used for Fl_Widget. */
    void (*destroy)(Fl_Image *self);
} Fl_Image_Ops;

struct Fl_Image {
    const Fl_Image_Ops *ops;
    int w, h, d, ld, count;
    const char *const *data;
};

extern const Fl_Image_Ops fl_image_ops;

void Fl_Image_init(Fl_Image *self, int W, int H, int D);
Fl_Image *Fl_Image_new(int W, int H, int D);
void Fl_Image_destroy(Fl_Image *self);
void Fl_Image_delete(Fl_Image *self);

static inline int Fl_Image_w(const Fl_Image *self) { return self->w; }
static inline int Fl_Image_h(const Fl_Image *self) { return self->h; }
static inline int Fl_Image_d(const Fl_Image *self) { return self->d; }
static inline int Fl_Image_ld(const Fl_Image *self) { return self->ld; }
static inline int Fl_Image_count(const Fl_Image *self) { return self->count; }
static inline const char *const *Fl_Image_data(const Fl_Image *self) { return self->data; }

int Fl_Image_fail(const Fl_Image *self);

static inline void Fl_Image_draw(Fl_Image *self, int X, int Y, int W, int H, int cx, int cy) {
    self->ops->draw(self, X, Y, W, H, cx, cy);
}
static inline void Fl_Image_draw_at(Fl_Image *self, int X, int Y) { Fl_Image_draw(self, X, Y, self->w, self->h, 0, 0); }

static inline Fl_Image *Fl_Image_copy_sized(const Fl_Image *self, int W, int H) { return self->ops->copy(self, W, H); }
static inline Fl_Image *Fl_Image_copy(const Fl_Image *self) { return Fl_Image_copy_sized(self, self->w, self->h); }

static inline void Fl_Image_color_average(Fl_Image *self, Fl_Color c, float i) { self->ops->color_average(self, c, i); }
static inline void Fl_Image_inactive(Fl_Image *self) { Fl_Image_color_average(self, FL_GRAY, 0.33f); }
static inline void Fl_Image_desaturate(Fl_Image *self) { self->ops->desaturate(self); }
static inline void Fl_Image_uncache(Fl_Image *self) { self->ops->uncache(self); }

/* Upstream's Fl_Image::label(Fl_Widget*): sets widget->image(this). */
static inline void Fl_Image_set_as_label(Fl_Image *self, Fl_Widget *w) { Fl_Widget_set_image(w, self); }

void Fl_Image_draw_empty(const Fl_Image *self, int X, int Y);

void Fl_Image_set_RGB_scaling(int method);
int Fl_Image_RGB_scaling(void);

#ifdef __cplusplus
}
#endif

#endif
