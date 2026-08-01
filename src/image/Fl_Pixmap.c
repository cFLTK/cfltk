/*
 * cfltk - Fl_Pixmap.c
 * See include/cfltk/Fl_Pixmap.h for the class-conversion notes.
 * Translated from src/Fl_Pixmap.cxx and the XPM parsing engine in
 * src/fl_draw_pixmap.cxx (fl_measure_pixmap/fl_convert_pixmap).
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cfltk/Fl_Pixmap.h"
#include "cfltk/fl_draw.h"
#include "cfltk/fl_colormap.h"

/* ncolors/chars_per_pixel from the most recent fl_measure_pixmap()
 * call, consumed by fl_convert_pixmap() -- matches upstream's own
 * pair of file-static globals in fl_draw_pixmap.cxx (the two calls are
 * always made back-to-back by the same caller). */
static int g_ncolors, g_chars_per_pixel;

int fl_measure_pixmap(const char *const *cdata, int *W, int *H) {
    int n = sscanf(cdata[0], "%d%d%d%d", W, H, &g_ncolors, &g_chars_per_pixel);
    if (n < 4 || *W <= 0 || *H <= 0 || (g_chars_per_pixel != 1 && g_chars_per_pixel != 2)) {
        *W = 0;
        return 0;
    }
    return 1;
}

/* Scans past the "c word" (or last word if no "c" context) entry in an
 * XPM color-table line, starting just after the char(s)-per-pixel
 * index -- the exact same little state machine upstream repeats three
 * times (fl_convert_pixmap, Fl_Pixmap::color_average,
 * Fl_Pixmap::desaturate). */
static const char *find_color_word(const char *p) {
    const char *previous_word = p;
    for (;;) {
        char what;
        while (*p && isspace((unsigned char)*p)) p++;
        what = *p++;
        while (*p && !isspace((unsigned char)*p)) p++;
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) return previous_word;
        if (what == 'c') return p;
        previous_word = p;
        while (*p && !isspace((unsigned char)*p)) p++;
    }
}

int fl_convert_pixmap(const char *const *cdata, uchar *out, Fl_Color bg) {
    int w, h, i, X, Y;
    const unsigned char *const *data = (const unsigned char *const *)(cdata + 1);
    unsigned char (*colors)[4];
    unsigned char *q = out;

    if (!fl_measure_pixmap(cdata, &w, &h)) return 0;
    if (g_chars_per_pixel < 1 || g_chars_per_pixel > 2) return 0;

    colors = (unsigned char (*)[4])malloc(sizeof(unsigned char[4]) << (g_chars_per_pixel * 8));

    if (g_ncolors < 0) {
        const unsigned char *p = *data++;
        int n = -g_ncolors;
        if (*p == ' ') {
            unsigned char *c = colors[' '];
            fl_get_color_rgb(bg, &c[0], &c[1], &c[2]);
            c[3] = 0;
            p += 4;
            n--;
        }
        for (i = 0; i < n; i++) {
            unsigned char *c = colors[*p++];
            c[0] = *p++; c[1] = *p++; c[2] = *p++; c[3] = 255;
        }
    } else {
        for (i = 0; i < g_ncolors; i++) {
            const unsigned char *p = *data++;
            int ind = *p++;
            unsigned char *c;
            const char *word;
            if (g_chars_per_pixel > 1) ind = (ind << 8) | *p++;
            c = colors[ind];
            word = find_color_word((const char *)p);
            if (fl_parse_color(word, &c[0], &c[1], &c[2])) {
                c[3] = 255;
            } else {
                fl_get_color_rgb(bg, &c[0], &c[1], &c[2]);
                c[3] = 0;
            }
        }
    }

    for (Y = 0; Y < h; Y++) {
        const unsigned char *p = data[Y];
        if (g_chars_per_pixel <= 1) {
            for (X = 0; X < w; X++) { memcpy(q, colors[*p++], 4); q += 4; }
        } else {
            for (X = 0; X < w; X++) {
                int ind = (*p++) << 8;
                ind |= *p++;
                memcpy(q, colors[ind], 4);
                q += 4;
            }
        }
    }

    free(colors);
    return 1;
}

static void measure(Fl_Pixmap *self) {
    int W, H;
    if (self->image.w < 0 && self->image.data) {
        fl_measure_pixmap(self->image.data, &W, &H);
        self->image.w = W;
        self->image.h = H;
    }
}

static void set_data(Fl_Pixmap *self, const char *const *p) {
    int height, ncolors;
    if (p) {
        sscanf(p[0], "%*d%d%d", &height, &ncolors);
        self->image.data = p;
        self->image.count = ncolors < 0 ? height + 2 : height + ncolors + 1;
    }
}

void Fl_Pixmap_init(Fl_Pixmap *self, const char *const *D) {
    Fl_Image_init(&self->image, -1, 0, 1);
    self->image.ops = &fl_pixmap_ops;
    self->alloc_data = 0;
    set_data(self, D);
    measure(self);
}

Fl_Pixmap *Fl_Pixmap_new(const char *const *D) {
    Fl_Pixmap *self = (Fl_Pixmap *)malloc(sizeof(Fl_Pixmap));
    Fl_Pixmap_init(self, D);
    return self;
}

static void delete_data(Fl_Pixmap *self) {
    if (self->alloc_data) {
        int i;
        for (i = 0; i < self->image.count; i++) free((void *)self->image.data[i]);
        free((void *)self->image.data);
    }
}

static void pixmap_destroy(Fl_Image *base) { delete_data((Fl_Pixmap *)base); }
static void pixmap_uncache(Fl_Image *base) { (void)base; }

static void copy_data(Fl_Pixmap *self) {
    char **new_data, **new_row;
    int i, ncolors, chars_per_pixel, chars_per_line;

    if (self->alloc_data) return;

    sscanf(self->image.data[0], "%*d%*d%d%d", &ncolors, &chars_per_pixel);
    chars_per_line = chars_per_pixel * self->image.w + 1;

    new_data = (char **)malloc(sizeof(char *) * (size_t)(ncolors < 0 ? self->image.h + 2 : self->image.h + ncolors + 1));
    new_data[0] = (char *)malloc(strlen(self->image.data[0]) + 1);
    strcpy(new_data[0], self->image.data[0]);

    if (ncolors < 0) {
        ncolors = -ncolors;
        new_row = new_data + 1;
        *new_row = (char *)malloc((size_t)ncolors * 4);
        memcpy(*new_row, self->image.data[1], (size_t)ncolors * 4);
        ncolors = 1;
        new_row++;
    } else {
        for (i = 0, new_row = new_data + 1; i < ncolors; i++, new_row++) {
            *new_row = (char *)malloc(strlen(self->image.data[i + 1]) + 1);
            strcpy(*new_row, self->image.data[i + 1]);
        }
    }

    for (i = 0; i < self->image.h; i++, new_row++) {
        *new_row = (char *)malloc((size_t)chars_per_line);
        memcpy(*new_row, self->image.data[i + ncolors + 1], (size_t)chars_per_line);
    }

    self->image.data = (const char *const *)new_data;
    self->image.count = self->image.h + ncolors + 1;
    self->alloc_data = 1;
}

/* Composites a fully-opaque-or-fully-transparent (alpha 0/255) RGBA
 * source region against the pixels currently on screen: a hard mask,
 * not a soft blend, matching the fact that fl_convert_pixmap() never
 * produces intermediate alpha values (see Fl_Pixmap.h). */
static void mask_composite(const uchar *src, int W, int H, int X, int Y) {
    uchar *dst = (uchar *)malloc((size_t)W * (size_t)H * 3);
    int x, y;
    const uchar *s = src;
    uchar *d;

    if (!dst) return;
    fl_read_image(dst, X, Y, W, H, 3);

    d = dst;
    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            if (s[3]) { d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; }
            s += 4;
            d += 3;
        }
    }

    fl_draw_image(dst, X, Y, W, H, 3, 0);
    free(dst);
}

static void pixmap_draw(Fl_Image *base, int XP, int YP, int WP, int HP, int cx, int cy) {
    Fl_Pixmap *self = (Fl_Pixmap *)base;
    int X, Y, W, H, w, h;
    uchar *buffer;

    if (self->image.w < 0) measure(self);
    if (!self->image.data) { Fl_Image_draw_empty(base, XP, YP); return; }

    if (WP == -1) { WP = self->image.w; HP = self->image.h; }
    if (!self->image.w) return;

    w = self->image.w; h = self->image.h;
    fl_clip_box(XP, YP, WP, HP, &X, &Y, &W, &H);
    cx += X - XP; cy += Y - YP;
    if (cx < 0) { W += cx; X -= cx; cx = 0; }
    if (cx + W > w) W = w - cx;
    if (W <= 0) return;
    if (cy < 0) { H += cy; Y -= cy; cy = 0; }
    if (cy + H > h) H = h - cy;
    if (H <= 0) return;

    buffer = (uchar *)malloc((size_t)w * (size_t)h * 4);
    if (!fl_convert_pixmap(self->image.data, buffer, FL_GRAY)) { free(buffer); return; }

    {
        const uchar *cropped = buffer + ((size_t)cy * (size_t)w + (size_t)cx) * 4;
        if (cx == 0 && W == w) {
            mask_composite(buffer + (size_t)cy * (size_t)w * 4, W, H, X, Y);
        } else {
            /* Non-full-width crop: repack rows into a contiguous WxH
             * buffer first since mask_composite()/fl_draw_image() take
             * no stride parameter for the RGBA source. */
            uchar *packed = (uchar *)malloc((size_t)W * (size_t)H * 4);
            int row;
            for (row = 0; row < H; row++)
                memcpy(packed + (size_t)row * (size_t)W * 4, cropped + (size_t)row * (size_t)w * 4, (size_t)W * 4);
            mask_composite(packed, W, H, X, Y);
            free(packed);
        }
    }

    free(buffer);
}

static Fl_Image *pixmap_copy(const Fl_Image *base, int W, int H) {
    const Fl_Pixmap *self = (const Fl_Pixmap *)base;
    Fl_Pixmap *new_image;
    char **new_data, **new_row, *new_ptr, new_info[255];
    const char *old_ptr;
    int i, c, sy, dx, dy, xerr, yerr, xmod, ymod, xstep, ystep;
    int ncolors, chars_per_pixel, chars_per_line;

    if (W == self->image.w && H == self->image.h) {
        new_image = Fl_Pixmap_new(self->image.data);
        copy_data(new_image);
        return &new_image->image;
    }
    if (W <= 0 || H <= 0) return NULL;

    sscanf(self->image.data[0], "%*d%*d%d%d", &ncolors, &chars_per_pixel);
    chars_per_line = chars_per_pixel * W + 1;
    snprintf(new_info, sizeof(new_info), "%d %d %d %d", W, H, ncolors, chars_per_pixel);

    xmod = self->image.w % W;
    xstep = (self->image.w / W) * chars_per_pixel;
    ymod = self->image.h % H;
    ystep = self->image.h / H;

    new_data = (char **)malloc(sizeof(char *) * (size_t)(ncolors < 0 ? H + 2 : H + ncolors + 1));
    new_data[0] = (char *)malloc(strlen(new_info) + 1);
    strcpy(new_data[0], new_info);

    if (ncolors < 0) {
        int n = -ncolors;
        new_row = new_data + 1;
        *new_row = (char *)malloc((size_t)n * 4);
        memcpy(*new_row, self->image.data[1], (size_t)n * 4);
        ncolors = 1; /* the compressed colormap occupies exactly 1 data() slot */
        new_row++;
    } else {
        for (i = 0, new_row = new_data + 1; i < ncolors; i++, new_row++) {
            *new_row = (char *)malloc(strlen(self->image.data[i + 1]) + 1);
            strcpy(*new_row, self->image.data[i + 1]);
        }
    }

    for (dy = H, sy = 0, yerr = H; dy > 0; dy--, new_row++) {
        *new_row = (char *)malloc((size_t)chars_per_line);
        new_ptr = *new_row;

        for (dx = W, xerr = W, old_ptr = self->image.data[sy + ncolors + 1];
             dx > 0; dx--) {
            for (c = 0; c < chars_per_pixel; c++) *new_ptr++ = old_ptr[c];
            old_ptr += xstep;
            xerr -= xmod;
            if (xerr <= 0) { xerr += W; old_ptr += chars_per_pixel; }
        }
        *new_ptr = '\0';

        sy += ystep;
        yerr -= ymod;
        if (yerr <= 0) { yerr += H; sy++; }
    }

    new_image = Fl_Pixmap_new((const char *const *)new_data);
    new_image->alloc_data = 1;
    return &new_image->image;
}

static void pixmap_color_average(Fl_Image *base, Fl_Color c, float i) {
    Fl_Pixmap *self = (Fl_Pixmap *)base;
    char line[255];
    int color, ncolors, chars_per_pixel;
    uchar r, g, b;
    unsigned ia, ir, ig, ib;

    Fl_Image_uncache(base);
    copy_data(self);

    fl_get_color_rgb(c, &r, &g, &b);
    if (i < 0.0f) i = 0.0f;
    else if (i > 1.0f) i = 1.0f;
    ia = (unsigned)(256 * i);
    ir = r * (256 - ia); ig = g * (256 - ia); ib = b * (256 - ia);

    sscanf(self->image.data[0], "%*d%*d%d%d", &ncolors, &chars_per_pixel);

    if (ncolors < 0) {
        uchar *cmap = (uchar *)(self->image.data[1]);
        ncolors = -ncolors;
        for (color = 0; color < ncolors; color++, cmap += 4) {
            cmap[1] = (uchar)((ia * cmap[1] + ir) >> 8);
            cmap[2] = (uchar)((ia * cmap[2] + ig) >> 8);
            cmap[3] = (uchar)((ia * cmap[3] + ib) >> 8);
        }
    } else {
        for (color = 0; color < ncolors; color++) {
            const char *p = find_color_word(self->image.data[color + 1] + chars_per_pixel + 1);
            if (fl_parse_color(p, &r, &g, &b)) {
                r = (uchar)((ia * r + ir) >> 8);
                g = (uchar)((ia * g + ig) >> 8);
                b = (uchar)((ia * b + ib) >> 8);
                if (chars_per_pixel > 1)
                    snprintf(line, sizeof(line), "%c%c c #%02X%02X%02X", self->image.data[color + 1][0], self->image.data[color + 1][1], r, g, b);
                else
                    snprintf(line, sizeof(line), "%c c #%02X%02X%02X", self->image.data[color + 1][0], r, g, b);
                free((void *)self->image.data[color + 1]);
                ((char **)self->image.data)[color + 1] = (char *)malloc(strlen(line) + 1);
                strcpy((char *)self->image.data[color + 1], line);
            }
        }
    }
}

static void pixmap_desaturate(Fl_Image *base) {
    Fl_Pixmap *self = (Fl_Pixmap *)base;
    char line[255];
    int i, ncolors, chars_per_pixel;
    uchar r, g, b;

    Fl_Image_uncache(base);
    copy_data(self);

    sscanf(self->image.data[0], "%*d%*d%d%d", &ncolors, &chars_per_pixel);

    if (ncolors < 0) {
        uchar *cmap = (uchar *)(self->image.data[1]);
        ncolors = -ncolors;
        for (i = 0; i < ncolors; i++, cmap += 4) {
            uchar gray = (uchar)((cmap[1] * 31 + cmap[2] * 61 + cmap[3] * 8) / 100);
            cmap[1] = cmap[2] = cmap[3] = gray;
        }
    } else {
        for (i = 0; i < ncolors; i++) {
            const char *p = find_color_word(self->image.data[i + 1] + chars_per_pixel + 1);
            if (fl_parse_color(p, &r, &g, &b)) {
                uchar gray = (uchar)((r * 31 + g * 61 + b * 8) / 100);
                if (chars_per_pixel > 1)
                    snprintf(line, sizeof(line), "%c%c c #%02X%02X%02X", self->image.data[i + 1][0], self->image.data[i + 1][1], gray, gray, gray);
                else
                    snprintf(line, sizeof(line), "%c c #%02X%02X%02X", self->image.data[i + 1][0], gray, gray, gray);
                free((void *)self->image.data[i + 1]);
                ((char **)self->image.data)[i + 1] = (char *)malloc(strlen(line) + 1);
                strcpy((char *)self->image.data[i + 1], line);
            }
        }
    }
}

const Fl_Image_Ops fl_pixmap_ops = {
    pixmap_draw, pixmap_copy, pixmap_color_average, pixmap_desaturate, pixmap_uncache, pixmap_destroy
};
