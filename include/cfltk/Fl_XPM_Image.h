/*
 * cfltk - Fl_XPM_Image.h
 *
 * C translation of FLTK 1.3 FL/Fl_XPM_Image.H / src/Fl_XPM_Image.cxx.
 *
 * Original class : Fl_XPM_Image : public Fl_Pixmap (constructor-only:
 *                   parses a .xpm file from disk into the same C-string-
 *                   array representation Fl_Pixmap's own constructor
 *                   takes directly, then hands it to Fl_Pixmap the same
 *                   way).
 * New C structure : none of its own; reuses struct Fl_Pixmap (same
 *                   "subclass reuses parent struct" pattern as
 *                   Fl_Hold_Browser reusing struct Fl_Browser).
 * Ownership       : unlike a plain Fl_Pixmap_new(D) (D is caller-owned
 *                   unless alloc_data is set), the parsed line array is
 *                   always owned by the returned Fl_Pixmap (alloc_data
 *                   set), freed automatically on destroy.
 * Known differences:
 *   - On a malformed/unreadable file, returns a Fl_Pixmap with no data
 *     (w()==-1, matching upstream's own "constructor returns early,
 *     leaving Fl_Image's default null state" behavior) rather than
 *     NULL -- callers already need to handle a null-image Fl_Pixmap
 *     from the plain constructor, so this doesn't add a new case.
 */
#ifndef CFLTK_FL_XPM_IMAGE_H
#define CFLTK_FL_XPM_IMAGE_H

#include "cfltk/Fl_Pixmap.h"

#ifdef __cplusplus
extern "C" {
#endif

Fl_Pixmap *Fl_XPM_Image_new(const char *filename);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_XPM_IMAGE_H */
