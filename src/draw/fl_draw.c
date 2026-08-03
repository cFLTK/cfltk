/*
 * cfltk - fl_draw.c
 *
 * Backend-independent half of FL/fl_draw.H: color math, box drawing
 * (the fl_box_table dispatch, translated from src/fl_boxtype.cxx), and
 * label drawing/measuring (translated from the protected Fl_Label
 * methods in src/Fl_Widget.cxx). Everything that actually touches a
 * display -- fl_graphics_driver itself -- is installed by the platform
 * backend (src/backend/x11/fl_x11_driver.c).
 */
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cfltk/fl_draw.h"
#include "cfltk/fl_colormap.h"
#include "cfltk/Fl_Image.h"
#include "../backend/fl_backend.h"

static const Fl_Graphics_Driver *g_driver = NULL;

void fl_set_graphics_driver(const Fl_Graphics_Driver *driver) { g_driver = driver; }

/* Lazily opens the display/installs the driver on first use, exactly
 * like upstream's fl_open_display() being called implicitly by font
 * measurement -- this is what lets client code measure text (e.g.
 * Fl_Browser building its item list) before any window is shown, a
 * very common FLTK usage pattern. Previously this only happened as a
 * side effect of Fl_Widget_show() on a window, which crashed any code
 * that measured text first (fl_graphics_driver() returning NULL). */
const Fl_Graphics_Driver *fl_graphics_driver(void) {
    if (!g_driver) fl_backend_init();
    return g_driver;
}

/* ------------------------------------------------------------------ */
/* Color math (src/fl_color.cxx)                                       */
/* ------------------------------------------------------------------ */

Fl_Color fl_color_average(Fl_Color color1, Fl_Color color2, float weight) {
    unsigned rgb1, rgb2;
    uchar r, g, b;

    rgb1 = (color1 & 0xffffff00u) ? color1 : fl_colormap[color1 & 255];
    rgb2 = (color2 & 0xffffff00u) ? color2 : fl_colormap[color2 & 255];

    r = (uchar)(((uchar)(rgb1 >> 24)) * weight + ((uchar)(rgb2 >> 24)) * (1 - weight));
    g = (uchar)(((uchar)(rgb1 >> 16)) * weight + ((uchar)(rgb2 >> 16)) * (1 - weight));
    b = (uchar)(((uchar)(rgb1 >> 8)) * weight + ((uchar)(rgb2 >> 8)) * (1 - weight));

    return fl_rgb_color(r, g, b);
}

Fl_Color fl_inactive(Fl_Color c) { return fl_color_average(c, FL_GRAY, 0.33f); }

Fl_Color fl_contrast(Fl_Color fg, Fl_Color bg) {
    unsigned c1, c2;
    int l1, l2;

    c1 = (fg & 0xffffff00u) ? fg : fl_colormap[fg];
    c2 = (bg & 0xffffff00u) ? bg : fl_colormap[bg];

    l1 = ((c1 >> 24) * 30 + ((c1 >> 16) & 255) * 59 + ((c1 >> 8) & 255) * 11) / 100;
    l2 = ((c2 >> 24) * 30 + ((c2 >> 16) & 255) * 59 + ((c2 >> 8) & 255) * 11) / 100;

    if ((l1 - l2) > 99) return fg;
    else if ((l2 - l1) > 99) return fg;
    else if (l2 > 127) return FL_BLACK;
    else return FL_WHITE;
}

/* ------------------------------------------------------------------ */
/* Text                                                                 */
/* ------------------------------------------------------------------ */

void fl_draw(const char *str, int x, int y) {
    if (str) fl_draw_text(str, (int)strlen(str), x, y);
}

/* ------------------------------------------------------------------ */
/* Portable vertex/matrix drawing (src/fl_vertex.cxx)                  */
/*                                                                      */
/* All state below is file-static rather than per-driver-instance: */
/* cfltk's Fl_Graphics_Driver is a stateless vtable (the backend        */
/* installs one shared const struct), so the matrix stack and path      */
/* point buffer -- both genuinely mutable, per-call state in upstream's */
/* Fl_Graphics_Driver object -- live here instead, in the portable      */
/* (backend-independent) layer that owns this API. Not thread-safe,     */
/* matching upstream (FLTK drawing is single-threaded by design).       */
/* ------------------------------------------------------------------ */

#ifndef CFLTK_PI
#define CFLTK_PI 3.14159265358979323846
#endif

typedef struct { double a, b, c, d, x, y; } Fl_Matrix;

#define MATRIX_STACK_SIZE 32
static Fl_Matrix s_matrix_stack[MATRIX_STACK_SIZE];
static int s_matrix_sp = 0;
static Fl_Matrix s_matrix = { 1, 0, 0, 1, 0, 0 };

enum { V_POINT, V_LINE, V_LOOP, V_POLYGON };
static int s_what = V_POINT;
static int s_gap = 0;

static int *s_px = NULL, *s_py = NULL;
static int s_pn = 0, s_psize = 0;

void fl_push_matrix(void) {
    if (s_matrix_sp == MATRIX_STACK_SIZE) {
        fprintf(stderr, "cfltk: fl_push_matrix(): matrix stack overflow\n");
    } else {
        s_matrix_stack[s_matrix_sp++] = s_matrix;
    }
}

void fl_pop_matrix(void) {
    if (s_matrix_sp == 0) {
        fprintf(stderr, "cfltk: fl_pop_matrix(): matrix stack underflow\n");
    } else {
        s_matrix = s_matrix_stack[--s_matrix_sp];
    }
}

void fl_mult_matrix(double a, double b, double c, double d, double x, double y) {
    Fl_Matrix o;
    o.a = a * s_matrix.a + b * s_matrix.c;
    o.b = a * s_matrix.b + b * s_matrix.d;
    o.c = c * s_matrix.a + d * s_matrix.c;
    o.d = c * s_matrix.b + d * s_matrix.d;
    o.x = x * s_matrix.a + y * s_matrix.c + s_matrix.x;
    o.y = x * s_matrix.b + y * s_matrix.d + s_matrix.y;
    s_matrix = o;
}

void fl_translate(double x, double y) { fl_mult_matrix(1, 0, 0, 1, x, y); }
void fl_scale(double x, double y) { fl_mult_matrix(x, 0, 0, y, 0, 0); }

void fl_rotate(double d) {
    double s, c;
    if (!d) return;
    if (d == 90) { s = 1; c = 0; }
    else if (d == 180) { s = 0; c = -1; }
    else if (d == 270 || d == -90) { s = -1; c = 0; }
    else { s = sin(d * CFLTK_PI / 180.0); c = cos(d * CFLTK_PI / 180.0); }
    fl_mult_matrix(c, -s, s, c, 0, 0);
}

double fl_transform_x(double x, double y) { return x * s_matrix.a + y * s_matrix.c + s_matrix.x; }
double fl_transform_y(double x, double y) { return x * s_matrix.b + y * s_matrix.d + s_matrix.y; }

/* Appends an already-transformed, already-rounded point, skipping if
 * it duplicates the last one (matches upstream's transformed_vertex0,
 * which keeps XFillPolygon/XDrawLines from choking on zero-length
 * segments). */
static void push_point(int x, int y) {
    if (s_pn > 0 && x == s_px[s_pn - 1] && y == s_py[s_pn - 1]) return;
    if (s_pn >= s_psize) {
        s_psize = s_px ? s_psize * 2 : 16;
        s_px = (int *)realloc(s_px, (size_t)s_psize * sizeof(int));
        s_py = (int *)realloc(s_py, (size_t)s_psize * sizeof(int));
    }
    s_px[s_pn] = x;
    s_py[s_pn] = y;
    s_pn++;
}

static int rnd(double v) { return (int)floor(v + 0.5); }

void fl_begin_points(void) { s_pn = 0; s_what = V_POINT; }
void fl_begin_line(void) { s_pn = 0; s_what = V_LINE; }
void fl_begin_loop(void) { s_pn = 0; s_what = V_LOOP; }
void fl_begin_polygon(void) { s_pn = 0; s_what = V_POLYGON; }
void fl_begin_complex_polygon(void) { s_pn = 0; s_what = V_POLYGON; s_gap = 0; }

void fl_vertex(double x, double y) {
    push_point(rnd(x * s_matrix.a + y * s_matrix.c + s_matrix.x),
               rnd(x * s_matrix.b + y * s_matrix.d + s_matrix.y));
}

void fl_gap(void) {
    while (s_pn > s_gap + 2 && s_px[s_pn - 1] == s_px[s_gap] && s_py[s_pn - 1] == s_py[s_gap]) s_pn--;
    if (s_pn > s_gap + 2) {
        push_point(s_px[s_gap], s_py[s_gap]);
        s_gap = s_pn;
    } else {
        s_pn = s_gap;
    }
}

/* Removes trailing points that duplicate the first, so a caller that
 * already closed the path manually (last vertex == first vertex)
 * doesn't get a degenerate zero-length closing segment. */
static void fixloop(void) {
    while (s_pn > 2 && s_px[s_pn - 1] == s_px[0] && s_py[s_pn - 1] == s_py[0]) s_pn--;
}

void fl_end_points(void) {
    int i;
    for (i = 0; i < s_pn; i++) fl_graphics_driver()->point(s_px[i], s_py[i]);
}

void fl_end_line(void) {
    if (s_pn < 2) { fl_end_points(); return; }
    fl_graphics_driver()->draw_polyline(s_px, s_py, s_pn, 0);
}

void fl_end_loop(void) {
    fixloop();
    if (s_pn > 2) push_point(s_px[0], s_py[0]);
    fl_end_line();
}

void fl_end_polygon(void) {
    fixloop();
    if (s_pn < 3) { fl_end_line(); return; }
    fl_graphics_driver()->fill_polygon(s_px, s_py, s_pn);
}

void fl_end_complex_polygon(void) {
    fl_gap();
    if (s_pn < 3) { fl_end_line(); return; }
    fl_graphics_driver()->fill_polygon(s_px, s_py, s_pn);
}

/* Shortcuts closed circles to an arc/pie call (matching upstream);
 * does not draw rotated ellipses correctly under a skewed matrix --
 * same documented upstream limitation. Fills if called between
 * fl_begin_polygon()/fl_end_polygon() (or _complex_polygon), strokes
 * otherwise -- tracked via the same s_what left over from the last
 * begin/end pair, exactly mirroring upstream's use of its `what`
 * member as ambient state for standalone fl_circle() calls. */
void fl_circle(double x, double y, double r) {
    double xt = fl_transform_x(x, y);
    double yt = fl_transform_y(x, y);
    double rx = r * (s_matrix.c != 0 ? sqrt(s_matrix.a * s_matrix.a + s_matrix.c * s_matrix.c) : fabs(s_matrix.a));
    double ry = r * (s_matrix.b != 0 ? sqrt(s_matrix.b * s_matrix.b + s_matrix.d * s_matrix.d) : fabs(s_matrix.d));
    int llx = rnd(xt - rx);
    int w = rnd(xt + rx) - llx;
    int lly = rnd(yt - ry);
    int h = rnd(yt + ry) - lly;
    if (s_what == V_POLYGON) {
        fl_pie(llx, lly, w, h, 0, 360);
    } else {
        fl_arc(llx, lly, w, h, 0, 360);
    }
}

/* ------------------------------------------------------------------ */
/* Box drawing (src/fl_boxtype.cxx)                                    */
/* ------------------------------------------------------------------ */

#define D1 2 /* BORDER_WIDTH, matches upstream's configure default of 2 */
#define D2 (D1 + D1)

static int g_box_drawing_active = 1;

void fl_set_box_drawing_active(int active) { g_box_drawing_active = active; }
int fl_box_drawing_active(void) { return g_box_drawing_active; }

Fl_Color fl_box_color(Fl_Color c) { return g_box_drawing_active ? c : fl_inactive(c); }
void fl_set_box_color(Fl_Color c) { fl_color(fl_box_color(c)); }

/* 'A'..'X' -> one of 24 standard grayscale steps, dimmed when the box
 * being drawn is inactive. Mirrors fl_gray_ramp()/active_ramp/inactive_ramp. */
static const uchar k_active_ramp[24] = {
    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
    44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55
};
static const uchar k_inactive_ramp[24] = {
    43, 43, 44, 44, 44, 45, 45, 46, 46, 46, 47, 47,
    48, 48, 48, 49, 49, 49, 50, 50, 51, 51, 52, 52
};

static void fl_ramp_frame(const char *s, int x, int y, int w, int h) {
    const uchar *g = (g_box_drawing_active ? k_active_ramp : k_inactive_ramp) - 'A';
    if (h <= 0 || w <= 0) return;
    for (; *s;) {
        fl_color(g[(int)*s++]);
        fl_xyline(x, y, x + w - 1);
        y++; if (--h <= 0) break;
        fl_color(g[(int)*s++]);
        fl_yxline(x, y + h - 1, y);
        x++; if (--w <= 0) break;
        fl_color(g[(int)*s++]);
        fl_xyline(x, y + h - 1, x + w - 1);
        if (--h <= 0) break;
        fl_color(g[(int)*s++]);
        fl_yxline(x + w - 1, y + h - 1, y);
        if (--w <= 0) break;
    }
}

static void fl_ramp_frame2(const char *s, int x, int y, int w, int h) {
    const uchar *g = (g_box_drawing_active ? k_active_ramp : k_inactive_ramp) - 'A';
    if (h <= 0 || w <= 0) return;
    for (; *s;) {
        fl_color(g[(int)*s++]);
        fl_xyline(x, y + h - 1, x + w - 1);
        if (--h <= 0) break;
        fl_color(g[(int)*s++]);
        fl_yxline(x + w - 1, y + h - 1, y);
        if (--w <= 0) break;
        fl_color(g[(int)*s++]);
        fl_xyline(x, y, x + w - 1);
        y++; if (--h <= 0) break;
        fl_color(g[(int)*s++]);
        fl_yxline(x, y + h - 1, y);
        x++; if (--w <= 0) break;
    }
}

static void fl_no_box(int x, int y, int w, int h, Fl_Color c) { (void)x; (void)y; (void)w; (void)h; (void)c; }

static void fl_flat_box(int x, int y, int w, int h, Fl_Color c) { fl_rectf_c(x, y, w, h, fl_box_color(c)); }

static void fl_thin_up_frame(int x, int y, int w, int h, Fl_Color c) { (void)c; fl_ramp_frame2("HHWW", x, y, w, h); }
static void fl_thin_up_box(int x, int y, int w, int h, Fl_Color c) {
    fl_thin_up_frame(x, y, w, h, c);
    fl_set_box_color(c);
    fl_rectf(x + 1, y + 1, w - 2, h - 2);
}
static void fl_thin_down_frame(int x, int y, int w, int h, Fl_Color c) { (void)c; fl_ramp_frame2("WWHH", x, y, w, h); }
static void fl_thin_down_box(int x, int y, int w, int h, Fl_Color c) {
    fl_thin_down_frame(x, y, w, h, c);
    fl_set_box_color(c);
    fl_rectf(x + 1, y + 1, w - 2, h - 2);
}

static void fl_up_frame(int x, int y, int w, int h, Fl_Color c) { (void)c; fl_ramp_frame2("AAWWMMTT", x, y, w, h); }
static void fl_up_box(int x, int y, int w, int h, Fl_Color c) {
    fl_up_frame(x, y, w, h, c);
    fl_set_box_color(c);
    fl_rectf(x + D1, y + D1, w - D2, h - D2);
}
static void fl_down_frame(int x, int y, int w, int h, Fl_Color c) { (void)c; fl_ramp_frame2("WWMMPPAA", x, y, w, h); }
static void fl_down_box(int x, int y, int w, int h, Fl_Color c) {
    fl_down_frame(x, y, w, h, c);
    fl_set_box_color(c);
    fl_rectf(x + D1, y + D1, w - D2, h - D2);
}

static void fl_engraved_frame(int x, int y, int w, int h, Fl_Color c) { (void)c; fl_ramp_frame("HHWWWWHH", x, y, w, h); }
static void fl_engraved_box(int x, int y, int w, int h, Fl_Color c) {
    fl_engraved_frame(x, y, w, h, c);
    fl_set_box_color(c);
    fl_rectf(x + 2, y + 2, w - 4, h - 4);
}
static void fl_embossed_frame(int x, int y, int w, int h, Fl_Color c) { (void)c; fl_ramp_frame("WWHHHHWW", x, y, w, h); }
static void fl_embossed_box(int x, int y, int w, int h, Fl_Color c) {
    fl_embossed_frame(x, y, w, h, c);
    fl_set_box_color(c);
    fl_rectf(x + 2, y + 2, w - 4, h - 4);
}

static void fl_border_frame(int x, int y, int w, int h, Fl_Color c) {
    fl_set_box_color(c);
    fl_rect(x, y, w, h);
}
static void fl_border_box(int x, int y, int w, int h, Fl_Color c) {
    fl_set_box_color(FL_BLACK);
    fl_rect(x, y, w, h);
    fl_set_box_color(c);
    fl_rectf(x + 1, y + 1, w - 2, h - 2);
}

typedef struct Fl_Box_Table_Entry {
    Fl_Box_Draw_F *fn;
    uchar dx, dy, dw, dh;
    int is_frame;
} Fl_Box_Table_Entry;

/* Indexed exactly like enum Fl_Boxtype in Enumerations.h. Entries beyond
 * FL_BORDER_FRAME fall back to fl_flat_box/fl_border_box in fl_draw_box()
 * until their dedicated box-type files are ported (see docs/DESIGN.md);
 * upstream keeps those in separate translation units for the same reason
 * cfltk will: don't force every embedded build to link rounded/plastic/
 * gtk/gleam box art it will never use. */
static const Fl_Box_Table_Entry k_box_table[] = {
    { fl_no_box,          0, 0, 0,  0,  0 }, /* FL_NO_BOX */
    { fl_flat_box,        0, 0, 0,  0,  0 }, /* FL_FLAT_BOX */
    { fl_up_box,         D1,D1,D2, D2, 0 },  /* FL_UP_BOX */
    { fl_down_box,       D1,D1,D2, D2, 0 },  /* FL_DOWN_BOX */
    { fl_up_frame,       D1,D1,D2, D2, 1 },  /* FL_UP_FRAME */
    { fl_down_frame,     D1,D1,D2, D2, 1 },  /* FL_DOWN_FRAME */
    { fl_thin_up_box,     1, 1, 2,  2,  0 }, /* FL_THIN_UP_BOX */
    { fl_thin_down_box,   1, 1, 2,  2,  0 }, /* FL_THIN_DOWN_BOX */
    { fl_thin_up_frame,   1, 1, 2,  2,  1 }, /* FL_THIN_UP_FRAME */
    { fl_thin_down_frame, 1, 1, 2,  2,  1 }, /* FL_THIN_DOWN_FRAME */
    { fl_engraved_box,    2, 2, 4,  4,  0 }, /* FL_ENGRAVED_BOX */
    { fl_embossed_box,    2, 2, 4,  4,  0 }, /* FL_EMBOSSED_BOX */
    { fl_engraved_frame,  2, 2, 4,  4,  1 }, /* FL_ENGRAVED_FRAME */
    { fl_embossed_frame,  2, 2, 4,  4,  1 }, /* FL_EMBOSSED_FRAME */
    { fl_border_box,      1, 1, 2,  2,  0 }, /* FL_BORDER_BOX */
    { fl_border_box,      1, 1, 2,  2,  0 }, /* FL_SHADOW_BOX (falls back) */
    { fl_border_frame,    1, 1, 2,  2,  1 }, /* FL_BORDER_FRAME */
    { fl_border_frame,    1, 1, 2,  2,  1 }, /* FL_SHADOW_FRAME (falls back) */
};
#define K_BOX_TABLE_COUNT (int)(sizeof(k_box_table) / sizeof(k_box_table[0]))

/* Custom boxtypes registered via fl_set_boxtype(), indexed by
 * (boxtype - FL_FREE_BOXTYPE). Matches upstream's own free-boxtype-
 * numbering convention (FL_FREE_BOXTYPE, see Enumerations.h). */
#define CUSTOM_BOXTYPES_MAX 32
static Fl_Box_Table_Entry g_custom_boxtypes[CUSTOM_BOXTYPES_MAX];
static int g_custom_boxtype_hi = -1; /* highest slot actually registered, -1 = none */

void fl_set_boxtype(uchar new_boxtype, Fl_Box_Draw_F *fn, uchar dx, uchar dy, uchar dw, uchar dh) {
    int slot = new_boxtype - FL_FREE_BOXTYPE;
    if (slot < 0 || slot >= CUSTOM_BOXTYPES_MAX) return; /* out of range: silently ignored, same as an invalid upstream call */
    g_custom_boxtypes[slot].fn = fn;
    g_custom_boxtypes[slot].dx = dx;
    g_custom_boxtypes[slot].dy = dy;
    g_custom_boxtypes[slot].dw = dw;
    g_custom_boxtypes[slot].dh = dh;
    g_custom_boxtypes[slot].is_frame = 0;
    if (slot > g_custom_boxtype_hi) g_custom_boxtype_hi = slot;
}

static const Fl_Box_Table_Entry *box_entry(uchar boxtype) {
    if (boxtype < K_BOX_TABLE_COUNT) return &k_box_table[boxtype];
    if (boxtype >= FL_FREE_BOXTYPE) {
        int slot = boxtype - FL_FREE_BOXTYPE;
        if (slot <= g_custom_boxtype_hi && g_custom_boxtypes[slot].fn) return &g_custom_boxtypes[slot];
    }
    return &k_box_table[FL_UP_BOX]; /* safe, visible fallback for not-yet-ported/unregistered types */
}

void fl_draw_box(uchar boxtype, int x, int y, int w, int h, Fl_Color c) {
    box_entry(boxtype)->fn(x, y, w, h, c);
}
int fl_box_dx(uchar boxtype) { return box_entry(boxtype)->dx; }
int fl_box_dy(uchar boxtype) { return box_entry(boxtype)->dy; }
int fl_box_dw(uchar boxtype) { return box_entry(boxtype)->dw; }
int fl_box_dh(uchar boxtype) { return box_entry(boxtype)->dh; }
int fl_box_is_frame(uchar boxtype) { return box_entry(boxtype)->is_frame; }
Fl_Box_Draw_F *fl_box_fn(uchar boxtype) { return box_entry(boxtype)->fn; }

/* ------------------------------------------------------------------ */
/* Label drawing/measuring (protected Fl_Label::draw()/measure() in
 * src/Fl_Widget.cxx, reduced to the FL_NORMAL_LABEL text+image case --
 * the shadow/engraved/embossed label types are follow-up work, see
 * docs/DESIGN.md). */
/* ------------------------------------------------------------------ */

int fl_draw_shortcut = 0;

/* Strips a single '&' mnemonic marker from label text into out (capped
 * to outcap-1 bytes): "&&" collapses to one literal '&'; a lone '&'
 * before any other character is removed and that character's byte
 * offset in `out` is returned via *underline (-1 if no marker, or if
 * fl_draw_shortcut is unset -- text is then copied through unchanged). */
static int strip_shortcut_marker(const char *in, char *out, int outcap, int *underline) {
    int oi = 0;
    *underline = -1;
    if (!in) { out[0] = '\0'; return 0; }
    if (!fl_draw_shortcut) {
        int n = (int)strlen(in);
        if (n > outcap - 1) n = outcap - 1;
        memcpy(out, in, (size_t)n);
        out[n] = '\0';
        return n;
    }
    while (*in && oi < outcap - 1) {
        if (*in == '&') {
            if (in[1] == '&') { out[oi++] = '&'; in += 2; continue; }
            in++;
            if (*in && *underline < 0) *underline = oi;
            continue;
        }
        out[oi++] = *in++;
    }
    out[oi] = '\0';
    return oi;
}

/* Splits a leading and/or trailing '@symbol' off `buf` (already past
 * strip_shortcut_marker()), matching the two cases upstream's fl_draw()
 * detects (src/fl_draw.cxx): a label starting with "@name " (symbol
 * name terminated by whitespace, which is consumed) and/or a label
 * ending in "@name" (found via the last '@' at least 2 bytes in, so a
 * leading symbol's own '@' is never mistaken for a trailing one).
 * "@@" is never treated as a symbol start (escape, matching upstream),
 * though -- unlike upstream's full multi-line engine -- a literal "@@"
 * elsewhere in running text is not collapsed to one '@' here (out of
 * scope for this single-line label engine; see docs/DESIGN.md).
 * *text_out points into buf (possibly past a consumed leading symbol,
 * and NUL-truncated before a trailing one). Adjusts *underline (from
 * strip_shortcut_marker) if it fell inside consumed/removed text. */
/* When set, extract_symbols() below never recognizes a '@symbol' at
 * all (the whole string is left as plain text) - analogous to
 * fl_draw_shortcut's existing control over '&' mnemonic handling.
 * Meant to be toggled around a fl_label_draw_default()/
 * fl_label_measure_default() call from inside a custom labeltype
 * handler registered via Fl_set_labeltype() that wants the normal
 * layout/rendering but with untrusted label text never triggering
 * '@'-symbol-glyph substitution (the actual real-world need this was
 * added for - see Fl_set_labeltype()'s own doc comment). */
int fl_label_no_symbols = 0;

static void extract_symbols(char *buf, char **text_out, char *sym0, size_t sym0cap,
                             char *sym1, size_t sym1cap, int *underline) {
    char *p = buf;
    sym0[0] = '\0';
    sym1[0] = '\0';
    if (fl_label_no_symbols) { *text_out = buf; return; }
    if (p[0] == '@' && p[1] && p[1] != '@') {
        char *s = sym0;
        char *q = p;
        while (*q && !isspace((unsigned char)*q) && (size_t)(s - sym0) < sym0cap - 1) *s++ = *q++;
        *s = '\0';
        if (isspace((unsigned char)*q)) q++;
        if (*underline >= 0) {
            int consumed = (int)(q - p);
            if (*underline >= consumed) *underline -= consumed;
            else *underline = -1;
        }
        p = q;
    }
    {
        char *p2 = strrchr(p, '@');
        if (p2 && p2 > p + 1 && p2[-1] != '@') {
            size_t n = sym1cap - 1;
            size_t avail = strlen(p2);
            if (n > avail) n = avail;
            memcpy(sym1, p2, n);
            sym1[n] = '\0';
            if (*underline >= 0 && (p + *underline) >= p2) *underline = -1;
            *p2 = '\0';
        }
    }
    *text_out = p;
}

/* Raw text metrics in the current font (caller must fl_font() first,
 * matching upstream) -- no mnemonic/'&' handling (that's specific to
 * widget label drawing, see fl_label_draw()/fl_label_measure()), just
 * '\n'-separated line splitting plus the same leading/trailing
 * '@symbol' detection as fl_label_draw() when draw_symbols is set.
 * Used directly by common-dialog layout code (src/dialogs/fl_ask.c). */
void fl_measure(const char *str, int *w, int *h, int draw_symbols) {
    int nlines = 0, maxw = 0, symw0 = 0, symw1 = 0, lh;
    const char *p, *nl, *body;
    char sym0[64] = "", sym1[64] = "";
    char buf[1024];
    int dummy_underline = -1;

    if (!str || !*str) { *w = 0; *h = 0; return; }

    lh = fl_height();
    body = str;

    if (draw_symbols) {
        char *text_ptr;
        size_t n = strlen(str);
        if (n > sizeof(buf) - 1) n = sizeof(buf) - 1;
        memcpy(buf, str, n);
        buf[n] = '\0';
        extract_symbols(buf, &text_ptr, sym0, sizeof(sym0), sym1, sizeof(sym1), &dummy_underline);
        if (sym0[0]) symw0 = lh;
        if (sym1[0]) symw1 = lh;
        body = text_ptr;
    }

    for (p = body; p; ) {
        int len;
        nl = strchr(p, '\n');
        len = nl ? (int)(nl - p) : (int)strlen(p);
        { int lw = (int)fl_width(p, len); if (lw > maxw) maxw = lw; }
        nlines++;
        p = nl ? nl + 1 : NULL;
    }

    *w = maxw + symw0 + symw1;
    *h = nlines * lh;
}

void fl_label_measure_default(const Fl_Label *label, int *w, int *h) {
    char buf[512], sym0[64], sym1[64], *text;
    int underline = -1;
    int symw0 = 0, symw1 = 0, lw = 0, lh = 0;
    *w = 0;
    *h = 0;
    if (label->value && label->value[0]) {
        fl_font(label->font, label->size);
        strip_shortcut_marker(label->value, buf, (int)sizeof(buf), &underline);
        extract_symbols(buf, &text, sym0, sizeof(sym0), sym1, sizeof(sym1), &underline);
        if (sym0[0]) symw0 = fl_height();
        if (sym1[0]) symw1 = fl_height();
        if (text[0]) { lw = (int)fl_width_str(text); lh = fl_height(); }
        else if (symw0 || symw1) lh = fl_height();
        *w = symw0 + lw + symw1;
        *h = lh;
    }
    if (label->image) {
        int iw = Fl_Image_w(label->image), ih = Fl_Image_h(label->image);
        if (iw > *w) *w = iw;
        *h += ih;
    }
}

/* Combined text+image label drawing, scoped down from upstream's
 * fl_draw(str,x,y,w,h,align,img,draw_symbols) (see docs/DESIGN.md):
 * stacks the image above or below the text depending on
 * FL_ALIGN_TEXT_OVER_IMAGE, or draws image-only / text-only when the
 * other is absent -- covering the overwhelming majority of real
 * widget labels (icon-only toolbar buttons, and text-with-icon
 * buttons/menu items). A leading and/or trailing '@symbol' in the text
 * is drawn as a glyph (fl_draw_symbol(), see extract_symbols() above)
 * sized to the font's line height, laid out immediately beside the
 * remaining text with no gap, matching upstream's single-line spacing.
 * Known differences: no FL_ALIGN_IMAGE_NEXT_TO_TEXT side-by-side
 * image+text layout, no multi-line wrap (pre-existing text-only
 * limitation, unchanged) -- so a symbol is only ever recognized at the
 * very start/end of the whole (single-line) label, not per-line. */
void fl_label_draw_default(const Fl_Label *label, int x, int y, int w, int h, Fl_Align align) {
    int lw = 0, lh = 0, lx = 0, ly, underline = -1, dn = 0, baseline;
    int imgw = 0, imgh = 0, total_h, top;
    int symw0 = 0, symw1 = 0;
    char buf[512], sym0[64], sym1[64], *text = buf;
    int has_text = label->value && label->value[0] ? 1 : 0;
    Fl_Image *img = (align & FL_ALIGN_IMAGE_BACKDROP) ? NULL : label->image;
    int text_over_image = (align & FL_ALIGN_TEXT_OVER_IMAGE) ? 1 : 0;

    if (label->type == FL_NO_LABEL) return;
    if (!has_text && !img) return;

    fl_font(label->font, label->size);
    fl_color(label->color);

    buf[0] = '\0';
    if (has_text) {
        strip_shortcut_marker(label->value, buf, (int)sizeof(buf), &underline);
        extract_symbols(buf, &text, sym0, sizeof(sym0), sym1, sizeof(sym1), &underline);
        dn = (int)strlen(text);
        if (sym0[0]) symw0 = fl_height();
        if (sym1[0]) symw1 = fl_height();
        has_text = text[0] ? 1 : 0;
        if (has_text) { lw = (int)fl_width_str(text); lh = fl_height(); }
        else if (symw0 || symw1) lh = fl_height();
    }
    if (img) { imgw = Fl_Image_w(img); imgh = Fl_Image_h(img); }

    if (align & FL_ALIGN_CLIP) fl_push_clip(x, y, w, h);

    total_h = lh + imgh;
    if (align & FL_ALIGN_BOTTOM) top = y + h - total_h;
    else if (align & FL_ALIGN_TOP) top = y;
    else top = y + (h - total_h) / 2;

    if (img && !text_over_image) {
        if (align & FL_ALIGN_LEFT) lx = x;
        else if (align & FL_ALIGN_RIGHT) lx = x + w - imgw;
        else lx = x + (w - imgw) / 2;
        Fl_Image_draw_at(img, lx, top);
        top += imgh;
    }

    if (has_text || symw0 || symw1) {
        int block_w = symw0 + lw + symw1;
        int bx, sx, sy;
        if (align & FL_ALIGN_LEFT) bx = x;
        else if (align & FL_ALIGN_RIGHT) bx = x + w - block_w;
        else bx = x + (w - block_w) / 2;

        ly = top;
        if (symw0) {
            sy = ly + (lh - symw0) / 2;
            fl_draw_symbol(sym0, bx, sy, symw0, symw0, label->color);
        }
        lx = bx + symw0;
        if (has_text) {
            baseline = ly + fl_height() - fl_descent();
            fl_draw_text(text, dn, lx, baseline);
            if (underline >= 0) {
                int ux = lx + (int)fl_width(text, underline);
                int uw = (int)fl_width(text + underline, 1);
                if (uw < 1) uw = 1;
                fl_line(ux, baseline + 2, ux + uw - 1, baseline + 2);
            }
        }
        if (symw1) {
            sx = bx + symw0 + lw;
            sy = ly + (lh - symw1) / 2;
            fl_draw_symbol(sym1, sx, sy, symw1, symw1, label->color);
        }
        top += lh;
    }

    if (img && text_over_image) {
        if (align & FL_ALIGN_LEFT) lx = x;
        else if (align & FL_ALIGN_RIGHT) lx = x + w - imgw;
        else lx = x + (w - imgw) / 2;
        Fl_Image_draw_at(img, lx, top);
    }

    if (align & FL_ALIGN_CLIP) fl_pop_clip();
}

/* Pluggable labeltype registry - see fl_draw.h's own doc comment on
 * Fl_set_labeltype(). A plain 256-entry sparse table (any uchar
 * labeltype, not just >=FL_FREE_LABELTYPE, can be overridden - most
 * notably FL_NORMAL_LABEL itself, upstream's own most common use). */
typedef struct { Fl_Label_Draw_F *draw; Fl_Label_Measure_F *measure; int active; } LabelTypeEntry;
static LabelTypeEntry g_labeltypes[256];

void Fl_set_labeltype(uchar type, Fl_Label_Draw_F *draw, Fl_Label_Measure_F *measure) {
    g_labeltypes[type].draw = draw;
    g_labeltypes[type].measure = measure;
    g_labeltypes[type].active = 1;
}

void fl_label_measure(const Fl_Label *label, int *w, int *h) {
    if (g_labeltypes[label->type].active && g_labeltypes[label->type].measure) {
        g_labeltypes[label->type].measure(label, w, h);
        return;
    }
    fl_label_measure_default(label, w, h);
}

void fl_label_draw(const Fl_Label *label, int x, int y, int w, int h, Fl_Align align) {
    if (g_labeltypes[label->type].active && g_labeltypes[label->type].draw) {
        g_labeltypes[label->type].draw(label, x, y, w, h, align);
        return;
    }
    fl_label_draw_default(label, x, y, w, h, align);
}
