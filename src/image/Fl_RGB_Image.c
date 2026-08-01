/*
 * cfltk - Fl_RGB_Image.c
 * See include/cfltk/Fl_RGB_Image.h for the class-conversion notes.
 * Translated from the Fl_RGB_Image portion of src/Fl_Image.cxx.
 */
#include <stdlib.h>
#include <string.h>

#include "cfltk/Fl_RGB_Image.h"
#include "cfltk/fl_draw.h"
#include "cfltk/fl_colormap.h"

static size_t g_max_size = ~(size_t)0;

void Fl_RGB_Image_max_size(size_t size) { g_max_size = size; }
size_t Fl_RGB_Image_get_max_size(void) { return g_max_size; }

void Fl_RGB_Image_init(Fl_RGB_Image *self, const uchar *bits, int W, int H, int D, int LD) {
    Fl_Image_init(&self->image, W, H, D);
    self->image.ops = &fl_rgb_image_ops;
    self->array = bits;
    self->alloc_array = 0;
    self->image.data = (const char *const *)&self->array;
    self->image.count = 1;
    self->image.ld = LD;
}

Fl_RGB_Image *Fl_RGB_Image_new(const uchar *bits, int W, int H, int D, int LD) {
    Fl_RGB_Image *self = (Fl_RGB_Image *)malloc(sizeof(Fl_RGB_Image));
    Fl_RGB_Image_init(self, bits, W, H, D, LD);
    return self;
}

static void rgb_destroy(Fl_Image *base) {
    Fl_RGB_Image *self = (Fl_RGB_Image *)base;
    if (self->alloc_array) free((void *)self->array);
}

static void rgb_uncache(Fl_Image *base) { (void)base; }

/* Matches upstream's file-static start(): clips (XP,YP,WP,HP,cx,cy) to
 * the currently active clip region and to the image's own bounds,
 * producing the on-screen rectangle (X,Y,W,H) to actually blit and the
 * adjusted source offset (cx,cy). Returns non-zero if nothing is
 * visible. */
static int rgb_start(int XP, int YP, int WP, int HP, int w, int h, int *cx, int *cy,
                      int *X, int *Y, int *W, int *H) {
    fl_clip_box(XP, YP, WP, HP, X, Y, W, H);
    *cx += *X - XP;
    *cy += *Y - YP;
    if (*cx < 0) { *W += *cx; *X -= *cx; *cx = 0; }
    if (*cx + *W > w) *W = w - *cx;
    if (*W <= 0) return 1;
    if (*cy < 0) { *H += *cy; *Y -= *cy; *cy = 0; }
    if (*cy + *H > h) *H = h - *cy;
    if (*H <= 0) return 1;
    return 0;
}

/* Software alpha composite of a 2- (gray+alpha) or 4- (RGBA) channel
 * source region against the pixels currently on screen, matching
 * upstream's non-WIN32/non-Apple alpha_blend() fallback exactly (the
 * path any Xlib build takes when accelerated alpha isn't available --
 * cfltk always takes it, see header). */
static void rgb_alpha_blend(const Fl_RGB_Image *img, int X, int Y, int W, int H, int cx, int cy) {
    int ld = img->image.ld ? img->image.ld : img->image.w * img->image.d;
    const uchar *srcptr = img->array + (size_t)cy * (size_t)ld + (size_t)cx * (size_t)img->image.d;
    int srcskip = ld - img->image.d * W;
    uchar *dst = (uchar *)malloc((size_t)W * (size_t)H * 3);
    uchar *dstptr = dst;
    int x, y;

    if (!dst) return;
    fl_read_image(dst, X, Y, W, H, 3);

    if (img->image.d == 2) {
        for (y = H; y > 0; y--, srcptr += srcskip) {
            for (x = W; x > 0; x--) {
                uchar srcg = *srcptr++;
                uchar srca = *srcptr++;
                uchar dstr = dstptr[0], dstg = dstptr[1], dstb = dstptr[2];
                uchar dsta = (uchar)(255 - srca);
                *dstptr++ = (uchar)(((unsigned)srcg * srca + (unsigned)dstr * dsta) >> 8);
                *dstptr++ = (uchar)(((unsigned)srcg * srca + (unsigned)dstg * dsta) >> 8);
                *dstptr++ = (uchar)(((unsigned)srcg * srca + (unsigned)dstb * dsta) >> 8);
            }
        }
    } else {
        for (y = H; y > 0; y--, srcptr += srcskip) {
            for (x = W; x > 0; x--) {
                uchar srcr = *srcptr++, srcg = *srcptr++, srcb = *srcptr++, srca = *srcptr++;
                uchar dstr = dstptr[0], dstg = dstptr[1], dstb = dstptr[2];
                uchar dsta = (uchar)(255 - srca);
                *dstptr++ = (uchar)(((unsigned)srcr * srca + (unsigned)dstr * dsta) >> 8);
                *dstptr++ = (uchar)(((unsigned)srcg * srca + (unsigned)dstg * dsta) >> 8);
                *dstptr++ = (uchar)(((unsigned)srcb * srca + (unsigned)dstb * dsta) >> 8);
            }
        }
    }

    fl_draw_image(dst, X, Y, W, H, 3, 0);
    free(dst);
}

static void rgb_draw(Fl_Image *base, int XP, int YP, int WP, int HP, int cx, int cy) {
    Fl_RGB_Image *img = (Fl_RGB_Image *)base;
    int X, Y, W, H, ld;

    if (!img->image.d || !img->array) {
        Fl_Image_draw_empty(base, XP, YP);
        return;
    }
    if (rgb_start(XP, YP, WP, HP, img->image.w, img->image.h, &cx, &cy, &X, &Y, &W, &H)) return;

    ld = img->image.ld ? img->image.ld : img->image.w * img->image.d;

    if (img->image.d == 1 || img->image.d == 3) {
        const uchar *src = img->array + (size_t)cy * (size_t)ld + (size_t)cx * (size_t)img->image.d;
        fl_draw_image(src, X, Y, W, H, img->image.d, ld);
    } else {
        rgb_alpha_blend(img, X, Y, W, H, cx, cy);
    }
}

/* Same-size (or empty) copy: a plain duplicate of the pixel array,
 * de-striding it if `ld` padding is present. Split out from rgb_copy()
 * so its early returns don't share a declaration scope with the
 * resize path below (GCC's -Wmaybe-uninitialized otherwise flags a
 * false positive on the shared "new_array" spanning both paths). */
static Fl_Image *rgb_copy_same_size(const Fl_RGB_Image *self) {
    Fl_RGB_Image *new_image;
    uchar *new_array;

    if (!self->array) return &Fl_RGB_Image_new(self->array, self->image.w, self->image.h, self->image.d, self->image.ld)->image;

    {
        size_t sz = (size_t)self->image.w * (size_t)self->image.h * (size_t)self->image.d;
        new_array = (uchar *)malloc(sz);
        if (self->image.ld && self->image.ld != self->image.w * self->image.d) {
            const uchar *src = self->array;
            uchar *dst = new_array;
            int dy, dh = self->image.h, wd = self->image.w * self->image.d, wld = self->image.ld;
            for (dy = 0; dy < dh; dy++) { memcpy(dst, src, (size_t)wd); src += wld; dst += wd; }
        } else {
            memcpy(new_array, self->array, sz);
        }
    }
    new_image = Fl_RGB_Image_new(new_array, self->image.w, self->image.h, self->image.d, 0);
    new_image->alloc_array = 1;
    return &new_image->image;
}

static Fl_Image *rgb_copy(const Fl_Image *base, int W, int H) {
    const Fl_RGB_Image *self = (const Fl_RGB_Image *)base;
    Fl_RGB_Image *new_image;
    uchar *new_array;
    int line_d;

    if ((W == self->image.w && H == self->image.h) || !self->image.w || !self->image.h || !self->image.d || !self->array)
        return rgb_copy_same_size(self);
    if (W <= 0 || H <= 0) return NULL;

    new_array = (uchar *)malloc((size_t)W * (size_t)H * (size_t)self->image.d);
    /* GCC 13 -Wmaybe-uninitialized false positive: new_array is
     * unconditionally assigned by the malloc() on the line directly
     * above before this use; there is no path that reaches here
     * without it having been set. */
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
    new_image = Fl_RGB_Image_new(new_array, W, H, self->image.d, 0);
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
    new_image->alloc_array = 1;

    line_d = self->image.ld ? self->image.ld : self->image.w * self->image.d;

    if (Fl_Image_RGB_scaling() == FL_RGB_SCALING_NEAREST) {
        int c, sy, xerr, yerr, xmod, ymod, xstep, ystep, dx, dy;
        uchar *new_ptr;
        const uchar *old_ptr;

        xmod = self->image.w % W;
        xstep = (self->image.w / W) * self->image.d;
        ymod = self->image.h % H;
        ystep = self->image.h / H;

        for (dy = H, sy = 0, yerr = H, new_ptr = new_array; dy > 0; dy--) {
            for (dx = W, xerr = W, old_ptr = self->array + (size_t)sy * (size_t)line_d; dx > 0; dx--) {
                for (c = 0; c < self->image.d; c++) *new_ptr++ = old_ptr[c];
                old_ptr += xstep;
                xerr -= xmod;
                if (xerr <= 0) { xerr += W; old_ptr += self->image.d; }
            }
            sy += ystep;
            yerr -= ymod;
            if (yerr <= 0) { yerr += H; sy++; }
        }
    } else {
        int d = self->image.d;
        int dx, dy, i;
        float xscale = (self->image.w - 1) / (float)W;
        float yscale = (self->image.h - 1) / (float)H;

        for (dy = 0; dy < H; dy++) {
            float oldy = dy * yscale;
            float yfract;
            unsigned lefty, dlefty;
            if (oldy >= self->image.h) oldy = (float)(self->image.h - 1);
            yfract = oldy - (unsigned)oldy;
            lefty = (unsigned)oldy;
            dlefty = (unsigned)(oldy + 1 >= self->image.h ? oldy : oldy + 1);

            for (dx = 0; dx < W; dx++) {
                float oldx = dx * xscale;
                float xfract;
                unsigned leftx, rightx, righty, dleftx, drightx, drighty;
                uchar left[4], right[4], downleft[4], downright[4];
                float leftf, rightf, upf, downf;
                uchar *new_ptr = new_array + (size_t)dy * (size_t)W * (size_t)d + (size_t)dx * (size_t)d;

                if (oldx >= self->image.w) oldx = (float)(self->image.w - 1);
                xfract = oldx - (unsigned)oldx;
                leftx = (unsigned)oldx;
                rightx = (unsigned)(oldx + 1 >= self->image.w ? oldx : oldx + 1);
                righty = lefty;
                dleftx = leftx;
                drightx = rightx;
                drighty = dlefty;

                memcpy(left, self->array + (size_t)lefty * (size_t)line_d + (size_t)leftx * (size_t)d, (size_t)d);
                memcpy(right, self->array + (size_t)righty * (size_t)line_d + (size_t)rightx * (size_t)d, (size_t)d);
                memcpy(downleft, self->array + (size_t)dlefty * (size_t)line_d + (size_t)dleftx * (size_t)d, (size_t)d);
                memcpy(downright, self->array + (size_t)drighty * (size_t)line_d + (size_t)drightx * (size_t)d, (size_t)d);

                if (d == 4) {
                    for (i = 0; i < 3; i++) {
                        left[i] = (uchar)(left[i] * left[3] / 255.0f);
                        right[i] = (uchar)(right[i] * right[3] / 255.0f);
                        downleft[i] = (uchar)(downleft[i] * downleft[3] / 255.0f);
                        downright[i] = (uchar)(downright[i] * downright[3] / 255.0f);
                    }
                }

                leftf = 1 - xfract; rightf = xfract; upf = 1 - yfract; downf = yfract;
                for (i = 0; i < d; i++) {
                    new_ptr[i] = (uchar)((left[i] * leftf + right[i] * rightf) * upf +
                                          (downleft[i] * leftf + downright[i] * rightf) * downf);
                }
                if (d == 4 && new_ptr[3]) {
                    for (i = 0; i < 3; i++) new_ptr[i] = (uchar)(new_ptr[i] / (new_ptr[3] / 255.0f));
                }
            }
        }
    }

    return &new_image->image;
}

static void rgb_color_average(Fl_Image *base, Fl_Color c, float i) {
    Fl_RGB_Image *self = (Fl_RGB_Image *)base;
    uchar *new_array, *new_ptr;
    const uchar *old_ptr;
    uchar r, g, b;
    unsigned ia, ir, ig, ib;
    int x, y, line_i;

    if (!self->image.w || !self->image.h || !self->image.d || !self->array) return;
    Fl_Image_uncache(base);

    if (!self->alloc_array) new_array = (uchar *)malloc((size_t)self->image.h * (size_t)self->image.w * (size_t)self->image.d);
    else new_array = (uchar *)self->array;

    fl_get_color_rgb(c, &r, &g, &b);
    if (i < 0.0f) i = 0.0f;
    else if (i > 1.0f) i = 1.0f;

    ia = (unsigned)(256 * i);
    ir = r * (256 - ia);
    ig = g * (256 - ia);
    ib = b * (256 - ia);

    line_i = self->image.ld ? self->image.ld - (self->image.w * self->image.d) : 0;

    if (self->image.d < 3) {
        ig = (r * 31 + g * 61 + b * 8) / 100 * (256 - ia);
        for (new_ptr = new_array, old_ptr = self->array, y = 0; y < self->image.h; y++, old_ptr += line_i)
            for (x = 0; x < self->image.w; x++) {
                *new_ptr++ = (uchar)((*old_ptr++ * ia + ig) >> 8);
                if (self->image.d > 1) *new_ptr++ = *old_ptr++;
            }
    } else {
        for (new_ptr = new_array, old_ptr = self->array, y = 0; y < self->image.h; y++, old_ptr += line_i)
            for (x = 0; x < self->image.w; x++) {
                *new_ptr++ = (uchar)((*old_ptr++ * ia + ir) >> 8);
                *new_ptr++ = (uchar)((*old_ptr++ * ia + ig) >> 8);
                *new_ptr++ = (uchar)((*old_ptr++ * ia + ib) >> 8);
                if (self->image.d > 3) *new_ptr++ = *old_ptr++;
            }
    }

    if (!self->alloc_array) {
        self->array = new_array;
        self->alloc_array = 1;
        self->image.ld = 0;
    }
}

static void rgb_desaturate(Fl_Image *base) {
    Fl_RGB_Image *self = (Fl_RGB_Image *)base;
    uchar *new_array, *new_ptr;
    const uchar *old_ptr;
    int new_d, x, y, line_i;

    if (!self->image.w || !self->image.h || !self->image.d || !self->array) return;
    if (self->image.d < 3) return;
    Fl_Image_uncache(base);

    new_d = self->image.d - 2;
    new_array = (uchar *)malloc((size_t)self->image.h * (size_t)self->image.w * (size_t)new_d);

    line_i = self->image.ld ? self->image.ld - (self->image.w * self->image.d) : 0;

    for (new_ptr = new_array, old_ptr = self->array, y = 0; y < self->image.h; y++, old_ptr += line_i) {
        for (x = 0; x < self->image.w; x++, old_ptr += self->image.d) {
            *new_ptr++ = (uchar)((31 * old_ptr[0] + 61 * old_ptr[1] + 8 * old_ptr[2]) / 100);
            if (self->image.d > 3) *new_ptr++ = old_ptr[3];
        }
    }

    if (self->alloc_array) free((void *)self->array);
    self->array = new_array;
    self->alloc_array = 1;
    self->image.ld = 0;
    self->image.d = new_d;
}

const Fl_Image_Ops fl_rgb_image_ops = {
    rgb_draw, rgb_copy, rgb_color_average, rgb_desaturate, rgb_uncache, rgb_destroy
};
