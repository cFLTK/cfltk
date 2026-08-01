/*
 * cfltk - fl_x11_driver.c
 *
 * X11/Xft implementation of Fl_Graphics_Driver (see cfltk/fl_draw.h).
 * Translated in spirit from src/xlib/Fl_Xlib_Graphics_Driver_*.cxx and
 * src/fl_font.cxx (font loading via Xft rather than X core fonts, which
 * upstream itself moved to via Xft/Pango on modern Linux builds).
 *
 * Known differences vs upstream: clipping is a single-rectangle stack
 * (intersected on push), not an arbitrary X Region; complex/concave
 * polygon fills, transforms and offscreen surfaces are not implemented.
 * See docs/DESIGN.md.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fl_x11_internal.h"

#define CLIP_STACK_DEPTH 64

typedef struct { int x, y, w, h; int active; } ClipRect;

static ClipRect g_clip_stack[CLIP_STACK_DEPTH];
static int g_clip_top = -1; /* -1 means "no clip" */

static Fl_Color g_current_color = FL_BLACK;
static Fl_Font g_current_font = FL_HELVETICA;
static Fl_Fontsize g_current_size = 14;

Fl_X11_Window *fl_x11_current_target = NULL;

/* ------------------------------------------------------------------ */
/* Color                                                               */
/* ------------------------------------------------------------------ */

static XftColor g_xft_color;
static int g_xft_color_valid = 0;

static void apply_color(Fl_Color c) {
    uchar r, g, b;
    XRenderColor rc;

    fl_get_color_rgb(c, &r, &g, &b);
    g_current_color = c;

    if (!fl_x11_current_target) return;

    if (g_xft_color_valid) XftColorFree(fl_x11_display, fl_x11_visual, fl_x11_colormap, &g_xft_color);
    rc.red = (unsigned short)(r << 8 | r);
    rc.green = (unsigned short)(g << 8 | g);
    rc.blue = (unsigned short)(b << 8 | b);
    rc.alpha = 0xffff;
    XftColorAllocValue(fl_x11_display, fl_x11_visual, fl_x11_colormap, &rc, &g_xft_color);
    g_xft_color_valid = 1;

    XSetForeground(fl_x11_display, fl_x11_current_target->gc,
                    (r << 16) | (g << 8) | b);
}

static void d_color_index(Fl_Color c) { apply_color(c); }
static void d_color_rgb(uchar r, uchar g, uchar b) { apply_color(fl_rgb_color(r, g, b)); }
static Fl_Color d_current_color(void) { return g_current_color; }

/* ------------------------------------------------------------------ */
/* Clipping                                                            */
/* ------------------------------------------------------------------ */

static void apply_clip(void) {
    if (!fl_x11_current_target) return;
    if (g_clip_top < 0 || !g_clip_stack[g_clip_top].active) {
        XSetClipMask(fl_x11_display, fl_x11_current_target->gc, None);
        if (fl_x11_current_target->xft_draw) XftDrawSetClip(fl_x11_current_target->xft_draw, None);
    } else {
        XRectangle r;
        ClipRect *c = &g_clip_stack[g_clip_top];
        r.x = (short)c->x; r.y = (short)c->y;
        r.width = (unsigned short)(c->w > 0 ? c->w : 0);
        r.height = (unsigned short)(c->h > 0 ? c->h : 0);
        XSetClipRectangles(fl_x11_display, fl_x11_current_target->gc, 0, 0, &r, 1, Unsorted);
        /* Xft text is drawn through a separate XftDraw object (see
         * d_draw_text()) that does NOT share the GC's clip mask -- it
         * needs its own, independently-set clip or clipped widgets'
         * labels render straight through their box borders. */
        if (fl_x11_current_target->xft_draw) XftDrawSetClipRectangles(fl_x11_current_target->xft_draw, 0, 0, &r, 1);
    }
}

static void d_push_clip(int x, int y, int w, int h) {
    ClipRect next;
    next.x = x; next.y = y; next.w = w; next.h = h; next.active = 1;
    if (g_clip_top >= 0 && g_clip_stack[g_clip_top].active) {
        ClipRect *p = &g_clip_stack[g_clip_top];
        int x1 = x > p->x ? x : p->x;
        int y1 = y > p->y ? y : p->y;
        int x2 = (x + w) < (p->x + p->w) ? (x + w) : (p->x + p->w);
        int y2 = (y + h) < (p->y + p->h) ? (y + h) : (p->y + p->h);
        next.x = x1; next.y = y1; next.w = x2 - x1; next.h = y2 - y1;
    }
    if (g_clip_top + 1 < CLIP_STACK_DEPTH) g_clip_stack[++g_clip_top] = next;
    apply_clip();
}

static void d_push_no_clip(void) {
    ClipRect none = {0, 0, 0, 0, 0};
    if (g_clip_top + 1 < CLIP_STACK_DEPTH) g_clip_stack[++g_clip_top] = none;
    apply_clip();
}

static void d_pop_clip(void) {
    if (g_clip_top >= 0) g_clip_top--;
    apply_clip();
}

static int d_not_clipped(int x, int y, int w, int h) {
    if (g_clip_top < 0 || !g_clip_stack[g_clip_top].active) return 1;
    ClipRect *c = &g_clip_stack[g_clip_top];
    return !(x + w <= c->x || x >= c->x + c->w || y + h <= c->y || y >= c->y + c->h);
}

static int d_clip_box(int x, int y, int w, int h, int *X, int *Y, int *W, int *H) {
    *X = x; *Y = y; *W = w; *H = h;
    if (g_clip_top < 0 || !g_clip_stack[g_clip_top].active) return 0;
    {
        ClipRect *c = &g_clip_stack[g_clip_top];
        int x1 = x > c->x ? x : c->x;
        int y1 = y > c->y ? y : c->y;
        int x2 = (x + w) < (c->x + c->w) ? (x + w) : (c->x + c->w);
        int y2 = (y + h) < (c->y + c->h) ? (y + h) : (c->y + c->h);
        *X = x1; *Y = y1; *W = x2 - x1; *H = y2 - y1;
        return (*X != x || *Y != y || *W != w || *H != h);
    }
}

/* ------------------------------------------------------------------ */
/* Line style                                                           */
/* ------------------------------------------------------------------ */

static void d_line_style(int style, int width, const char *dashes) {
    static const char k_dash[] = {6, 0};
    static const char k_dot[] = {1, 3, 0};
    static const char k_dashdot[] = {5, 3, 1, 3, 0};
    static const char k_dashdotdot[] = {5, 3, 1, 3, 1, 3, 0};
    int gc_cap, gc_join, gc_dash_style = LineSolid;
    const char *pattern = dashes;

    switch (style & 0xf00) {
        case FL_CAP_ROUND: gc_cap = CapRound; break;
        case FL_CAP_SQUARE: gc_cap = CapProjecting; break;
        default: gc_cap = CapButt; break;
    }
    switch (style & 0xf000) {
        case FL_JOIN_ROUND: gc_join = JoinRound; break;
        case FL_JOIN_BEVEL: gc_join = JoinBevel; break;
        default: gc_join = JoinMiter; break;
    }

    if (!pattern) {
        switch (style & 0xff) {
            case FL_DASH: pattern = k_dash; break;
            case FL_DOT: pattern = k_dot; break;
            case FL_DASHDOT: pattern = k_dashdot; break;
            case FL_DASHDOTDOT: pattern = k_dashdotdot; break;
            default: pattern = NULL; break;
        }
    }
    if (pattern && *pattern) gc_dash_style = LineOnOffDash;

    if (!fl_x11_current_target) return;
    XSetLineAttributes(fl_x11_display, fl_x11_current_target->gc, (unsigned)(width > 0 ? width : 0),
                        gc_dash_style, gc_cap, gc_join);
    if (gc_dash_style == LineOnOffDash) {
        int n = 0;
        char buf[16];
        while (pattern[n] && n < (int)sizeof(buf)) { buf[n] = pattern[n]; n++; }
        if (n > 0) XSetDashes(fl_x11_display, fl_x11_current_target->gc, 0, buf, n);
    }
}

/* ------------------------------------------------------------------ */
/* Primitives                                                          */
/* ------------------------------------------------------------------ */

static void d_point(int x, int y) {
    if (fl_x11_current_target) XDrawPoint(fl_x11_display, fl_x11_current_target->xid, fl_x11_current_target->gc, x, y);
}
static void d_line(int x, int y, int x1, int y1) {
    if (fl_x11_current_target) XDrawLine(fl_x11_display, fl_x11_current_target->xid, fl_x11_current_target->gc, x, y, x1, y1);
}
static void d_line3(int x, int y, int x1, int y1, int x2, int y2) {
    XPoint pts[3] = {{(short)x, (short)y}, {(short)x1, (short)y1}, {(short)x2, (short)y2}};
    if (fl_x11_current_target) XDrawLines(fl_x11_display, fl_x11_current_target->xid, fl_x11_current_target->gc, pts, 3, CoordModeOrigin);
}
static void d_xyline(int x, int y, int x1) { d_line(x, y, x1, y); }
static void d_yxline(int x, int y, int y1) { d_line(x, y, x, y1); }

static void d_rect(int x, int y, int w, int h) {
    if (!fl_x11_current_target || w <= 0 || h <= 0) return;
    XDrawRectangle(fl_x11_display, fl_x11_current_target->xid, fl_x11_current_target->gc, x, y, (unsigned)(w - 1), (unsigned)(h - 1));
}
static void d_rectf(int x, int y, int w, int h) {
    if (!fl_x11_current_target || w <= 0 || h <= 0) return;
    XFillRectangle(fl_x11_display, fl_x11_current_target->xid, fl_x11_current_target->gc, x, y, (unsigned)w, (unsigned)h);
}

static void d_arc(int x, int y, int w, int h, double a1, double a2) {
    if (!fl_x11_current_target || w <= 0 || h <= 0) return;
    XDrawArc(fl_x11_display, fl_x11_current_target->xid, fl_x11_current_target->gc,
              x, y, (unsigned)w, (unsigned)h, (int)(a1 * 64.0), (int)((a2 - a1) * 64.0));
}
static void d_pie(int x, int y, int w, int h, double a1, double a2) {
    if (!fl_x11_current_target || w <= 0 || h <= 0) return;
    XFillArc(fl_x11_display, fl_x11_current_target->xid, fl_x11_current_target->gc,
              x, y, (unsigned)w, (unsigned)h, (int)(a1 * 64.0), (int)((a2 - a1) * 64.0));
}

static void d_loop3(int x, int y, int x1, int y1, int x2, int y2) {
    XPoint pts[4] = {{(short)x, (short)y}, {(short)x1, (short)y1}, {(short)x2, (short)y2}, {(short)x, (short)y}};
    if (fl_x11_current_target) XDrawLines(fl_x11_display, fl_x11_current_target->xid, fl_x11_current_target->gc, pts, 4, CoordModeOrigin);
}
static void d_polygon3(int x, int y, int x1, int y1, int x2, int y2) {
    XPoint pts[3] = {{(short)x, (short)y}, {(short)x1, (short)y1}, {(short)x2, (short)y2}};
    if (fl_x11_current_target) XFillPolygon(fl_x11_display, fl_x11_current_target->xid, fl_x11_current_target->gc, pts, 3, Convex, CoordModeOrigin);
}

/* ------------------------------------------------------------------ */
/* Fonts / text                                                        */
/* ------------------------------------------------------------------ */

#define FONT_CACHE_SIZE 64
typedef struct { Fl_Font face; Fl_Fontsize size; XftFont *font; } FontCacheEntry;
static FontCacheEntry g_font_cache[FONT_CACHE_SIZE];
static int g_font_cache_count = 0;

static const char *font_family(Fl_Font face) {
    switch (face & ~3) {
        case FL_COURIER: return "monospace";
        case FL_TIMES: return "serif";
        default: return "sans";
    }
}
static int font_is_bold(Fl_Font face) { return (face >= FL_HELVETICA && face <= FL_TIMES_BOLD_ITALIC) && (face & 1); }
static int font_is_italic(Fl_Font face) { return (face >= FL_HELVETICA && face <= FL_TIMES_BOLD_ITALIC) && (face & 2); }

static XftFont *load_font(Fl_Font face, Fl_Fontsize size) {
    int i;
    char pattern[128];
    XftFont *f;

    for (i = 0; i < g_font_cache_count; i++)
        if (g_font_cache[i].face == face && g_font_cache[i].size == size) return g_font_cache[i].font;

    snprintf(pattern, sizeof(pattern), "%s:pixelsize=%d%s%s",
             font_family(face), size <= 0 ? 14 : size,
             font_is_bold(face) ? ":bold" : "",
             font_is_italic(face) ? ":italic" : "");
    f = XftFontOpenName(fl_x11_display, fl_x11_screen, pattern);
    if (!f) f = XftFontOpenName(fl_x11_display, fl_x11_screen, "sans-10");

    if (g_font_cache_count < FONT_CACHE_SIZE) {
        g_font_cache[g_font_cache_count].face = face;
        g_font_cache[g_font_cache_count].size = size;
        g_font_cache[g_font_cache_count].font = f;
        g_font_cache_count++;
    }
    return f;
}

static void d_font(Fl_Font face, Fl_Fontsize size) {
    g_current_font = face;
    g_current_size = size;
}
static Fl_Font d_current_font(void) { return g_current_font; }
static Fl_Fontsize d_current_size(void) { return g_current_size; }

static int d_height(void) {
    XftFont *f = load_font(g_current_font, g_current_size);
    return f ? (int)(f->ascent + f->descent) : g_current_size;
}
static int d_descent(void) {
    XftFont *f = load_font(g_current_font, g_current_size);
    return f ? (int)f->descent : 0;
}
static double d_width(const char *text, int n) {
    XftFont *f;
    XGlyphInfo extents;
    if (!text || n <= 0) return 0.0;
    f = load_font(g_current_font, g_current_size);
    if (!f) return 0.0;
    XftTextExtentsUtf8(fl_x11_display, f, (const FcChar8 *)text, n, &extents);
    return (double)extents.xOff;
}

static void d_draw_text(const char *str, int n, int x, int y) {
    XftFont *f;
    if (!fl_x11_current_target || !str || n <= 0) return;
    f = load_font(g_current_font, g_current_size);
    if (!f) return;
    if (!fl_x11_current_target->xft_draw) return;
    XftDrawStringUtf8(fl_x11_current_target->xft_draw, &g_xft_color, f, x, y, (const FcChar8 *)str, n);
}

/* ------------------------------------------------------------------ */

static const Fl_Graphics_Driver g_driver = {
    d_color_index, d_color_rgb, d_current_color,
    d_push_clip, d_push_no_clip, d_pop_clip, d_not_clipped, d_clip_box,
    d_line_style,
    d_point, d_line, d_line3, d_xyline, d_yxline, d_rect, d_rectf,
    d_loop3, d_polygon3, d_arc, d_pie,
    d_font, d_current_font, d_current_size, d_height, d_descent, d_width,
    d_draw_text
};

const Fl_Graphics_Driver *fl_x11_graphics_driver(void) { return &g_driver; }

void fl_x11_driver_init(void) {
    g_clip_top = -1;
    fl_set_graphics_driver(&g_driver);
}
