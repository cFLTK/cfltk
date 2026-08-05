/*
 * cfltk - fl_nx_driver.c
 *
 * mwin (Microwindows Win32-compatible GDI) implementation of
 * Fl_Graphics_Driver (see cfltk/fl_draw.h). Modeled on
 * fl_x11_driver.c's structure.
 *
 * First-milestone scope: solid colors, clipping (single-rectangle
 * stack, same simplification the X11 backend itself makes), and the
 * line/rect/arc/polygon primitives. Text/font measurement and drawing,
 * raw image blit/read-back, bitmask stipple, and offscreen surfaces
 * are honest no-op/best-guess stubs, not silently wrong: see each
 * one's own comment below. None of them are reachable from a bare
 * Fl_Window with no label or child widgets, which is as far as this
 * milestone needs to go.
 */
#include <nuttx/config.h> /* must be first -- see fl_nx_window.c's comment */
#include <math.h>
#include <stdlib.h>

#include "fl_nx_internal.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define CLIP_STACK_DEPTH 64

typedef struct { int x, y, w, h; int active; } ClipRect;

static ClipRect g_clip_stack[CLIP_STACK_DEPTH];
static int g_clip_top = -1; /* -1 means "no clip" */

static Fl_Color g_current_color = FL_BLACK;
static Fl_Font g_current_font = FL_HELVETICA;
static Fl_Fontsize g_current_size = 14;

static HPEN g_pen = NULL;
static HBRUSH g_brush = NULL;

/* ------------------------------------------------------------------ */
/* Color                                                               */
/* ------------------------------------------------------------------ */

static void apply_color(Fl_Color c) {
    uchar r, g, b;
    COLORREF cr;

    fl_get_color_rgb(c, &r, &g, &b);
    g_current_color = c;

    if (!fl_nx_current_target) return;

    cr = RGB(r, g, b);
    if (g_pen) DeleteObject(g_pen);
    if (g_brush) DeleteObject(g_brush);
    g_pen = CreatePen(PS_SOLID, 1, cr);
    g_brush = CreateSolidBrush(cr);
    SelectObject(fl_nx_current_target->hdc, g_pen);
    SetTextColor(fl_nx_current_target->hdc, cr);
}

static void d_color_index(Fl_Color c) { apply_color(c); }
static void d_color_rgb(uchar r, uchar g, uchar b) { apply_color(fl_rgb_color(r, g, b)); }
static Fl_Color d_current_color(void) { return g_current_color; }

/* ------------------------------------------------------------------ */
/* Clipping                                                            */
/* ------------------------------------------------------------------ */

static void apply_clip(void) {
    if (!fl_nx_current_target) return;
    if (g_clip_top < 0 || !g_clip_stack[g_clip_top].active) {
        SelectClipRgn(fl_nx_current_target->hdc, NULL);
    } else {
        ClipRect *c = &g_clip_stack[g_clip_top];
        HRGN rgn = CreateRectRgn(c->x, c->y, c->x + c->w, c->y + c->h);
        SelectClipRgn(fl_nx_current_target->hdc, rgn);
        DeleteObject(rgn); /* SelectClipRgn copies it in; safe to drop our ref */
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
/* Line style                                                          */
/* ------------------------------------------------------------------ */

static void d_line_style(int style, int width, const char *dashes) {
    /* mwin's CreatePen() takes a single PS_SOLID/PS_DASH/PS_DOT style,
     * no cap/join/custom-dash-pattern control -- coarser than X11's
     * XSetLineAttributes(). Close enough for a blank window (no lines
     * drawn at all yet); revisit once real widget borders exercise
     * this. */
    int pen_style = PS_SOLID;
    (void)dashes;
    switch (style & 0xff) {
        case FL_DASH: pen_style = PS_DASH; break;
        case FL_DOT: pen_style = PS_DOT; break;
        default: pen_style = PS_SOLID; break;
    }
    if (!fl_nx_current_target) return;
    if (g_pen) DeleteObject(g_pen);
    {
        uchar r, g, b;
        fl_get_color_rgb(g_current_color, &r, &g, &b);
        g_pen = CreatePen(pen_style, width > 0 ? width : 1, RGB(r, g, b));
    }
    SelectObject(fl_nx_current_target->hdc, g_pen);
}

/* ------------------------------------------------------------------ */
/* Primitives                                                          */
/* ------------------------------------------------------------------ */

static void d_point(int x, int y) {
    uchar r, g, b;
    if (!fl_nx_current_target) return;
    fl_get_color_rgb(g_current_color, &r, &g, &b);
    SetPixel(fl_nx_current_target->hdc, x, y, RGB(r, g, b));
}
static void d_line(int x, int y, int x1, int y1) {
    if (!fl_nx_current_target) return;
    MoveToEx(fl_nx_current_target->hdc, x, y, NULL);
    LineTo(fl_nx_current_target->hdc, x1, y1);
}
static void d_line3(int x, int y, int x1, int y1, int x2, int y2) {
    if (!fl_nx_current_target) return;
    MoveToEx(fl_nx_current_target->hdc, x, y, NULL);
    LineTo(fl_nx_current_target->hdc, x1, y1);
    LineTo(fl_nx_current_target->hdc, x2, y2);
}
static void d_xyline(int x, int y, int x1) { d_line(x, y, x1, y); }
static void d_yxline(int x, int y, int y1) { d_line(x, y, x, y1); }

static void d_rect(int x, int y, int w, int h) {
    if (!fl_nx_current_target || w <= 0 || h <= 0) return;
    SelectObject(fl_nx_current_target->hdc, GetStockObject(NULL_BRUSH));
    Rectangle(fl_nx_current_target->hdc, x, y, x + w, y + h);
    SelectObject(fl_nx_current_target->hdc, g_brush);
}
static void d_rectf(int x, int y, int w, int h) {
    RECT rc;
    if (!fl_nx_current_target || w <= 0 || h <= 0) return;
    rc.left = x; rc.top = y; rc.right = x + w; rc.bottom = y + h;
    FillRect(fl_nx_current_target->hdc, &rc, g_brush);
}

static void angle_point(int x, int y, int w, int h, double deg, int *px, int *py) {
    double cx = x + w / 2.0, cy = y + h / 2.0, rx = w / 2.0, ry = h / 2.0;
    double rad = deg * M_PI / 180.0;
    *px = (int)(cx + rx * cos(rad));
    *py = (int)(cy - ry * sin(rad));
}

static void d_arc(int x, int y, int w, int h, double a1, double a2) {
    int sx, sy, ex, ey;
    if (!fl_nx_current_target || w <= 0 || h <= 0) return;
    angle_point(x, y, w, h, a1, &sx, &sy);
    angle_point(x, y, w, h, a2, &ex, &ey);
    Arc(fl_nx_current_target->hdc, x, y, x + w, y + h, sx, sy, ex, ey);
}
static void d_pie(int x, int y, int w, int h, double a1, double a2) {
    int sx, sy, ex, ey;
    if (!fl_nx_current_target || w <= 0 || h <= 0) return;
    angle_point(x, y, w, h, a1, &sx, &sy);
    angle_point(x, y, w, h, a2, &ex, &ey);
    Pie(fl_nx_current_target->hdc, x, y, x + w, y + h, sx, sy, ex, ey);
}

static void d_loop3(int x, int y, int x1, int y1, int x2, int y2) {
    if (!fl_nx_current_target) return;
    MoveToEx(fl_nx_current_target->hdc, x, y, NULL);
    LineTo(fl_nx_current_target->hdc, x1, y1);
    LineTo(fl_nx_current_target->hdc, x2, y2);
    LineTo(fl_nx_current_target->hdc, x, y);
}
static void d_polygon3(int x, int y, int x1, int y1, int x2, int y2) {
    POINT pts[3];
    if (!fl_nx_current_target) return;
    pts[0].x = x; pts[0].y = y;
    pts[1].x = x1; pts[1].y = y1;
    pts[2].x = x2; pts[2].y = y2;
    Polygon(fl_nx_current_target->hdc, pts, 3);
}

static void d_fill_polygon(const int *xs, const int *ys, int n) {
    POINT stackbuf[32];
    POINT *pts = stackbuf;
    int i;
    if (!fl_nx_current_target || n < 3) return;
    if (n > 32) pts = (POINT *)malloc(sizeof(POINT) * (size_t)n);
    for (i = 0; i < n; i++) { pts[i].x = xs[i]; pts[i].y = ys[i]; }
    Polygon(fl_nx_current_target->hdc, pts, n);
    if (pts != stackbuf) free(pts);
}

static void d_draw_polyline(const int *xs, const int *ys, int n, int closed) {
    POINT stackbuf[32];
    POINT *pts = stackbuf;
    int i, count = n;
    if (!fl_nx_current_target || n < 2) return;
    if (closed) count++;
    if (count > 32) pts = (POINT *)malloc(sizeof(POINT) * (size_t)count);
    for (i = 0; i < n; i++) { pts[i].x = xs[i]; pts[i].y = ys[i]; }
    if (closed) { pts[n].x = xs[0]; pts[n].y = ys[0]; }
    Polyline(fl_nx_current_target->hdc, pts, count);
    if (pts != stackbuf) free(pts);
}

static void d_scroll_blit(int src_x, int src_y, int src_w, int src_h, int dest_x, int dest_y) {
    if (!fl_nx_current_target || src_w <= 0 || src_h <= 0) return;
    BitBlt(fl_nx_current_target->hdc, dest_x, dest_y, src_w, src_h,
           fl_nx_current_target->hdc, src_x, src_y, SRCCOPY);
}

/* ------------------------------------------------------------------ */
/* Fonts / text -- NOT YET IMPLEMENTED.                                 */
/*                                                                      */
/* mwin's own font API (winfont.h: CreateFont/SelectObject(HFONT)/     */
/* TextOut/GetTextExtentPoint32) is real and could back this properly, */
/* but doing it right needs its own investigation pass (font          */
/* selection matching cfltk's Fl_Font/Fl_Fontsize model, the same way */
/* fl_x11_driver.c's load_font() maps them onto fontconfig patterns). */
/* Deliberately NOT guessed at here: draw_text() below draws nothing  */
/* rather than drawing something plausible-looking but wrong, and the */
/* size functions return small fixed placeholders so layout code that */
/* divides by height/width doesn't divide by zero -- neither is a     */
/* real implementation. A window with a label or any text widget will */
/* show blank space where the text should be until this is done.      */
/* ------------------------------------------------------------------ */

static void d_font(Fl_Font face, Fl_Fontsize size) { g_current_font = face; g_current_size = size; }
static Fl_Font d_current_font(void) { return g_current_font; }
static Fl_Fontsize d_current_size(void) { return g_current_size; }
static int d_height(void) { return g_current_size > 0 ? g_current_size + 2 : 16; }
static int d_descent(void) { return g_current_size > 0 ? g_current_size / 4 : 4; }
static double d_width(const char *text, int n) {
    if (!text || n <= 0) return 0.0;
    return (double)n * (double)(g_current_size > 0 ? g_current_size : 14) * 0.6;
}
static void d_draw_text(const char *str, int n, int x, int y) { (void)str; (void)n; (void)x; (void)y; }
static void d_text_extents(const char *text, int n, int *dx, int *dy, int *w, int *h) {
    (void)text; (void)n;
    *dx = 0; *dy = 0; *w = 0; *h = 0;
}

/* ------------------------------------------------------------------ */
/* Raw image blit/read-back, bitmask stipple -- NOT YET IMPLEMENTED.   */
/* Unreachable from a bare Fl_Window; real support is the Image        */
/* decoders/rendering milestone (docs/analysis.md Sec.6/7).            */
/* ------------------------------------------------------------------ */

static void d_draw_image(const unsigned char *buf, int x, int y, int w, int h, int d, int ld) {
    (void)buf; (void)x; (void)y; (void)w; (void)h; (void)d; (void)ld;
}
static void d_read_image(unsigned char *buf, int x, int y, int w, int h, int d) {
    /* Not implemented -- caller's buffer is left untouched. Only used
     * for software alpha compositing (see cfltk/fl_draw.h), not
     * exercised by a bare Fl_Window. */
    (void)buf; (void)x; (void)y; (void)w; (void)h; (void)d;
}
static void d_draw_bitmask(const unsigned char *bits, int bmp_w, int bmp_h, int cx, int cy, int x, int y, int w, int h) {
    (void)bits; (void)bmp_w; (void)bmp_h; (void)cx; (void)cy; (void)x; (void)y; (void)w; (void)h;
}

/* ------------------------------------------------------------------ */
/* Offscreen surfaces -- NOT YET IMPLEMENTED (see fl_nx_window.c's     */
/* fl_backend_window_flush(): double buffering isn't wired up yet      */
/* either, so nothing in this milestone calls these).                  */
/* ------------------------------------------------------------------ */

static Fl_Offscreen d_create_offscreen(int w, int h) { (void)w; (void)h; return (Fl_Offscreen)0; }
static void d_delete_offscreen(Fl_Offscreen o) { (void)o; }
static void d_begin_offscreen(Fl_Offscreen o) { (void)o; }
static void d_end_offscreen(void) {}
static void d_copy_offscreen(int x, int y, int w, int h, Fl_Offscreen o, int srcx, int srcy) {
    (void)x; (void)y; (void)w; (void)h; (void)o; (void)srcx; (void)srcy;
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
    d_create_offscreen, d_delete_offscreen, d_begin_offscreen,
    d_end_offscreen, d_copy_offscreen,
    d_scroll_blit
};

const Fl_Graphics_Driver *fl_nx_graphics_driver(void) { return &g_driver; }

void fl_nx_driver_init(void) {
    g_clip_top = -1;
    fl_set_graphics_driver(&g_driver);
}
