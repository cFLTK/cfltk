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
 * from fl_rgb_color()) to concrete 8-bit components - checks the
 * runtime override table below first, then falls back to
 * fl_colormap[]. */
void fl_get_color_rgb(Fl_Color c, uchar *r, uchar *g, uchar *b);

/* Runtime-remaps an indexed color 0-255 to a new RGB value, matching
 * upstream's Fl::set_color(Fl_Color, uchar, uchar, uchar) - e.g.
 * changing FL_GRAY/FL_BACKGROUND_COLOR to restyle every widget using
 * it. No-op for c > 255 (a direct RGB color, not an index - nothing
 * to remap). Takes effect on the next redraw; does not retroactively
 * repaint anything itself. */
void Fl_set_color(Fl_Color c, uchar r, uchar g, uchar b);

/* Reverts index c back to fl_colormap[]'s compiled-in default,
 * matching upstream's Fl::free_color(). */
void Fl_free_color(Fl_Color c);

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
