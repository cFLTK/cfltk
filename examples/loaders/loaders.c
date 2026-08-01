/*
 * cfltk example: loaders
 *
 * Exercises the file-format image loaders: Fl_BMP_Image and
 * Fl_GIF_Image (always built, self-contained decoders) plus
 * Fl_PNG_Image and Fl_JPEG_Image (built only when CFLTK_HAVE_PNG /
 * CFLTK_HAVE_JPEG are defined -- see CMakeLists.txt/Makefile).
 *
 * There are no bundled sample image files, so this program generates
 * tiny test files itself at startup: a hand-written 2x2 24-bit BMP, a
 * hand-verified 1x1 GIF (byte-traced against Fl_GIF_Image's own LZW
 * decoder while writing it, see the comment above tiny_gif[]), and --
 * when available -- a PNG and a JPEG encoded via libpng/libjpeg
 * themselves, so those two are a genuine round-trip test (encode with
 * the same library cfltk decodes with).
 */
#include <stdio.h>
#include <stdlib.h>

#include "cfltk/Fl.h"
#include "cfltk/Fl_Window.h"
#include "cfltk/Fl_Box.h"
#include "cfltk/Fl_BMP_Image.h"
#include "cfltk/Fl_GIF_Image.h"
#ifdef CFLTK_HAVE_PNG
#include "cfltk/Fl_PNG_Image.h"
#include <png.h>
#endif
#ifdef CFLTK_HAVE_JPEG
#include "cfltk/Fl_JPEG_Image.h"
#include <jpeglib.h>
#include <setjmp.h>
#endif

/* ---------------------------------------------------------------------
 * Test file generators
 * ------------------------------------------------------------------ */

static void put_le16(FILE *f, unsigned v) { fputc((int)(v & 0xff), f); fputc((int)((v >> 8) & 0xff), f); }
static void put_le32(FILE *f, unsigned v) {
    fputc((int)(v & 0xff), f); fputc((int)((v >> 8) & 0xff), f);
    fputc((int)((v >> 16) & 0xff), f); fputc((int)((v >> 24) & 0xff), f);
}

/* 2x2 24-bit uncompressed BMP: top row red/green, bottom row blue/white.
 * BMP stores rows bottom-up, so the file writes blue/white first. */
static void generate_bmp(const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) return;

    fputc('B', f); fputc('M', f);
    put_le32(f, 70);  /* file size: 14 + 40 + 16 */
    put_le32(f, 0);   /* reserved */
    put_le32(f, 54);  /* offset to pixel data */

    put_le32(f, 40);  /* DIB header size */
    put_le32(f, 2);   /* width */
    put_le32(f, 2);   /* height (positive => bottom-up) */
    put_le16(f, 1);   /* planes */
    put_le16(f, 24);  /* bits per pixel */
    put_le32(f, 0);   /* BI_RGB, no compression */
    put_le32(f, 0);   /* image size (0 OK for BI_RGB) */
    put_le32(f, 0);   /* x pels/meter */
    put_le32(f, 0);   /* y pels/meter */
    put_le32(f, 0);   /* colors used */
    put_le32(f, 0);   /* important colors */

    /* bottom row (blue, white), BGR order, padded to a 4-byte row */
    fputc(255, f); fputc(0, f); fputc(0, f);     /* blue   */
    fputc(255, f); fputc(255, f); fputc(255, f); /* white  */
    fputc(0, f); fputc(0, f);                    /* padding */
    /* top row (red, green) */
    fputc(0, f); fputc(0, f); fputc(255, f);     /* red    */
    fputc(0, f); fputc(255, f); fputc(0, f);     /* green  */
    fputc(0, f); fputc(0, f);                    /* padding */

    fclose(f);
}

/* A minimal valid 1x1 GIF89a: 2-color global palette (black, white),
 * one uncompressed-LZW-coded white pixel. The LZW data bytes {0x4C,
 * 0x01} were derived by hand and verified by tracing them through
 * Fl_GIF_Image's own decode loop: they decode to the code sequence
 * Clear(4), 1, EOI(5) at 3 bits/code (min code size 2 => codes start
 * at 3 bits), which is exactly one white (palette index 1) pixel. */
static const unsigned char tiny_gif[] = {
    'G', 'I', 'F', '8', '9', 'a',
    0x01, 0x00, 0x01, 0x00, 0x80, 0x00, 0x00, /* 1x1, global color table (2 entries) */
    0x00, 0x00, 0x00,                         /* palette[0] = black */
    0xFF, 0xFF, 0xFF,                         /* palette[1] = white */
    0x2C, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, /* image descriptor */
    0x02,             /* LZW minimum code size */
    0x02, 0x4C, 0x01, /* sub-block: size 2, data */
    0x00,             /* block terminator */
    0x3B              /* trailer */
};

static void generate_gif(const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) return;
    fwrite(tiny_gif, 1, sizeof(tiny_gif), f);
    fclose(f);
}

#ifdef CFLTK_HAVE_PNG
/* 4x4 RGB gradient, encoded with libpng itself (round-trip test). */
static void generate_png(const char *path) {
    FILE *f = fopen(path, "wb");
    png_structp pp;
    png_infop info;
    int x, y;
    uchar row[4 * 3];

    if (!f) return;
    pp = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    info = png_create_info_struct(pp);
    if (setjmp(png_jmpbuf(pp))) { fclose(f); return; }

    png_init_io(pp, f);
    png_set_IHDR(pp, info, 4, 4, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(pp, info);

    for (y = 0; y < 4; y++) {
        for (x = 0; x < 4; x++) {
            row[x * 3 + 0] = (uchar)(x * 85);
            row[x * 3 + 1] = (uchar)(y * 85);
            row[x * 3 + 2] = 200;
        }
        png_write_row(pp, row);
    }
    png_write_end(pp, NULL);
    png_destroy_write_struct(&pp, &info);
    fclose(f);
}
#endif

#ifdef CFLTK_HAVE_JPEG
/* 4x4 RGB solid-ish pattern, encoded with libjpeg itself (round-trip
 * test; JPEG is lossy, so this only checks that decoding succeeds and
 * produces a plausible image, not exact pixel values). */
static void generate_jpeg(const char *path) {
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    FILE *f = fopen(path, "wb");
    JSAMPROW row_pointer[1];
    uchar row[4 * 3];
    int x, y;

    if (!f) return;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, f);

    cinfo.image_width = 4;
    cinfo.image_height = 4;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults(&cinfo);
    jpeg_start_compress(&cinfo, TRUE);

    while (cinfo.next_scanline < cinfo.image_height) {
        y = (int)cinfo.next_scanline;
        for (x = 0; x < 4; x++) { row[x * 3 + 0] = 200; row[x * 3 + 1] = (uchar)(x * 60); row[x * 3 + 2] = (uchar)(y * 60); }
        row_pointer[0] = row;
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    fclose(f);
}
#endif

/* ---------------------------------------------------------------------
 * UI
 * ------------------------------------------------------------------ */

static void report(Fl_Box *lbl, Fl_Image *img, const char *name) {
    char buf[128];
    int fail = Fl_Image_fail(img);
    if (fail) snprintf(buf, sizeof(buf), "%s: fail()=%d", name, fail);
    else snprintf(buf, sizeof(buf), "%s: %dx%d d=%d", name, Fl_Image_w(img), Fl_Image_h(img), Fl_Image_d(img));
    Fl_Widget_copy_label(&lbl->widget, buf);
}

int main(void) {
    Fl_Window *window;
    Fl_Box *bmp_box, *gif_box, *bmp_lbl, *gif_lbl;
    Fl_BMP_Image *bmp;
    Fl_GIF_Image *gif;
#ifdef CFLTK_HAVE_PNG
    Fl_Box *png_box, *png_lbl;
    Fl_PNG_Image *png;
#endif
#ifdef CFLTK_HAVE_JPEG
    Fl_Box *jpg_box, *jpg_lbl;
    Fl_JPEG_Image *jpg;
#endif

    generate_bmp("/tmp/cfltk_test.bmp");
    generate_gif("/tmp/cfltk_test.gif");
#ifdef CFLTK_HAVE_PNG
    generate_png("/tmp/cfltk_test.png");
#endif
#ifdef CFLTK_HAVE_JPEG
    generate_jpeg("/tmp/cfltk_test.jpg");
#endif

    window = Fl_Window_new(0, 0, 560, 160, "cfltk image loaders");

    bmp_lbl = Fl_Box_new(20, 10, 120, 20, "");
    Fl_Widget_set_align(&bmp_lbl->widget, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    bmp_box = Fl_Box_new(20, 35, 60, 60, NULL);
    Fl_Widget_set_box(&bmp_box->widget, FL_DOWN_BOX);
    bmp = Fl_BMP_Image_new("/tmp/cfltk_test.bmp");
    Fl_Widget_set_image(&bmp_box->widget, &bmp->rgb.image);
    report(bmp_lbl, &bmp->rgb.image, "BMP");

    gif_lbl = Fl_Box_new(140, 10, 120, 20, "");
    Fl_Widget_set_align(&gif_lbl->widget, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    gif_box = Fl_Box_new(140, 35, 60, 60, NULL);
    Fl_Widget_set_box(&gif_box->widget, FL_DOWN_BOX);
    gif = Fl_GIF_Image_new("/tmp/cfltk_test.gif");
    Fl_Widget_set_image(&gif_box->widget, &gif->pixmap.image);
    report(gif_lbl, &gif->pixmap.image, "GIF");

#ifdef CFLTK_HAVE_PNG
    png_lbl = Fl_Box_new(260, 10, 120, 20, "");
    Fl_Widget_set_align(&png_lbl->widget, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    png_box = Fl_Box_new(260, 35, 60, 60, NULL);
    Fl_Widget_set_box(&png_box->widget, FL_DOWN_BOX);
    png = Fl_PNG_Image_new("/tmp/cfltk_test.png");
    Fl_Widget_set_image(&png_box->widget, &png->rgb.image);
    report(png_lbl, &png->rgb.image, "PNG");
#endif

#ifdef CFLTK_HAVE_JPEG
    jpg_lbl = Fl_Box_new(380, 10, 150, 20, "");
    Fl_Widget_set_align(&jpg_lbl->widget, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    jpg_box = Fl_Box_new(380, 35, 60, 60, NULL);
    Fl_Widget_set_box(&jpg_box->widget, FL_DOWN_BOX);
    jpg = Fl_JPEG_Image_new("/tmp/cfltk_test.jpg");
    Fl_Widget_set_image(&jpg_box->widget, &jpg->rgb.image);
    report(jpg_lbl, &jpg->rgb.image, "JPEG");
#endif

    Fl_Group_end(&window->group);
    Fl_Widget_show(FL_WIDGET(window));

    return Fl_run();
}
