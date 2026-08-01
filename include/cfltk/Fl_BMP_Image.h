/*
 * cfltk - Fl_BMP_Image.h
 *
 * C translation of FLTK 1.3 FL/Fl_BMP_Image.H.
 *
 * Original class : Fl_BMP_Image : public Fl_RGB_Image -- loads a
 *                   Windows Bitmap (.bmp) file into an Fl_RGB_Image.
 *                   Self-contained decoder (1/4/8/16/24/32-bit,
 *                   RLE4/RLE8 compression, optional alpha mask), no
 *                   external library dependency.
 * New C structure : struct Fl_BMP_Image { Fl_RGB_Image rgb; },
 *                    embedding Fl_RGB_Image as its first member. No
 *                    fields or behavior of its own beyond the loader
 *                    -- it reuses fl_rgb_image_ops verbatim (drawing,
 *                    copying, color_average()/desaturate() are all
 *                    inherited unchanged, exactly like upstream).
 * Known differences:
 *   - On error, upstream calls Fl::warning() before returning a
 *     zero-size failed image; cfltk has no Fl::warning() yet, so this
 *     writes to stderr instead. Fl_Image_fail() still reports the
 *     correct ERR_FILE_ACCESS/ERR_FORMAT code either way.
 */
#ifndef CFLTK_FL_BMP_IMAGE_H
#define CFLTK_FL_BMP_IMAGE_H

#include "cfltk/Fl_RGB_Image.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Fl_BMP_Image {
    Fl_RGB_Image rgb;
} Fl_BMP_Image;

void Fl_BMP_Image_init(Fl_BMP_Image *self, const char *filename);
Fl_BMP_Image *Fl_BMP_Image_new(const char *filename);

#ifdef __cplusplus
}
#endif

#endif
