/*
 * cfltk - Fl_Shared_Image.h
 *
 * C translation of FLTK 1.3 FL/Fl_Shared_Image.H.
 *
 * Original class : Fl_Shared_Image : public Fl_Image -- a reference-
 *                   counted, name-keyed cache of loaded images. Not a
 *                   pixel-data class itself: it wraps an owned
 *                   `Fl_Image *image_` and mirrors that image's w/h/
 *                   d/data() into its own Fl_Image fields (see
 *                   update()), so it can otherwise be used anywhere
 *                   an Fl_Image is expected (widget image() labels,
 *                   etc). get()/find() search a sorted array of every
 *                   currently-loaded shared image by (name, w, h);
 *                   release() decrements the refcount and frees the
 *                   record once it hits zero.
 * New C structure : struct Fl_Shared_Image { Fl_Image image; const
 *                    char *name; int original; int refcount; Fl_Image
 *                    *wrapped; int alloc_image; }, embedding Fl_Image
 *                    as its first member.
 * Vtbl            : fl_shared_image_ops (Fl_Image_Ops) -- draw/copy/
 *                    color_average/desaturate/uncache all delegate to
 *                    `wrapped` (falling back to Fl_Image's own
 *                    draw_empty() when unset, matching upstream).
 * Ownership       : the global image cache (a sorted, dynamically-
 *                    grown array of `Fl_Shared_Image*`, matching
 *                    upstream's own file-static `images_`/
 *                    `num_images_`/`alloc_images_`) owns every image
 *                    it holds; callers own only the reference they
 *                    got back from get()/find() and must release() it
 *                    exactly once when done, never call
 *                    Fl_Image_delete() on it directly (matching
 *                    upstream's protected destructor -- release() is
 *                    the only sanctioned teardown path).
 * Known differences:
 *   - `Fl_Shared_Image::scale()`/the deferred-resize-on-draw
 *     `scaled_image_` field (FLTK_ABI_VERSION >= 10304, an HiDPI/
 *     printing feature added in FLTK 1.3.4) is not ported. Draw always
 *     delegates straight to `wrapped` at its native size, matching
 *     upstream's own pre-1.3.4 fallback behavior.
 *   - `.xbm`/`.xpm` *text file* auto-detection in reload() (upstream
 *     dispatches these via Fl_XBM_Image/Fl_XPM_Image, neither of which
 *     is ported -- Fl_Bitmap/Fl_Pixmap already load from in-memory
 *     `char**`/XBM byte data, which covers the common compiled-in-icon
 *     case) is not ported. fl_register_images() below covers
 *     BMP/GIF/PNG/JPEG via magic-byte sniffing, matching upstream's
 *     own fl_images_core.cxx dispatch table minus Fl_PNM_Image (PNM/
 *     PPM/PGM/PBM, not ported -- no client needs it yet).
 *   - The PNG/JPEG-from-memory auto-registration friendship (upstream:
 *     Fl_PNG_Image/Fl_JPEG_Image's memory constructors directly
 *     construct and add() an Fl_Shared_Image) is not ported, matching
 *     the equivalent note already in Fl_PNG_Image.h/Fl_JPEG_Image.h.
 *     Call Fl_Shared_Image_get_from_rgb() to add an already-loaded
 *     image to the cache manually.
 */
#ifndef CFLTK_FL_SHARED_IMAGE_H
#define CFLTK_FL_SHARED_IMAGE_H

#include "cfltk/Fl_Image.h"
#include "cfltk/Fl_RGB_Image.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Fl_Shared_Image Fl_Shared_Image;

typedef Fl_Image *(*Fl_Shared_Handler)(const char *name, uchar *header, int headerlen);

struct Fl_Shared_Image {
    Fl_Image image;
    const char *name;
    int original;
    int refcount;
    Fl_Image *wrapped;
    int alloc_image;
};

extern const Fl_Image_Ops fl_shared_image_ops;

static inline const char *Fl_Shared_Image_name(const Fl_Shared_Image *self) { return self->name; }
static inline int Fl_Shared_Image_refcount(const Fl_Shared_Image *self) { return self->refcount; }
static inline int Fl_Shared_Image_original(const Fl_Shared_Image *self) { return self->original; }
static inline Fl_Image *Fl_Shared_Image_wrapped(const Fl_Shared_Image *self) { return self->wrapped; }

void Fl_Shared_Image_release(Fl_Shared_Image *self);
void Fl_Shared_Image_reload(Fl_Shared_Image *self);

static inline void Fl_Shared_Image_draw(Fl_Shared_Image *self, int X, int Y, int W, int H, int cx, int cy) {
    Fl_Image_draw(&self->image, X, Y, W, H, cx, cy);
}
static inline void Fl_Shared_Image_draw_at(Fl_Shared_Image *self, int X, int Y) { Fl_Image_draw_at(&self->image, X, Y); }

Fl_Shared_Image *Fl_Shared_Image_find(const char *name, int W, int H);
Fl_Shared_Image *Fl_Shared_Image_get(const char *name, int W, int H);
Fl_Shared_Image *Fl_Shared_Image_get_from_rgb(Fl_RGB_Image *rgb, int own_it);

Fl_Shared_Image **Fl_Shared_Image_images(void);
int Fl_Shared_Image_num_images(void);

void Fl_Shared_Image_add_handler(Fl_Shared_Handler f);
void Fl_Shared_Image_remove_handler(Fl_Shared_Handler f);

/* Registers the built-in BMP/GIF/PNG/JPEG format-sniffing handler
 * (matching upstream's fl_register_images(), normally provided by a
 * separate fltk_images library) so Fl_Shared_Image_get() can load
 * files of those formats. Safe to call more than once. */
void fl_register_images(void);

#ifdef __cplusplus
}
#endif

#endif
