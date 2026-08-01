/*
 * cfltk - Fl_PNG_Image.h
 *
 * C translation of FLTK 1.3 FL/Fl_PNG_Image.H.
 *
 * Original class : Fl_PNG_Image : public Fl_RGB_Image -- loads a PNG
 *                   file (or an in-memory PNG buffer) via libpng,
 *                   handling palette expansion, 16-bit-to-8-bit strip,
 *                   sub-8-bit packing, and tRNS/alpha transparency.
 * New C structure : struct Fl_PNG_Image { Fl_RGB_Image rgb; },
 *                    embedding Fl_RGB_Image as its first member. No
 *                    fields or behavior of its own beyond the loader.
 * Build           : only compiled when the CFLTK_ENABLE_PNG build
 *                    option is on (default ON when libpng is found;
 *                    see CMakeLists.txt/Makefile) -- the "behind a
 *                    compile-time switch" plan from docs/DESIGN.md,
 *                    since a NuttX/embedded target may not want the
 *                    libpng dependency at all.
 * Known differences:
 *   - The in-memory-buffer constructor's upstream side effect of
 *     auto-registering the loaded image with Fl_Shared_Image (so it's
 *     found by name later) is not ported -- Fl_Shared_Image doesn't
 *     exist in cfltk yet (see docs/DESIGN.md's Next phases). The
 *     decode itself is fully ported; callers just don't get automatic
 *     registration.
 *   - On error, upstream calls Fl::warning(); cfltk has none yet, so
 *     this writes to stderr instead. Fl_Image_fail() still reports the
 *     correct error code either way.
 */
#ifndef CFLTK_FL_PNG_IMAGE_H
#define CFLTK_FL_PNG_IMAGE_H

#include "cfltk/Fl_RGB_Image.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Fl_PNG_Image {
    Fl_RGB_Image rgb;
} Fl_PNG_Image;

void Fl_PNG_Image_init(Fl_PNG_Image *self, const char *filename);
Fl_PNG_Image *Fl_PNG_Image_new(const char *filename);

void Fl_PNG_Image_init_from_memory(Fl_PNG_Image *self, const unsigned char *buffer, int datasize);
Fl_PNG_Image *Fl_PNG_Image_new_from_memory(const unsigned char *buffer, int datasize);

#ifdef __cplusplus
}
#endif

#endif
