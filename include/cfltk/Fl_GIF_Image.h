/*
 * cfltk - Fl_GIF_Image.h
 *
 * C translation of FLTK 1.3 FL/Fl_GIF_Image.H.
 *
 * Original class : Fl_GIF_Image : public Fl_Pixmap -- loads the first
 *                   frame of a Compuserve GIF file (self-contained LZW
 *                   decoder, no external library) into an Fl_Pixmap's
 *                   compressed-colormap data format (the same
 *                   `ncolors<0` FLTK-specific XPM dialect
 *                   Fl_Pixmap_color_average()/desaturate() already
 *                   understand), preserving GIF's single transparent
 *                   color index if present.
 * New C structure : struct Fl_GIF_Image { Fl_Pixmap pixmap; },
 *                    embedding Fl_Pixmap as its first member. No
 *                    fields or behavior of its own beyond the loader
 *                    -- drawing/copying/color_average()/desaturate()
 *                    are all inherited unchanged via fl_pixmap_ops.
 * Known differences:
 *   - Only the first image/frame is loaded (matches upstream: "loads
 *     the first image", no animation support).
 *   - On error, upstream calls Fl::error()/Fl::warning(); cfltk has
 *     neither yet, so this writes to stderr instead. Fl_Image_fail()
 *     still reports the correct error code either way.
 */
#ifndef CFLTK_FL_GIF_IMAGE_H
#define CFLTK_FL_GIF_IMAGE_H

#include "cfltk/Fl_Pixmap.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Fl_GIF_Image {
    Fl_Pixmap pixmap;
} Fl_GIF_Image;

void Fl_GIF_Image_init(Fl_GIF_Image *self, const char *filename);
Fl_GIF_Image *Fl_GIF_Image_new(const char *filename);

#ifdef __cplusplus
}
#endif

#endif
