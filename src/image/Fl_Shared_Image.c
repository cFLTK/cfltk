/*
 * cfltk - Fl_Shared_Image.c
 * See include/cfltk/Fl_Shared_Image.h for the class-conversion notes.
 * Translated from src/Fl_Shared_Image.cxx (pre-1.3.4-ABI draw() path)
 * and src/fl_images_core.cxx (fl_register_images()/fl_check_images()).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cfltk/Fl_Shared_Image.h"
#include "cfltk/Fl_BMP_Image.h"
#include "cfltk/Fl_GIF_Image.h"
#ifdef CFLTK_HAVE_PNG
#include "cfltk/Fl_PNG_Image.h"
#endif
#ifdef CFLTK_HAVE_JPEG
#include "cfltk/Fl_JPEG_Image.h"
#endif

static Fl_Shared_Image **g_images = NULL;
static int g_num_images = 0;
static int g_alloc_images = 0;

static Fl_Shared_Handler *g_handlers = NULL;
static int g_num_handlers = 0;
static int g_alloc_handlers = 0;

static int shared_compare(const void *a, const void *b) {
    Fl_Shared_Image *i0 = *(Fl_Shared_Image *const *)a;
    Fl_Shared_Image *i1 = *(Fl_Shared_Image *const *)b;
    int c = strcmp(i0->name, i1->name);
    if (c) return c;
    if (i0->image.w != i1->image.w) return i0->image.w - i1->image.w;
    return i0->image.h - i1->image.h;
}

static void shared_init_bare(Fl_Shared_Image *self) {
    Fl_Image_init(&self->image, 0, 0, 0);
    self->image.ops = &fl_shared_image_ops;
    self->name = NULL;
    self->refcount = 1;
    self->original = 0;
    self->wrapped = NULL;
    self->alloc_image = 0;
}

static void shared_update(Fl_Shared_Image *self) {
    if (self->wrapped) {
        self->image.w = self->wrapped->w;
        self->image.h = self->wrapped->h;
        self->image.d = self->wrapped->d;
        self->image.data = self->wrapped->data;
        self->image.count = self->wrapped->count;
    }
}

static void shared_add(Fl_Shared_Image *self) {
    if (g_num_images >= g_alloc_images) {
        Fl_Shared_Image **temp = (Fl_Shared_Image **)malloc(sizeof(Fl_Shared_Image *) * (size_t)(g_alloc_images + 32));
        if (g_alloc_images) {
            memcpy(temp, g_images, sizeof(Fl_Shared_Image *) * (size_t)g_alloc_images);
            free(g_images);
        }
        g_images = temp;
        g_alloc_images += 32;
    }
    g_images[g_num_images++] = self;
    if (g_num_images > 1) qsort(g_images, (size_t)g_num_images, sizeof(Fl_Shared_Image *), shared_compare);
}

void Fl_Shared_Image_reload(Fl_Shared_Image *self) {
    FILE *fp;
    uchar header[64];
    Fl_Image *img = NULL;
    int i;

    if (!self->name) return;
    fp = fopen(self->name, "rb");
    if (!fp) return;
    if (fread(header, 1, sizeof(header), fp) == 0) { /* ignore; handlers re-check length implicitly */ }
    fclose(fp);

    for (i = 0; i < g_num_handlers && !img; i++) img = g_handlers[i](self->name, header, sizeof(header));

    if (img) {
        /* self->wrapped is NULL the first time reload() runs (right
         * after shared_init_named() set alloc_image=1 in anticipation
         * of this call) -- unlike C++'s "delete nullptr", which
         * upstream relies on here, Fl_Image_delete() is not
         * null-safe, so this needs an explicit guard. */
        if (self->alloc_image && self->wrapped) Fl_Image_delete(self->wrapped);
        self->alloc_image = 1;
        self->wrapped = img;
        shared_update(self);
    }
}

static void shared_init_named(Fl_Shared_Image *self, const char *n, Fl_Image *img) {
    size_t len;
    shared_init_bare(self);
    len = strlen(n) + 1;
    self->name = (const char *)malloc(len);
    memcpy((void *)self->name, n, len);
    self->wrapped = img;
    self->alloc_image = img ? 0 : 1;
    self->original = 1;
    if (!img) Fl_Shared_Image_reload(self);
    else shared_update(self);
}

static void shared_destroy(Fl_Image *base) {
    Fl_Shared_Image *self = (Fl_Shared_Image *)base;
    if (self->name) free((void *)self->name);
    if (self->alloc_image && self->wrapped) Fl_Image_delete(self->wrapped);
}

void Fl_Shared_Image_release(Fl_Shared_Image *self) {
    int i;
    Fl_Shared_Image *the_original = NULL;

    if (self->refcount <= 0) return;
    self->refcount--;
    if (self->refcount > 0) return;

    if (!self->original) {
        Fl_Shared_Image *o = Fl_Shared_Image_find(self->name, 0, 0);
        if (o) {
            if (o->original && o != self && o->refcount > 1) the_original = o;
            Fl_Shared_Image_release(o);
        }
    }

    for (i = 0; i < g_num_images; i++) {
        if (g_images[i] == self) {
            g_num_images--;
            if (i < g_num_images) memmove(g_images + i, g_images + i + 1, sizeof(Fl_Shared_Image *) * (size_t)(g_num_images - i));
            break;
        }
    }

    Fl_Image_delete(&self->image);

    if (g_num_images == 0 && g_images) {
        free(g_images);
        g_images = NULL;
        g_alloc_images = 0;
    }

    if (the_original) Fl_Shared_Image_release(the_original);
}

static Fl_Image *shared_copy(const Fl_Image *base, int W, int H) {
    const Fl_Shared_Image *self = (const Fl_Shared_Image *)base;
    Fl_Shared_Image *out;
    size_t len;

    out = (Fl_Shared_Image *)malloc(sizeof(Fl_Shared_Image));
    shared_init_bare(out);

    len = strlen(self->name) + 1;
    out->name = (const char *)malloc(len);
    memcpy((void *)out->name, self->name, len);

    out->wrapped = self->wrapped ? Fl_Image_copy_sized(self->wrapped, W, H) : NULL;
    out->alloc_image = 1;
    shared_update(out);
    return &out->image;
}

static void shared_color_average(Fl_Image *base, Fl_Color c, float i) {
    Fl_Shared_Image *self = (Fl_Shared_Image *)base;
    if (!self->wrapped) return;
    Fl_Image_color_average(self->wrapped, c, i);
    shared_update(self);
}

static void shared_desaturate(Fl_Image *base) {
    Fl_Shared_Image *self = (Fl_Shared_Image *)base;
    if (!self->wrapped) return;
    Fl_Image_desaturate(self->wrapped);
    shared_update(self);
}

static void shared_uncache(Fl_Image *base) {
    Fl_Shared_Image *self = (Fl_Shared_Image *)base;
    if (self->wrapped) Fl_Image_uncache(self->wrapped);
}

static void shared_draw(Fl_Image *base, int X, int Y, int W, int H, int cx, int cy) {
    Fl_Shared_Image *self = (Fl_Shared_Image *)base;
    if (self->wrapped) Fl_Image_draw(self->wrapped, X, Y, W, H, cx, cy);
    else Fl_Image_draw_empty(base, X, Y);
}

const Fl_Image_Ops fl_shared_image_ops = {
    shared_draw, shared_copy, shared_color_average, shared_desaturate, shared_uncache, shared_destroy
};

Fl_Shared_Image *Fl_Shared_Image_find(const char *name, int W, int H) {
    if (!g_num_images) return NULL;

    if (W) {
        Fl_Shared_Image key;
        Fl_Shared_Image *keyp = &key;
        Fl_Shared_Image **match;

        shared_init_bare(&key);
        key.name = name;
        key.image.w = W;
        key.image.h = H;

        match = (Fl_Shared_Image **)bsearch(&keyp, g_images, (size_t)g_num_images, sizeof(Fl_Shared_Image *), shared_compare);
        if (match) { (*match)->refcount++; return *match; }
        return NULL;
    }

    {
        int i;
        for (i = 0; i < g_num_images; i++) {
            Fl_Shared_Image *img = g_images[i];
            if (img->original && img->name && strcmp(img->name, name) == 0) { img->refcount++; return img; }
        }
    }
    return NULL;
}

Fl_Shared_Image *Fl_Shared_Image_get(const char *name, int W, int H) {
    Fl_Shared_Image *temp;
    int temp_referenced = 0;

    temp = Fl_Shared_Image_find(name, W, H);
    if (temp) return temp;

    temp = Fl_Shared_Image_find(name, 0, 0);
    if (temp) {
        temp_referenced = 1;
    } else {
        temp = (Fl_Shared_Image *)malloc(sizeof(Fl_Shared_Image));
        shared_init_named(temp, name, NULL);
        if (!temp->wrapped) { Fl_Image_delete(&temp->image); return NULL; }
        shared_add(temp);
    }

    if ((temp->image.w != W || temp->image.h != H) && W && H) {
        Fl_Shared_Image *new_temp = (Fl_Shared_Image *)Fl_Image_copy_sized(&temp->image, W, H);
        if (!new_temp) return NULL;
        if (!temp_referenced) temp->refcount++;
        shared_add(new_temp);
        return new_temp;
    }

    return temp;
}

Fl_Shared_Image *Fl_Shared_Image_get_from_rgb(Fl_RGB_Image *rgb, int own_it) {
    static unsigned long counter = 0;
    char namebuf[32];
    Fl_Shared_Image *self = (Fl_Shared_Image *)malloc(sizeof(Fl_Shared_Image));

    snprintf(namebuf, sizeof(namebuf), "<rgb:%lu>", counter++);
    shared_init_named(self, namebuf, &rgb->image);
    self->alloc_image = own_it;
    shared_add(self);
    return self;
}

Fl_Shared_Image **Fl_Shared_Image_images(void) { return g_images; }
int Fl_Shared_Image_num_images(void) { return g_num_images; }

void Fl_Shared_Image_add_handler(Fl_Shared_Handler f) {
    int i;
    for (i = 0; i < g_num_handlers; i++) if (g_handlers[i] == f) return;

    if (g_num_handlers >= g_alloc_handlers) {
        Fl_Shared_Handler *temp = (Fl_Shared_Handler *)malloc(sizeof(Fl_Shared_Handler) * (size_t)(g_alloc_handlers + 32));
        if (g_alloc_handlers) {
            memcpy(temp, g_handlers, sizeof(Fl_Shared_Handler) * (size_t)g_alloc_handlers);
            free(g_handlers);
        }
        g_handlers = temp;
        g_alloc_handlers += 32;
    }
    g_handlers[g_num_handlers++] = f;
}

void Fl_Shared_Image_remove_handler(Fl_Shared_Handler f) {
    int i;
    for (i = 0; i < g_num_handlers; i++) if (g_handlers[i] == f) break;
    if (i >= g_num_handlers) return;
    g_num_handlers--;
    if (i < g_num_handlers) memmove(g_handlers + i, g_handlers + i + 1, sizeof(Fl_Shared_Handler) * (size_t)(g_num_handlers - i));
}

/* ---------------------------------------------------------------------
 * fl_register_images()/fl_check_images() (from fl_images_core.cxx)
 * ------------------------------------------------------------------ */

static Fl_Image *fl_check_images(const char *name, uchar *header, int headerlen) {
    (void)headerlen;

    if (memcmp(header, "GIF87a", 6) == 0 || memcmp(header, "GIF89a", 6) == 0)
        return &Fl_GIF_Image_new(name)->pixmap.image;

    if (memcmp(header, "BM", 2) == 0)
        return &Fl_BMP_Image_new(name)->rgb.image;

#ifdef CFLTK_HAVE_PNG
    if (memcmp(header, "\211PNG", 4) == 0)
        return &Fl_PNG_Image_new(name)->rgb.image;
#endif

#ifdef CFLTK_HAVE_JPEG
    if (memcmp(header, "\377\330\377", 3) == 0 && header[3] >= 0xc0 && header[3] <= 0xfe)
        return &Fl_JPEG_Image_new(name)->rgb.image;
#endif

    return NULL;
}

static int g_images_registered = 0;

void fl_register_images(void) {
    if (g_images_registered) return;
    g_images_registered = 1;
    Fl_Shared_Image_add_handler(fl_check_images);
}
