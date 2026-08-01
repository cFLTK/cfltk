/*
 * cfltk - Fl_JPEG_Image.c
 * See include/cfltk/Fl_JPEG_Image.h for the class-conversion notes.
 * Translated from src/Fl_JPEG_Image.cxx (HAVE_LIBJPEG path; only
 * compiled when CFLTK_ENABLE_JPEG is on, see CMakeLists.txt).
 */
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

#include <jpeglib.h>

#include "cfltk/Fl_JPEG_Image.h"

struct fl_jpeg_error_mgr {
    struct jpeg_error_mgr pub_;
    jmp_buf errhand_;
};

static void fl_jpeg_error_handler(j_common_ptr dinfo) {
    longjmp(((struct fl_jpeg_error_mgr *)(dinfo->err))->errhand_, 1);
}

static void fl_jpeg_output_handler(j_common_ptr dinfo) { (void)dinfo; }

static void load_jpeg(Fl_JPEG_Image *self, const char *filename, const unsigned char *data, unsigned long datasize) {
    FILE *fp = NULL;
    struct jpeg_decompress_struct dinfo;
    struct fl_jpeg_error_mgr jerr;
    JSAMPROW row;
    int from_memory = data != NULL;

    Fl_RGB_Image_init(&self->rgb, NULL, 0, 0, 0, 0);

    if (!from_memory) {
        fp = fopen(filename, "rb");
        if (!fp) { self->rgb.image.ld = FL_IMAGE_ERR_FILE_ACCESS; return; }
    }

    dinfo.err = jpeg_std_error(&jerr.pub_);
    jerr.pub_.error_exit = fl_jpeg_error_handler;
    jerr.pub_.output_message = fl_jpeg_output_handler;

    if (setjmp(jerr.errhand_)) {
        fprintf(stderr, "cfltk: JPEG \"%s\" is too large or contains errors\n", from_memory ? "in-memory data" : filename);
        if (self->rgb.array) jpeg_finish_decompress(&dinfo);
        jpeg_destroy_decompress(&dinfo);
        if (!from_memory) fclose(fp);
        self->rgb.image.w = 0; self->rgb.image.h = 0; self->rgb.image.d = 0;
        if (self->rgb.array) {
            free((void *)self->rgb.array);
            self->rgb.array = NULL;
            self->rgb.alloc_array = 0;
        }
        self->rgb.image.ld = FL_IMAGE_ERR_FORMAT;
        return;
    }

    jpeg_create_decompress(&dinfo);
    if (from_memory) jpeg_mem_src(&dinfo, data, datasize);
    else jpeg_stdio_src(&dinfo, fp);
    jpeg_read_header(&dinfo, TRUE);

    dinfo.quantize_colors = FALSE;
    dinfo.out_color_space = JCS_RGB;
    dinfo.out_color_components = 3;
    dinfo.output_components = 3;

    jpeg_calc_output_dimensions(&dinfo);

    self->rgb.image.w = (int)dinfo.output_width;
    self->rgb.image.h = (int)dinfo.output_height;
    self->rgb.image.d = dinfo.output_components;

    if (((size_t)self->rgb.image.w) * (size_t)self->rgb.image.h * (size_t)self->rgb.image.d > Fl_RGB_Image_get_max_size())
        longjmp(jerr.errhand_, 1);

    self->rgb.array = (uchar *)malloc((size_t)self->rgb.image.w * (size_t)self->rgb.image.h * (size_t)self->rgb.image.d);
    self->rgb.alloc_array = 1;

    jpeg_start_decompress(&dinfo);

    while (dinfo.output_scanline < dinfo.output_height) {
        row = (JSAMPROW)(self->rgb.array + (size_t)dinfo.output_scanline * (size_t)dinfo.output_width * (size_t)dinfo.output_components);
        jpeg_read_scanlines(&dinfo, &row, (JDIMENSION)1);
    }

    jpeg_finish_decompress(&dinfo);
    jpeg_destroy_decompress(&dinfo);

    if (!from_memory) fclose(fp);
}

void Fl_JPEG_Image_init(Fl_JPEG_Image *self, const char *filename) {
    load_jpeg(self, filename, NULL, 0);
}

Fl_JPEG_Image *Fl_JPEG_Image_new(const char *filename) {
    Fl_JPEG_Image *self = (Fl_JPEG_Image *)malloc(sizeof(Fl_JPEG_Image));
    Fl_JPEG_Image_init(self, filename);
    return self;
}

void Fl_JPEG_Image_init_from_memory(Fl_JPEG_Image *self, const unsigned char *data, unsigned long datasize) {
    load_jpeg(self, NULL, data, datasize);
}

Fl_JPEG_Image *Fl_JPEG_Image_new_from_memory(const unsigned char *data, unsigned long datasize) {
    Fl_JPEG_Image *self = (Fl_JPEG_Image *)malloc(sizeof(Fl_JPEG_Image));
    Fl_JPEG_Image_init_from_memory(self, data, datasize);
    return self;
}
