/*
 * cfltk - Fl_PNG_Image.c
 * See include/cfltk/Fl_PNG_Image.h for the class-conversion notes.
 * Translated from src/Fl_PNG_Image.cxx (HAVE_LIBPNG && HAVE_LIBZ path;
 * only compiled when CFLTK_ENABLE_PNG is on, see CMakeLists.txt).
 */
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <png.h>

#include "cfltk/Fl_PNG_Image.h"

typedef struct {
    png_structp pp;
    const unsigned char *current;
    const unsigned char *last;
} fl_png_memory;

static void png_read_data_from_mem(png_structp png_ptr, png_bytep data, png_size_t length) {
    fl_png_memory *m = (fl_png_memory *)png_get_io_ptr(png_ptr);
    if (m->current + length > m->last) {
        png_error(m->pp, "Invalid attempt to read row data");
        return;
    }
    memcpy(data, m->current, length);
    m->current += length;
}

static void load_png(Fl_PNG_Image *self, const char *name_png, const unsigned char *buffer_png, int maxsize) {
    int i, channels;
    FILE *fp = NULL;
    png_structp pp;
    png_infop info = NULL;
    png_bytep *rows;
    fl_png_memory png_mem_data;
    int from_memory = buffer_png != NULL;
    const char *display_name = name_png ? name_png : "in-memory PNG data";
    int num_trans;

    Fl_RGB_Image_init(&self->rgb, NULL, 0, 0, 0, 0);

    if (!from_memory) {
        fp = fopen(name_png, "rb");
        if (!fp) { self->rgb.image.ld = FL_IMAGE_ERR_FILE_ACCESS; return; }
    }

    pp = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (pp) info = png_create_info_struct(pp);
    if (!pp || !info) {
        if (pp) png_destroy_read_struct(&pp, NULL, NULL);
        if (!from_memory) fclose(fp);
        fprintf(stderr, "cfltk: cannot allocate memory to read PNG \"%s\"\n", display_name);
        self->rgb.image.w = 0; self->rgb.image.h = 0; self->rgb.image.d = 0;
        self->rgb.image.ld = FL_IMAGE_ERR_FORMAT;
        return;
    }

    if (setjmp(png_jmpbuf(pp))) {
        png_destroy_read_struct(&pp, &info, NULL);
        if (!from_memory) fclose(fp);
        fprintf(stderr, "cfltk: PNG \"%s\" is too large or contains errors\n", display_name);
        self->rgb.image.w = 0; self->rgb.image.h = 0; self->rgb.image.d = 0;
        self->rgb.image.ld = FL_IMAGE_ERR_FORMAT;
        return;
    }

    if (from_memory) {
        png_mem_data.current = buffer_png;
        png_mem_data.last = buffer_png + maxsize;
        png_mem_data.pp = pp;
        png_set_read_fn(pp, (png_voidp)&png_mem_data, png_read_data_from_mem);
    } else {
        png_init_io(pp, fp);
    }

    png_read_info(pp, info);

    if (png_get_color_type(pp, info) == PNG_COLOR_TYPE_PALETTE) png_set_expand(pp);

    channels = (png_get_color_type(pp, info) & PNG_COLOR_MASK_COLOR) ? 3 : 1;

    num_trans = 0;
    png_get_tRNS(pp, info, NULL, &num_trans, NULL);
    if ((png_get_color_type(pp, info) & PNG_COLOR_MASK_ALPHA) || num_trans != 0) channels++;

    self->rgb.image.w = (int)png_get_image_width(pp, info);
    self->rgb.image.h = (int)png_get_image_height(pp, info);
    self->rgb.image.d = channels;

    if (png_get_bit_depth(pp, info) < 8) {
        png_set_packing(pp);
        png_set_expand(pp);
    } else if (png_get_bit_depth(pp, info) == 16) {
        png_set_strip_16(pp);
    }

    if (png_get_valid(pp, info, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(pp);

    if (((size_t)self->rgb.image.w) * (size_t)self->rgb.image.h * (size_t)self->rgb.image.d > Fl_RGB_Image_get_max_size())
        longjmp(png_jmpbuf(pp), 1);

    self->rgb.array = (uchar *)malloc((size_t)self->rgb.image.w * (size_t)self->rgb.image.h * (size_t)self->rgb.image.d);
    self->rgb.alloc_array = 1;

    rows = (png_bytep *)malloc(sizeof(png_bytep) * (size_t)self->rgb.image.h);
    for (i = 0; i < self->rgb.image.h; i++)
        rows[i] = (png_bytep)(self->rgb.array + (size_t)i * (size_t)self->rgb.image.w * (size_t)self->rgb.image.d);

    for (i = png_set_interlace_handling(pp); i > 0; i--) png_read_rows(pp, rows, NULL, (png_uint_32)self->rgb.image.h);

    free(rows);

    png_read_end(pp, info);
    png_destroy_read_struct(&pp, &info, NULL);

    if (!from_memory) fclose(fp);
}

void Fl_PNG_Image_init(Fl_PNG_Image *self, const char *filename) {
    load_png(self, filename, NULL, 0);
}

Fl_PNG_Image *Fl_PNG_Image_new(const char *filename) {
    Fl_PNG_Image *self = (Fl_PNG_Image *)malloc(sizeof(Fl_PNG_Image));
    Fl_PNG_Image_init(self, filename);
    return self;
}

void Fl_PNG_Image_init_from_memory(Fl_PNG_Image *self, const unsigned char *buffer, int datasize) {
    load_png(self, NULL, buffer, datasize);
}

Fl_PNG_Image *Fl_PNG_Image_new_from_memory(const unsigned char *buffer, int datasize) {
    Fl_PNG_Image *self = (Fl_PNG_Image *)malloc(sizeof(Fl_PNG_Image));
    Fl_PNG_Image_init_from_memory(self, buffer, datasize);
    return self;
}
