/*
 * cfltk - Fl_JPEG_Image.h
 *
 * C translation of FLTK 1.3 FL/Fl_JPEG_Image.H.
 *
 * Original class : Fl_JPEG_Image : public Fl_RGB_Image -- loads a
 *                   JPEG file (or an in-memory JPEG buffer) via
 *                   libjpeg, always decoded to 3-channel RGB.
 * New C structure : struct Fl_JPEG_Image { Fl_RGB_Image rgb; },
 *                    embedding Fl_RGB_Image as its first member. No
 *                    fields or behavior of its own beyond the loader.
 * Build           : only compiled when the CFLTK_ENABLE_JPEG build
 *                    option is on (default ON when libjpeg is found;
 *                    see CMakeLists.txt/Makefile), matching
 *                    Fl_PNG_Image's compile-time switch.
 * Known differences:
 *   - The in-memory constructor takes an explicit `datasize` upstream
 *     doesn't have: upstream's hand-rolled `jpeg_mem_src()` always
 *     claims 4096 bytes are available per read with no bounds check
 *     against the buffer's real size, silently reading past the end
 *     on any input under ~4KB or not block-aligned. This translation
 *     uses libjpeg's own standard, bounds-safe `jpeg_mem_src(cinfo,
 *     buffer, size)` (present in libjpeg-turbo and all IJG releases
 *     >= 8) instead of reimplementing the unsafe version, matching
 *     this project's practice of fixing real bugs found while
 *     translating (see docs/DESIGN.md's Fl_Text_Editor compose() fix
 *     and Fl_GIF_Image's EOF-check fix for precedent).
 *   - Auto-registration with Fl_Shared_Image is not ported (doesn't
 *     exist in cfltk yet, see docs/DESIGN.md's Next phases).
 *   - On error, upstream calls Fl::warning(); cfltk has none yet, so
 *     this writes to stderr instead.
 */
#ifndef CFLTK_FL_JPEG_IMAGE_H
#define CFLTK_FL_JPEG_IMAGE_H

#include "cfltk/Fl_RGB_Image.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Fl_JPEG_Image {
    Fl_RGB_Image rgb;
} Fl_JPEG_Image;

void Fl_JPEG_Image_init(Fl_JPEG_Image *self, const char *filename);
Fl_JPEG_Image *Fl_JPEG_Image_new(const char *filename);

void Fl_JPEG_Image_init_from_memory(Fl_JPEG_Image *self, const unsigned char *data, unsigned long datasize);
Fl_JPEG_Image *Fl_JPEG_Image_new_from_memory(const unsigned char *data, unsigned long datasize);

#ifdef __cplusplus
}
#endif

#endif
