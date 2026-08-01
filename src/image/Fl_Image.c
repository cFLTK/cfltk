/*
 * cfltk - Fl_Image.c
 * See include/cfltk/Fl_Image.h for the class-conversion notes.
 * Translated from the Fl_Image portion of src/Fl_Image.cxx.
 */
#include <stdlib.h>

#include "cfltk/Fl_Image.h"
#include "cfltk/fl_draw.h"

static int g_rgb_scaling = FL_RGB_SCALING_NEAREST;

void Fl_Image_draw_empty(const Fl_Image *self, int X, int Y) {
    if (self->w > 0 && self->h > 0) {
        fl_color(FL_FOREGROUND_COLOR);
        fl_rect(X, Y, self->w, self->h);
        fl_line(X, Y, X + self->w - 1, Y + self->h - 1);
        fl_line(X, Y + self->h - 1, X + self->w - 1, Y);
    }
}

static void base_draw(Fl_Image *self, int X, int Y, int W, int H, int cx, int cy) {
    (void)W; (void)H; (void)cx; (void)cy;
    Fl_Image_draw_empty(self, X, Y);
}

static Fl_Image *base_copy(const Fl_Image *self, int W, int H) {
    return Fl_Image_new(W, H, self->d);
}

static void base_color_average(Fl_Image *self, Fl_Color c, float i) { (void)self; (void)c; (void)i; }
static void base_desaturate(Fl_Image *self) { (void)self; }
static void base_uncache(Fl_Image *self) { (void)self; }
static void base_destroy(Fl_Image *self) { (void)self; }

const Fl_Image_Ops fl_image_ops = {
    base_draw, base_copy, base_color_average, base_desaturate, base_uncache, base_destroy
};

void Fl_Image_init(Fl_Image *self, int W, int H, int D) {
    self->ops = &fl_image_ops;
    self->w = W;
    self->h = H;
    self->d = D;
    self->ld = 0;
    self->count = 0;
    self->data = NULL;
}

Fl_Image *Fl_Image_new(int W, int H, int D) {
    Fl_Image *self = (Fl_Image *)malloc(sizeof(Fl_Image));
    Fl_Image_init(self, W, H, D);
    return self;
}

void Fl_Image_destroy(Fl_Image *self) { self->ops->destroy(self); }

void Fl_Image_delete(Fl_Image *self) {
    Fl_Image_destroy(self);
    free(self);
}

int Fl_Image_fail(const Fl_Image *self) {
    if (self->w <= 0 || self->h <= 0 || self->d <= 0) {
        if (self->ld == 0) return FL_IMAGE_ERR_NO_IMAGE;
        return self->ld;
    }
    return 0;
}

void Fl_Image_set_RGB_scaling(int method) { g_rgb_scaling = method; }
int Fl_Image_RGB_scaling(void) { return g_rgb_scaling; }
