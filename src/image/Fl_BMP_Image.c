/*
 * cfltk - Fl_BMP_Image.c
 * See include/cfltk/Fl_BMP_Image.h for the class-conversion notes.
 * Translated from src/Fl_BMP_Image.cxx.
 */
#include <stdio.h>
#include <stdlib.h>

#include "cfltk/Fl_BMP_Image.h"

#define BI_RGB       0
#define BI_RLE8      1
#define BI_RLE4      2
#define BI_BITFIELDS 3

static unsigned short read_word(FILE *fp) {
    unsigned char b0, b1;
    b0 = (uchar)getc(fp);
    b1 = (uchar)getc(fp);
    return (unsigned short)((b1 << 8) | b0);
}

static unsigned int read_dword(FILE *fp) {
    unsigned char b0, b1, b2, b3;
    b0 = (uchar)getc(fp);
    b1 = (uchar)getc(fp);
    b2 = (uchar)getc(fp);
    b3 = (uchar)getc(fp);
    return (unsigned int)((((((b3 << 8) | b2) << 8) | b1) << 8) | b0);
}

static int read_long(FILE *fp) {
    unsigned char b0, b1, b2, b3;
    b0 = (uchar)getc(fp);
    b1 = (uchar)getc(fp);
    b2 = (uchar)getc(fp);
    b3 = (uchar)getc(fp);
    return (int)((unsigned)(((((b3 << 8) | b2) << 8) | b1) << 8) | b0);
}

void Fl_BMP_Image_init(Fl_BMP_Image *self, const char *bmp) {
    FILE *fp;
    int info_size, depth, bDepth = 3, compression, colors_used;
    int x, y, color, repcount, temp, align, dataSize, row_order, start_y, end_y;
    long offbits;
    uchar bit, byte;
    uchar *ptr;
    uchar colormap[256][3];
    uchar havemask;
    int use_5_6_5;
    int w, h;

    Fl_RGB_Image_init(&self->rgb, NULL, 0, 0, 0, 0);

    fp = fopen(bmp, "rb");
    if (!fp) { self->rgb.image.ld = FL_IMAGE_ERR_FILE_ACCESS; return; }

    byte = (uchar)getc(fp);
    bit = (uchar)getc(fp);
    if (byte != 'B' || bit != 'M') { fclose(fp); self->rgb.image.ld = FL_IMAGE_ERR_FORMAT; return; }

    read_dword(fp);
    read_word(fp);
    read_word(fp);
    offbits = (long)read_dword(fp);

    info_size = (int)read_dword(fp);

    havemask = 0;
    row_order = -1;
    use_5_6_5 = 0;

    if (info_size < 40) {
        w = read_word(fp);
        h = read_word(fp);
        read_word(fp);
        depth = read_word(fp);
        compression = BI_RGB;
        colors_used = 0;
        repcount = info_size - 12;
    } else {
        w = read_long(fp);
        temp = read_long(fp);
        if (temp < 0) row_order = 1;
        h = abs(temp);
        read_word(fp);
        depth = read_word(fp);
        compression = (int)read_dword(fp);
        dataSize = (int)read_dword(fp);
        read_long(fp);
        read_long(fp);
        colors_used = (int)read_dword(fp);
        read_dword(fp);

        repcount = info_size - 40;

        if (!compression && depth >= 8 && w > 32 / depth) {
            int Bpp = depth / 8;
            int maskSize = (((w * Bpp + 3) & ~3) * h) + (((((w + 7) / 8) + 3) & ~3) * h);
            if (maskSize == 2 * dataSize) {
                havemask = 1;
                h = h / 2;
                bDepth = 4;
            }
        }
    }

    while (repcount > 0) { getc(fp); repcount--; }

    if (!w || !h || !depth) {
        fclose(fp);
        self->rgb.image.w = 0; self->rgb.image.h = 0; self->rgb.image.d = 0;
        self->rgb.image.ld = FL_IMAGE_ERR_FORMAT;
        return;
    }

    if (colors_used == 0 && depth <= 8) colors_used = 1 << depth;

    for (repcount = 0; repcount < colors_used; repcount++) {
        if (fread(colormap[repcount], 1, 3, fp) == 0) { /* ignore */ }
        if (info_size > 12) getc(fp);
    }

    if (depth == 16) use_5_6_5 = (read_dword(fp) == 0xf800);
    if (depth == 32) bDepth = 4;

    self->rgb.image.w = w;
    self->rgb.image.h = h;
    self->rgb.image.d = bDepth;
    if (offbits) fseek(fp, offbits, SEEK_SET);

    if (((size_t)w) * (size_t)h * (size_t)bDepth > Fl_RGB_Image_get_max_size()) {
        fprintf(stderr, "cfltk: BMP file \"%s\" is too large\n", bmp);
        fclose(fp);
        self->rgb.image.w = 0; self->rgb.image.h = 0; self->rgb.image.d = 0;
        self->rgb.image.ld = FL_IMAGE_ERR_FORMAT;
        return;
    }
    self->rgb.array = (uchar *)malloc((size_t)w * (size_t)h * (size_t)bDepth);
    self->rgb.alloc_array = 1;

    color = 0; repcount = 0; align = 0; byte = 0; temp = 0;

    if (row_order < 0) { start_y = h - 1; end_y = -1; } else { start_y = 0; end_y = h; }

    for (y = start_y; y != end_y; y += row_order) {
        ptr = (uchar *)self->rgb.array + (size_t)y * (size_t)w * (size_t)bDepth;

        switch (depth) {
            case 1:
                for (x = w, bit = 128; x > 0; x--) {
                    if (bit == 128) byte = (uchar)getc(fp);
                    if (byte & bit) {
                        *ptr++ = colormap[1][2]; *ptr++ = colormap[1][1]; *ptr++ = colormap[1][0];
                    } else {
                        *ptr++ = colormap[0][2]; *ptr++ = colormap[0][1]; *ptr++ = colormap[0][0];
                    }
                    if (bit > 1) bit >>= 1; else bit = 128;
                }
                for (temp = (w + 7) / 8; temp & 3; temp++) getc(fp);
                break;

            case 4:
                for (x = w, bit = 0xf0; x > 0; x--) {
                    if (repcount == 0) {
                        if (compression != BI_RLE4) {
                            repcount = 2;
                            color = -1;
                        } else {
                            while (align > 0) { align--; getc(fp); }
                            if ((repcount = getc(fp)) == 0) {
                                if ((repcount = getc(fp)) == 0) { x++; continue; }
                                else if (repcount == 1) break;
                                else if (repcount == 2) { repcount = getc(fp) * getc(fp) * w; color = 0; }
                                else { color = -1; align = ((4 - (repcount & 3)) / 2) & 1; }
                            } else {
                                color = getc(fp);
                            }
                        }
                    }

                    repcount--;

                    if (bit == 0xf0) {
                        if (color < 0) temp = getc(fp); else temp = color;
                        *ptr++ = colormap[(temp >> 4) & 15][2];
                        *ptr++ = colormap[(temp >> 4) & 15][1];
                        *ptr++ = colormap[(temp >> 4) & 15][0];
                        bit = 0x0f;
                    } else {
                        bit = 0xf0;
                        *ptr++ = colormap[temp & 15][2];
                        *ptr++ = colormap[temp & 15][1];
                        *ptr++ = colormap[temp & 15][0];
                    }
                }
                if (!compression) for (temp = (w + 1) / 2; temp & 3; temp++) getc(fp);
                break;

            case 8:
                for (x = w; x > 0; x--) {
                    if (compression != BI_RLE8) { repcount = 1; color = -1; }
                    if (repcount == 0) {
                        while (align > 0) { align--; getc(fp); }
                        if ((repcount = getc(fp)) == 0) {
                            if ((repcount = getc(fp)) == 0) { x++; continue; }
                            else if (repcount == 1) break;
                            else if (repcount == 2) { repcount = getc(fp) * getc(fp) * w; color = 0; }
                            else { color = -1; align = (2 - (repcount & 1)) & 1; }
                        } else {
                            color = getc(fp);
                        }
                    }
                    if (color < 0) temp = getc(fp); else temp = color;
                    repcount--;
                    *ptr++ = colormap[temp][2]; *ptr++ = colormap[temp][1]; *ptr++ = colormap[temp][0];
                    if (havemask) ptr++;
                }
                if (!compression) for (temp = w; temp & 3; temp++) getc(fp);
                break;

            case 16:
                for (x = w; x > 0; x--, ptr += bDepth) {
                    uchar b = (uchar)getc(fp), a = (uchar)getc(fp);
                    if (use_5_6_5) {
                        ptr[2] = (uchar)((b << 3) & 0xf8);
                        ptr[1] = (uchar)(((a << 5) & 0xe0) | ((b >> 3) & 0x1c));
                        ptr[0] = (uchar)(a & 0xf8);
                    } else {
                        ptr[2] = (uchar)((b << 3) & 0xf8);
                        ptr[1] = (uchar)(((a << 6) & 0xc0) | ((b >> 2) & 0x38));
                        ptr[0] = (uchar)((a << 1) & 0xf8);
                    }
                }
                for (temp = w * 2; temp & 3; temp++) getc(fp);
                break;

            case 24:
                for (x = w; x > 0; x--, ptr += bDepth) {
                    ptr[2] = (uchar)getc(fp); ptr[1] = (uchar)getc(fp); ptr[0] = (uchar)getc(fp);
                }
                for (temp = w * 3; temp & 3; temp++) getc(fp);
                break;

            case 32:
                for (x = w; x > 0; x--, ptr += bDepth) {
                    ptr[2] = (uchar)getc(fp); ptr[1] = (uchar)getc(fp); ptr[0] = (uchar)getc(fp); ptr[3] = (uchar)getc(fp);
                }
                break;

            default:
                break;
        }
    }

    if (havemask) {
        for (y = h - 1; y >= 0; y--) {
            ptr = (uchar *)self->rgb.array + (size_t)y * (size_t)w * (size_t)bDepth + 3;
            for (x = w, bit = 128; x > 0; x--, ptr += bDepth) {
                if (bit == 128) byte = (uchar)getc(fp);
                if (byte & bit) *ptr = 0; else *ptr = 255;
                if (bit > 1) bit >>= 1; else bit = 128;
            }
            for (temp = (w + 7) / 8; temp & 3; temp++) getc(fp);
        }
    }

    fclose(fp);
}

Fl_BMP_Image *Fl_BMP_Image_new(const char *filename) {
    Fl_BMP_Image *self = (Fl_BMP_Image *)malloc(sizeof(Fl_BMP_Image));
    Fl_BMP_Image_init(self, filename);
    return self;
}
