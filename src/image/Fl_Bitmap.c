/*
 * cfltk - Fl_Bitmap.c
 * See include/cfltk/Fl_Bitmap.h for the class-conversion notes.
 * Translated from src/Fl_Bitmap.cxx.
 */
#include <stdlib.h>
#include <string.h>

#include "cfltk/Fl_Bitmap.h"
#include "cfltk/fl_draw.h"

void Fl_Bitmap_init(Fl_Bitmap *self, const uchar *bits, int W, int H) {
    Fl_Image_init(&self->image, W, H, 0);
    self->image.ops = &fl_bitmap_ops;
    self->array = bits;
    self->alloc_array = 0;
    self->image.data = (const char *const *)&self->array;
    self->image.count = 1;
}

Fl_Bitmap *Fl_Bitmap_new(const uchar *bits, int W, int H) {
    Fl_Bitmap *self = (Fl_Bitmap *)malloc(sizeof(Fl_Bitmap));
    Fl_Bitmap_init(self, bits, W, H);
    return self;
}

static void bitmap_destroy(Fl_Image *base) {
    Fl_Bitmap *self = (Fl_Bitmap *)base;
    if (self->alloc_array) free((void *)self->array);
}

static void bitmap_uncache(Fl_Image *base) { (void)base; }

/* Fl_Bitmap doesn't override color_average()/desaturate() upstream
 * either -- they inherit Fl_Image's no-op base bodies verbatim. */
static void bitmap_color_average(Fl_Image *base, Fl_Color c, float i) { (void)base; (void)c; (void)i; }
static void bitmap_desaturate(Fl_Image *base) { (void)base; }

static void bitmap_draw(Fl_Image *base, int XP, int YP, int WP, int HP, int cx, int cy) {
    Fl_Bitmap *self = (Fl_Bitmap *)base;
    int X, Y, W, H;

    if (!self->array) { Fl_Image_draw_empty(base, XP, YP); return; }

    fl_clip_box(XP, YP, WP, HP, &X, &Y, &W, &H);
    cx += X - XP; cy += Y - YP;
    if (cx < 0) { W += cx; X -= cx; cx = 0; }
    if (cx + W > self->image.w) W = self->image.w - cx;
    if (W <= 0) return;
    if (cy < 0) { H += cy; Y -= cy; cy = 0; }
    if (cy + H > self->image.h) H = self->image.h - cy;
    if (H <= 0) return;

    fl_draw_bitmask(self->array, self->image.w, self->image.h, cx, cy, X, Y, W, H);
}

static Fl_Image *bitmap_copy(const Fl_Image *base, int W, int H) {
    const Fl_Bitmap *self = (const Fl_Bitmap *)base;
    Fl_Bitmap *new_image;
    uchar *new_array;
    size_t row_bytes;

    if (W == self->image.w && H == self->image.h) {
        row_bytes = (size_t)((W + 7) / 8);
        new_array = (uchar *)malloc(row_bytes * (size_t)H);
        memcpy(new_array, self->array, row_bytes * (size_t)H);
        new_image = Fl_Bitmap_new(new_array, W, H);
        new_image->alloc_array = 1;
        return &new_image->image;
    }
    if (W <= 0 || H <= 0) return NULL;

    {
        int sx, sy, dx, dy, xerr, yerr, xmod, ymod, xstep, ystep;
        uchar *new_ptr, new_bit, old_bit;
        const uchar *old_ptr;

        xmod = self->image.w % W;
        xstep = self->image.w / W;
        ymod = self->image.h % H;
        ystep = self->image.h / H;

        row_bytes = (size_t)((W + 7) / 8);
        new_array = (uchar *)malloc(row_bytes * (size_t)H);
        memset(new_array, 0, row_bytes * (size_t)H);
        new_image = Fl_Bitmap_new(new_array, W, H);
        new_image->alloc_array = 1;

        for (dy = H, sy = 0, yerr = H, new_ptr = new_array; dy > 0; dy--) {
            for (dx = W, xerr = W, old_ptr = self->array + (size_t)sy * (size_t)((self->image.w + 7) / 8), sx = 0, new_bit = 1;
                 dx > 0; dx--) {
                old_bit = (uchar)(1 << (sx & 7));
                if (old_ptr[sx / 8] & old_bit) *new_ptr |= new_bit;

                if (new_bit < 128) new_bit = (uchar)(new_bit << 1);
                else { new_bit = 1; new_ptr++; }

                sx += xstep;
                xerr -= xmod;
                if (xerr <= 0) { xerr += W; sx++; }
            }
            if (new_bit > 1) new_ptr++;

            sy += ystep;
            yerr -= ymod;
            if (yerr <= 0) { yerr += H; sy++; }
        }
    }

    return &new_image->image;
}

const Fl_Image_Ops fl_bitmap_ops = {
    bitmap_draw, bitmap_copy, bitmap_color_average, bitmap_desaturate, bitmap_uncache, bitmap_destroy
};
