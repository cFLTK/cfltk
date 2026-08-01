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

/* Parses an X11-style color spec ("#RGB", "#RRGGBB", "#RRRGGGBBB",
 * "#RRRRGGGGBBBB", or a bare hex string without the '#') into 8-bit
 * components. Returns non-zero on success, 0 if `name` isn't a
 * recognized spec (e.g. "None" -- callers should treat that as "no
 * color"/transparent, matching Fl_Pixmap's own XPM color table
 * convention). A small fixed table of common X11 color names (black,
 * white, red, green, blue, ...) is also recognized. Known difference
 * from upstream's fl_parse_color(): upstream calls XParseColor() on
 * X11, which resolves the full X11 rgb.txt name database (hundreds of
 * names); cfltk only recognizes hex specs plus this fixed common-name
 * table, since embedding the full rgb.txt database is out of
 * proportion to what any current client (XPM icon color tables) needs
 * -- virtually all real-world XPM files use hex colors anyway. */
int fl_parse_color(const char *name, uchar *r, uchar *g, uchar *b);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_COLORMAP_H */
