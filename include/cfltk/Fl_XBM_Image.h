/*
 * cfltk - Fl_XBM_Image.h
 *
 * C translation of FLTK 1.3 FL/Fl_XBM_Image.H / src/Fl_XBM_Image.cxx.
 *
 * Original class : Fl_XBM_Image : public Fl_Bitmap (constructor-only:
 *                   parses a .xbm file (a C header: "#define w_width W",
 *                   "#define w_height H", then a static byte array)
 *                   from disk into the raw bit array Fl_Bitmap's own
 *                   constructor takes directly).
 * New C structure : none of its own; reuses struct Fl_Bitmap.
 * Ownership       : the parsed bit array is always owned by the
 *                   returned Fl_Bitmap (alloc_array set), freed
 *                   automatically on destroy -- unlike a plain
 *                   Fl_Bitmap_new(bits,w,h), whose bits are caller-owned.
 * Known differences:
 *   - On a malformed/unreadable file, returns a Fl_Bitmap with no data
 *     (w()==h()==0) rather than NULL, same reasoning as Fl_XPM_Image.h.
 */
#ifndef CFLTK_FL_XBM_IMAGE_H
#define CFLTK_FL_XBM_IMAGE_H

#include "cfltk/Fl_Bitmap.h"

#ifdef __cplusplus
extern "C" {
#endif

Fl_Bitmap *Fl_XBM_Image_new(const char *filename);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_XBM_IMAGE_H */
