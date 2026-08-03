/*
 * cfltk - fl_colormap.c
 *
 * The default 256-entry FLTK color palette, reproduced verbatim from
 * upstream's src/fl_cmap.h so that named colors (FL_RED, FL_BACKGROUND_COLOR,
 * the gray ramp, ...) resolve to the exact same RGB values FLTK 1.3
 * applications expect. Entries are packed as (r<<24 | g<<16 | b<<8), the
 * same layout fl_rgb_color() produces, so fl_get_color_rgb() below treats
 * indexed and direct RGB Fl_Color values uniformly.
 */
#include "cfltk/fl_colormap.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

const unsigned fl_colormap[256] = {
    0x00000000, 0xff000000, 0x00ff0000, 0xffff0000,
    0x0000ff00, 0xff00ff00, 0x00ffff00, 0xffffff00,
    0x55555500, 0xc6717100, 0x71c67100, 0x8e8e3800,
    0x7171c600, 0x8e388e00, 0x388e8e00, 0x00008000,
    0xa8a89800, 0xe8e8d800, 0x68685800, 0x98a8a800,
    0xd8e8e800, 0x58686800, 0x9c9ca800, 0xdcdce800,
    0x5c5c6800, 0x9ca89c00, 0xdce8dc00, 0x5c685c00,
    0x90909000, 0xc0c0c000, 0x50505000, 0xa0a0a000,
    0x00000000, 0x0d0d0d00, 0x1a1a1a00, 0x26262600,
    0x31313100, 0x3d3d3d00, 0x48484800, 0x55555500,
    0x5f5f5f00, 0x6a6a6a00, 0x75757500, 0x80808000,
    0x8a8a8a00, 0x95959500, 0xa0a0a000, 0xaaaaaa00,
    0xb5b5b500, 0xc0c0c000, 0xcbcbcb00, 0xd5d5d500,
    0xe0e0e000, 0xeaeaea00, 0xf5f5f500, 0xffffff00,
    0x00000000, 0x00240000, 0x00480000, 0x006d0000,
    0x00910000, 0x00b60000, 0x00da0000, 0x00ff0000,
    0x3f000000, 0x3f240000, 0x3f480000, 0x3f6d0000,
    0x3f910000, 0x3fb60000, 0x3fda0000, 0x3fff0000,
    0x7f000000, 0x7f240000, 0x7f480000, 0x7f6d0000,
    0x7f910000, 0x7fb60000, 0x7fda0000, 0x7fff0000,
    0xbf000000, 0xbf240000, 0xbf480000, 0xbf6d0000,
    0xbf910000, 0xbfb60000, 0xbfda0000, 0xbfff0000,
    0xff000000, 0xff240000, 0xff480000, 0xff6d0000,
    0xff910000, 0xffb60000, 0xffda0000, 0xffff0000,
    0x00003f00, 0x00243f00, 0x00483f00, 0x006d3f00,
    0x00913f00, 0x00b63f00, 0x00da3f00, 0x00ff3f00,
    0x3f003f00, 0x3f243f00, 0x3f483f00, 0x3f6d3f00,
    0x3f913f00, 0x3fb63f00, 0x3fda3f00, 0x3fff3f00,
    0x7f003f00, 0x7f243f00, 0x7f483f00, 0x7f6d3f00,
    0x7f913f00, 0x7fb63f00, 0x7fda3f00, 0x7fff3f00,
    0xbf003f00, 0xbf243f00, 0xbf483f00, 0xbf6d3f00,
    0xbf913f00, 0xbfb63f00, 0xbfda3f00, 0xbfff3f00,
    0xff003f00, 0xff243f00, 0xff483f00, 0xff6d3f00,
    0xff913f00, 0xffb63f00, 0xffda3f00, 0xffff3f00,
    0x00007f00, 0x00247f00, 0x00487f00, 0x006d7f00,
    0x00917f00, 0x00b67f00, 0x00da7f00, 0x00ff7f00,
    0x3f007f00, 0x3f247f00, 0x3f487f00, 0x3f6d7f00,
    0x3f917f00, 0x3fb67f00, 0x3fda7f00, 0x3fff7f00,
    0x7f007f00, 0x7f247f00, 0x7f487f00, 0x7f6d7f00,
    0x7f917f00, 0x7fb67f00, 0x7fda7f00, 0x7fff7f00,
    0xbf007f00, 0xbf247f00, 0xbf487f00, 0xbf6d7f00,
    0xbf917f00, 0xbfb67f00, 0xbfda7f00, 0xbfff7f00,
    0xff007f00, 0xff247f00, 0xff487f00, 0xff6d7f00,
    0xff917f00, 0xffb67f00, 0xffda7f00, 0xffff7f00,
    0x0000bf00, 0x0024bf00, 0x0048bf00, 0x006dbf00,
    0x0091bf00, 0x00b6bf00, 0x00dabf00, 0x00ffbf00,
    0x3f00bf00, 0x3f24bf00, 0x3f48bf00, 0x3f6dbf00,
    0x3f91bf00, 0x3fb6bf00, 0x3fdabf00, 0x3fffbf00,
    0x7f00bf00, 0x7f24bf00, 0x7f48bf00, 0x7f6dbf00,
    0x7f91bf00, 0x7fb6bf00, 0x7fdabf00, 0x7fffbf00,
    0xbf00bf00, 0xbf24bf00, 0xbf48bf00, 0xbf6dbf00,
    0xbf91bf00, 0xbfb6bf00, 0xbfdabf00, 0xbfffbf00,
    0xff00bf00, 0xff24bf00, 0xff48bf00, 0xff6dbf00,
    0xff91bf00, 0xffb6bf00, 0xffdabf00, 0xffffbf00,
    0x0000ff00, 0x0024ff00, 0x0048ff00, 0x006dff00,
    0x0091ff00, 0x00b6ff00, 0x00daff00, 0x00ffff00,
    0x3f00ff00, 0x3f24ff00, 0x3f48ff00, 0x3f6dff00,
    0x3f91ff00, 0x3fb6ff00, 0x3fdaff00, 0x3fffff00,
    0x7f00ff00, 0x7f24ff00, 0x7f48ff00, 0x7f6dff00,
    0x7f91ff00, 0x7fb6ff00, 0x7fdaff00, 0x7fffff00,
    0xbf00ff00, 0xbf24ff00, 0xbf48ff00, 0xbf6dff00,
    0xbf91ff00, 0xbfb6ff00, 0xbfdaff00, 0xbfffff00,
    0xff00ff00, 0xff24ff00, 0xff48ff00, 0xff6dff00,
    0xff91ff00, 0xffb6ff00, 0xffdaff00, 0xffffff00
};

/* Runtime overrides for indexed colors 0-255, matching upstream's
 * Fl::set_color()/get_color()/free_color() - fl_colormap[] itself
 * stays const (a compiled-in default table, reproduced verbatim from
 * upstream), this is a sparse override layer checked first. Index i
 * is overridden iff g_color_override_set[i] is nonzero; the actual
 * value lives in g_color_override[i], same packed (r<<24|g<<16|b<<8)
 * layout as fl_colormap[] so fl_get_color_rgb() needs no branch for
 * "is this overridden" beyond the one lookup. */
static unsigned g_color_override[256];
static unsigned char g_color_override_set[256];

void Fl_set_color(Fl_Color c, uchar r, uchar g, uchar b) {
    if (c > 255) return; /* direct RGB colors (fl_rgb_color()) aren't indexed - nothing to override */
    g_color_override[c] = ((unsigned)r << 24) | ((unsigned)g << 16) | ((unsigned)b << 8);
    g_color_override_set[c] = 1;
}

void Fl_free_color(Fl_Color c) {
    if (c > 255) return;
    g_color_override_set[c] = 0;
}

void fl_get_color_rgb(Fl_Color c, uchar *r, uchar *g, uchar *b) {
    unsigned packed;
    if (c <= 255 && g_color_override_set[c]) {
        packed = g_color_override[c];
    } else {
        packed = (c <= 255) ? fl_colormap[c] : c;
    }
    *r = (uchar)(packed >> 24);
    *g = (uchar)(packed >> 16);
    *b = (uchar)(packed >> 8);
}

static const struct { const char *name; uchar r, g, b; } named_colors[] = {
    { "black", 0, 0, 0 }, { "white", 255, 255, 255 },
    { "red", 255, 0, 0 }, { "green", 0, 255, 0 }, { "blue", 0, 0, 255 },
    { "yellow", 255, 255, 0 }, { "cyan", 0, 255, 255 }, { "magenta", 255, 0, 255 },
    { "gray", 190, 190, 190 }, { "grey", 190, 190, 190 },
    { "orange", 255, 165, 0 }, { "purple", 160, 32, 240 },
    { "brown", 165, 42, 42 }, { "pink", 255, 192, 203 }, { "navy", 0, 0, 128 },
};

int fl_parse_color(const char *name, uchar *r, uchar *g, uchar *b) {
    const char *p = name;
    size_t n, m, i;
    const char *pattern;
    int rr, gg, bb;

    if (!p || !*p) return 0;
    if (*p == '#') p++;

    n = strlen(p);
    m = n / 3;
    if (n && n % 3 == 0 && m >= 1 && m <= 4) {
        int all_hex = 1;
        for (i = 0; i < n; i++) if (!isxdigit((unsigned char)p[i])) { all_hex = 0; break; }
        if (all_hex) {
            switch (m) {
                case 1: pattern = "%1x%1x%1x"; break;
                case 2: pattern = "%2x%2x%2x"; break;
                case 3: pattern = "%3x%3x%3x"; break;
                default: pattern = "%4x%4x%4x"; break;
            }
            if (sscanf(p, pattern, &rr, &gg, &bb) == 3) {
                switch (m) {
                    case 1: rr *= 0x11; gg *= 0x11; bb *= 0x11; break;
                    case 3: rr >>= 4; gg >>= 4; bb >>= 4; break;
                    case 4: rr >>= 8; gg >>= 8; bb >>= 8; break;
                    default: break;
                }
                *r = (uchar)rr; *g = (uchar)gg; *b = (uchar)bb;
                return 1;
            }
        }
    }

    for (i = 0; i < sizeof(named_colors) / sizeof(named_colors[0]); i++) {
        if (strcasecmp(name, named_colors[i].name) == 0) {
            *r = named_colors[i].r; *g = named_colors[i].g; *b = named_colors[i].b;
            return 1;
        }
    }
    return 0;
}
