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
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE /* strdup()/strcasecmp() under strict -std=c99 */
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> /* strcasecmp */
#include <fontconfig/fontconfig.h>

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

/* Arbitrary-N-point versions backing the portable vertex/matrix layer
 * (fl_begin_polygon()/fl_vertex()/... in fl_draw.h) -- e.g. the
 * '@'-symbol label glyphs in fl_symbols.c. Uses a small fixed-size
 * stack buffer for the common case (every symbol shape has well under
 * 32 vertices) and falls back to malloc for anything larger. */
static void d_fill_polygon(const int *xs, const int *ys, int n) {
    XPoint stackbuf[32];
    XPoint *pts = stackbuf;
    int i;
    if (!fl_x11_current_target || n < 3) return;
    if (n > 32) pts = (XPoint *)malloc(sizeof(XPoint) * (size_t)n);
    for (i = 0; i < n; i++) { pts[i].x = (short)xs[i]; pts[i].y = (short)ys[i]; }
    XFillPolygon(fl_x11_display, fl_x11_current_target->xid, fl_x11_current_target->gc, pts, n, Complex, CoordModeOrigin);
    if (pts != stackbuf) free(pts);
}

static void d_draw_polyline(const int *xs, const int *ys, int n, int closed) {
    XPoint stackbuf[32];
    XPoint *pts = stackbuf;
    int i, count = n;
    if (!fl_x11_current_target || n < 2) return;
    if (closed) count++;
    if (count > 32) pts = (XPoint *)malloc(sizeof(XPoint) * (size_t)count);
    for (i = 0; i < n; i++) { pts[i].x = (short)xs[i]; pts[i].y = (short)ys[i]; }
    if (closed) { pts[n].x = (short)xs[0]; pts[n].y = (short)ys[0]; }
    XDrawLines(fl_x11_display, fl_x11_current_target->xid, fl_x11_current_target->gc, pts, count, CoordModeOrigin);
    if (pts != stackbuf) free(pts);
}

/* ------------------------------------------------------------------ */
/* Fonts / text                                                        */
/* ------------------------------------------------------------------ */

#define FONT_CACHE_SIZE 64
typedef struct { Fl_Font face; Fl_Fontsize size; XftFont *font; } FontCacheEntry;
static FontCacheEntry g_font_cache[FONT_CACHE_SIZE];
static int g_font_cache_count = 0;

/* Custom families registered via Fl_set_font_family(), one entry per
 * 4-slot block starting at FL_FREE_FONT (regular/bold/italic/bold-italic,
 * same packing the 12 builtin FL_HELVETICA/FL_COURIER/FL_TIMES faces
 * already use - see font_is_bold/font_is_italic below). */
#define CUSTOM_FONT_FAMILIES_MAX 64
static char *g_custom_families[CUSTOM_FONT_FAMILIES_MAX];
static int g_custom_family_count = 0;

Fl_Font Fl_set_font_family(const char *name) {
    int i;
    if (!name || !*name) name = "sans";
    for (i = 0; i < g_custom_family_count; i++) {
        if (strcasecmp(g_custom_families[i], name) == 0)
            return FL_FREE_FONT + i * 4;
    }
    if (g_custom_family_count >= CUSTOM_FONT_FAMILIES_MAX)
        return FL_HELVETICA; /* table full: fall back to a safe builtin */
    g_custom_families[g_custom_family_count] = strdup(name);
    return FL_FREE_FONT + (g_custom_family_count++) * 4;
}

/* Real fontconfig existence check (unlike Fl_set_font_family(), which
 * always succeeds by design - Xft/fontconfig substitute a default when
 * a family isn't installed, so it can't itself distinguish "found the
 * real thing" from "silently substituted"). Resolves name via the same
 * matching fontconfig/Xft would use, then checks whether the *matched*
 * family genuinely equals the requested one - an exact substring/case-
 * insensitive comparison, since fontconfig's own substitution aliases
 * (e.g. generic "sans"/"serif"/"monospace") legitimately resolve to a
 * different concrete family name on purpose. */
int Fl_font_family_exists(const char *name) {
    FcPattern *pat;
    FcResult result;
    FcPattern *match;
    int exists = 0;

    if (!name || !*name) return 0;

    pat = FcNameParse((const FcChar8 *)name);
    if (!pat) return 0;
    FcConfigSubstitute(NULL, pat, FcMatchPattern);
    FcDefaultSubstitute(pat);

    match = FcFontMatch(NULL, pat, &result);
    if (match) {
        FcChar8 *matched_family = NULL;
        if (FcPatternGetString(match, FC_FAMILY, 0, &matched_family) == FcResultMatch &&
            matched_family) {
            exists = (strcasecmp((const char *)matched_family, name) == 0);
        }
        FcPatternDestroy(match);
    }
    FcPatternDestroy(pat);
    return exists;
}

static const char *font_family(Fl_Font face) {
    if (face >= FL_FREE_FONT) {
        int idx = (face - FL_FREE_FONT) / 4;
        if (idx >= 0 && idx < g_custom_family_count) return g_custom_families[idx];
        return "sans";
    }
    switch (face & ~3) {
        case FL_COURIER: return "monospace";
        case FL_TIMES: return "serif";
        default: return "sans";
    }
}
static int font_is_bold(Fl_Font face) {
    return ((face >= FL_HELVETICA && face <= FL_TIMES_BOLD_ITALIC) || face >= FL_FREE_FONT) && (face & 1);
}
static int font_is_italic(Fl_Font face) {
    return ((face >= FL_HELVETICA && face <= FL_TIMES_BOLD_ITALIC) || face >= FL_FREE_FONT) && (face & 2);
}

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

static void d_text_extents(const char *text, int n, int *dx, int *dy, int *w, int *h) {
    XftFont *f;
    XGlyphInfo extents;
    if (!text || n <= 0) { *dx = *dy = *w = *h = 0; return; }
    f = load_font(g_current_font, g_current_size);
    if (!f) { *dx = *dy = *w = *h = 0; return; }
    XftTextExtentsUtf8(fl_x11_display, f, (const FcChar8 *)text, n, &extents);
    /* Xft's XGlyphInfo.x/y is the offset from the pen origin to the
     * ink box's top-left corner, stored negated (matches how
     * upstream's own Xft-based fl_text_extents() reads it). */
    *dx = -extents.x;
    *dy = -extents.y;
    *w = (int)extents.width;
    *h = (int)extents.height;
}

static void d_scroll_blit(int src_x, int src_y, int src_w, int src_h, int dest_x, int dest_y) {
    if (!fl_x11_current_target || src_w <= 0 || src_h <= 0) return;
    XCopyArea(fl_x11_display, fl_x11_current_target->xid, fl_x11_current_target->xid,
              fl_x11_current_target->gc, src_x, src_y, (unsigned)src_w, (unsigned)src_h,
              dest_x, dest_y);
}

/* ------------------------------------------------------------------ */
/* Raw image blit/read-back                                            */
/*                                                                      */
/* Assumes the default visual is a standard 24/32-bit TrueColor visual */
/* with byte order (r<<16)|(g<<8)|b -- already the assumption baked    */
/* into apply_color()'s XSetForeground() call above, so this adds no   */
/* new limitation. No offscreen/cached surface: every call builds a    */
/* fresh XImage and blits it through the current target's GC (whose    */
/* clip rectangle -- see apply_clip() -- X applies automatically to    */
/* XPutImage, so no separate clip-box math is needed here beyond what  */
/* Fl_RGB_Image itself does to crop the *source* array).                */
/* ------------------------------------------------------------------ */

static void d_draw_image(const unsigned char *buf, int x, int y, int w, int h, int d, int ld) {
    XImage *img;
    int row, col, stride;
    unsigned char *data;

    if (!fl_x11_current_target || w <= 0 || h <= 0) return;

    stride = ld != 0 ? ld : w * d;
    data = (unsigned char *)malloc((size_t)w * (size_t)h * 4);
    if (!data) return;

    img = XCreateImage(fl_x11_display, fl_x11_visual, 24, ZPixmap, 0, (char *)data, (unsigned)w, (unsigned)h, 32, 0);
    if (!img) { free(data); return; }

    for (row = 0; row < h; row++) {
        const unsigned char *src = buf + (size_t)row * (size_t)stride;
        for (col = 0; col < w; col++) {
            unsigned long pixel;
            if (d >= 3) {
                pixel = ((unsigned long)src[0] << 16) | ((unsigned long)src[1] << 8) | (unsigned long)src[2];
                src += d;
            } else {
                pixel = ((unsigned long)src[0] << 16) | ((unsigned long)src[0] << 8) | (unsigned long)src[0];
                src += d;
            }
            XPutPixel(img, col, row, pixel);
        }
    }

    XPutImage(fl_x11_display, fl_x11_current_target->xid, fl_x11_current_target->gc, img, 0, 0, x, y, (unsigned)w, (unsigned)h);
    XDestroyImage(img); /* also frees `data` */
}

/* XGetImage legitimately fails with a BadMatch protocol error if the
 * target window is (even momentarily) not fully viewable - e.g. mid
 * resize, mid expose, or partially obscured/off-screen at the exact
 * instant a masked pixmap is redrawn (see Fl_Pixmap.c's
 * mask_composite(), the caller most likely to hit this in practice: it
 * runs on every redraw of a transparent toolbar icon). Xlib's default
 * error handler treats *any* unhandled protocol error as fatal and
 * terminates the whole process - this crashed real, tested dillo
 * builds in practice, matching a real user's bug report. Matches
 * upstream FLTK's own fl_read_image() (fl_read_image.cxx), which
 * wraps this exact call with a temporary error handler for this exact
 * reason ("the window is obscured etc. the function will still fail.
 * Make sure we catch the error and continue"). */
static int xgetimage_err_handler(Display *display, XErrorEvent *error) {
    (void)display; (void)error;
    return 0;
}

static void d_read_image(unsigned char *buf, int x, int y, int w, int h, int d) {
    XImage *img;
    int row, col;
    XErrorHandler old_handler;

    if (!fl_x11_current_target || w <= 0 || h <= 0) return;
    old_handler = XSetErrorHandler(xgetimage_err_handler);
    img = XGetImage(fl_x11_display, fl_x11_current_target->xid, x, y, (unsigned)w, (unsigned)h, AllPlanes, ZPixmap);
    XSetErrorHandler(old_handler);
    if (!img) { memset(buf, 0, (size_t)w * (size_t)h * (size_t)d); return; }

    for (row = 0; row < h; row++) {
        unsigned char *dst = buf + (size_t)row * (size_t)w * (size_t)d;
        for (col = 0; col < w; col++) {
            unsigned long pixel = XGetPixel(img, col, row);
            unsigned char r = (unsigned char)((pixel >> 16) & 0xff);
            unsigned char g = (unsigned char)((pixel >> 8) & 0xff);
            unsigned char b = (unsigned char)(pixel & 0xff);
            if (d >= 3) {
                dst[0] = r; dst[1] = g; dst[2] = b;
                dst += d;
            } else {
                dst[0] = (unsigned char)((r * 31 + g * 61 + b * 8) / 100);
                dst += d;
            }
        }
    }
    XDestroyImage(img);
}

/* Fl_Bitmap: builds a throwaway 1-bit X Pixmap from the XBM-style data
 * (no caching, see fl_draw.h's known differences) and stipple-fills
 * the requested rectangle with it in the current foreground color. */
static void d_draw_bitmask(const unsigned char *bits, int bmp_w, int bmp_h, int cx, int cy, int x, int y, int w, int h) {
    Pixmap pm;
    int ox, oy;

    if (!fl_x11_current_target || w <= 0 || h <= 0) return;

    pm = XCreateBitmapFromData(fl_x11_display, fl_x11_current_target->xid, (const char *)bits, (unsigned)((bmp_w + 7) & ~7), (unsigned)bmp_h);
    if (!pm) return;

    XSetStipple(fl_x11_display, fl_x11_current_target->gc, pm);
    ox = x - cx; if (ox < 0) ox += bmp_w;
    oy = y - cy; if (oy < 0) oy += bmp_h;
    XSetTSOrigin(fl_x11_display, fl_x11_current_target->gc, ox, oy);
    XSetFillStyle(fl_x11_display, fl_x11_current_target->gc, FillStippled);
    XFillRectangle(fl_x11_display, fl_x11_current_target->xid, fl_x11_current_target->gc, x, y, (unsigned)w, (unsigned)h);
    XSetFillStyle(fl_x11_display, fl_x11_current_target->gc, FillSolid);
    XFreePixmap(fl_x11_display, pm);
}

static const Fl_Graphics_Driver g_driver = {
    d_color_index, d_color_rgb, d_current_color,
    d_push_clip, d_push_no_clip, d_pop_clip, d_not_clipped, d_clip_box,
    d_line_style,
    d_point, d_line, d_line3, d_xyline, d_yxline, d_rect, d_rectf,
    d_loop3, d_polygon3, d_arc, d_pie,
    d_font, d_current_font, d_current_size, d_height, d_descent, d_width,
    d_draw_text,
    d_draw_image, d_read_image,
    d_draw_bitmask,
    d_fill_polygon, d_draw_polyline,
    d_text_extents,
    fl_x11_create_offscreen, fl_x11_delete_offscreen, fl_x11_begin_offscreen,
    fl_x11_end_offscreen, fl_x11_copy_offscreen,
    d_scroll_blit
};

const Fl_Graphics_Driver *fl_x11_graphics_driver(void) { return &g_driver; }

void fl_x11_driver_init(void) {
    g_clip_top = -1;
    fl_set_graphics_driver(&g_driver);
}
