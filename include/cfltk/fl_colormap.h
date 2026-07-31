/*
 * cfltk - fl_colormap.h
 * The default 256-entry indexed color palette. See fl_colormap.c.
 */
#ifndef CFLTK_FL_COLORMAP_H
#define CFLTK_FL_COLORMAP_H

#include "cfltk/Enumerations.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const unsigned fl_colormap[256];

/* Resolves any Fl_Color (indexed 0-255 or a packed 24-bit direct color
 * from fl_rgb_color()) to concrete 8-bit components. */
void fl_get_color_rgb(Fl_Color c, uchar *r, uchar *g, uchar *b);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_COLORMAP_H */
