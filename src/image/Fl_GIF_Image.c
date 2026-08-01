/*
 * cfltk - Fl_GIF_Image.c
 * See include/cfltk/Fl_GIF_Image.h for the class-conversion notes.
 * Translated from src/Fl_GIF_Image.cxx (itself extensively modified
 * from gif2ras.c by Patrick J. Naughton).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cfltk/Fl_GIF_Image.h"

#define NEXTBYTE ((uchar)getc(gif_file))
#define GETSHORT(var) do { (var) = NEXTBYTE; (var) += NEXTBYTE << 8; } while (0)

void Fl_GIF_Image_init(Fl_GIF_Image *self, const char *infname) {
    FILE *gif_file;
    char **new_data;
    char b[6];
    int Width, Height;
    uchar ch;
    char HasColormap;
    int BitsPerPixel, ColorMapSize;
    uchar transparent_pixel = 0;
    char has_transparent = 0;
    uchar Red[256], Green[256], Blue[256];
    int CodeSize = 0;
    char Interlace = 0;
    uchar *Image, *p, *eol;
    int YC, Pass;
    int InitCodeSize, ClearCode, EOFCode, FirstFree, FinChar, ReadMask, FreeCode, OldCode;
    short int Prefix[4096];
    uchar Suffix[4096];
    uchar OutCode[4097];
    int blocklen;
    uchar thisbyte;
    int frombit;
    int i;

    Fl_Pixmap_init(&self->pixmap, NULL);

    gif_file = fopen(infname, "rb");
    if (!gif_file) {
        fprintf(stderr, "cfltk: Fl_GIF_Image: unable to open %s\n", infname);
        self->pixmap.image.ld = FL_IMAGE_ERR_FILE_ACCESS;
        return;
    }

    if (fread(b, 1, 6, gif_file) < 6) {
        fclose(gif_file);
        self->pixmap.image.ld = FL_IMAGE_ERR_FILE_ACCESS;
        return;
    }
    if (b[0] != 'G' || b[1] != 'I' || b[2] != 'F') {
        fclose(gif_file);
        fprintf(stderr, "cfltk: Fl_GIF_Image: %s is not a GIF file\n", infname);
        self->pixmap.image.ld = FL_IMAGE_ERR_FORMAT;
        return;
    }

    GETSHORT(Width);
    GETSHORT(Height);

    ch = NEXTBYTE;
    HasColormap = (char)((ch & 0x80) != 0);
    BitsPerPixel = (ch & 7) + 1;
    ColorMapSize = HasColormap ? (2 << (ch & 7)) : 0;
    ch = NEXTBYTE; /* background color index */
    ch = NEXTBYTE; /* aspect ratio */

    if (HasColormap) {
        for (i = 0; i < ColorMapSize; i++) { Red[i] = NEXTBYTE; Green[i] = NEXTBYTE; Blue[i] = NEXTBYTE; }
    }

    for (;;) {
        int c = NEXTBYTE;
        /* upstream checks "if (i<0)" here, but NEXTBYTE's (uchar) cast
         * makes that always false even at EOF (getc()'s -1 wraps to
         * 255) -- a latent bug that leaves upstream looping on
         * truncated files. Checking feof() directly instead is a
         * deliberate, minor robustness fix, not a behavior difference
         * for any well-formed GIF. */
        if (feof(gif_file)) {
            fclose(gif_file);
            fprintf(stderr, "cfltk: Fl_GIF_Image: %s - unexpected EOF\n", infname);
            self->pixmap.image.w = 0; self->pixmap.image.h = 0; self->pixmap.image.d = 0;
            self->pixmap.image.ld = FL_IMAGE_ERR_FORMAT;
            return;
        }

        if (c == 0x21) { /* GIF extension block */
            ch = NEXTBYTE;
            blocklen = NEXTBYTE;
            if (ch == 0xF9 && blocklen == 4) {
                char bits = (char)NEXTBYTE;
                getc(gif_file); getc(gif_file);
                transparent_pixel = NEXTBYTE;
                if (bits & 1) has_transparent = 1;
                blocklen = NEXTBYTE;
            } else if (ch == 0xFF) {
                /* Netscape repeat count, ignored */
            } else if (ch != 0xFE) {
                fprintf(stderr, "cfltk: Fl_GIF_Image: %s - unknown extension 0x%02x\n", infname, ch);
            }
        } else if (c == 0x2c) { /* image descriptor: this is the frame we want */
            ch = NEXTBYTE; ch = NEXTBYTE;
            ch = NEXTBYTE; ch = NEXTBYTE;
            GETSHORT(Width);
            GETSHORT(Height);
            ch = NEXTBYTE;
            Interlace = (char)((ch & 0x40) != 0);
            if (ch & 0x80) {
                BitsPerPixel = (ch & 7) + 1;
                ColorMapSize = 2 << (ch & 7);
                for (i = 0; i < ColorMapSize; i++) { Red[i] = NEXTBYTE; Green[i] = NEXTBYTE; Blue[i] = NEXTBYTE; }
            }
            CodeSize = NEXTBYTE + 1;
            break;
        } else {
            fprintf(stderr, "cfltk: Fl_GIF_Image: %s - unknown code 0x%02x\n", infname, c);
            blocklen = 0;
        }

        while (blocklen > 0) { while (blocklen--) ch = NEXTBYTE; blocklen = NEXTBYTE; }
    }

    if (BitsPerPixel >= CodeSize) {
        BitsPerPixel = CodeSize - 1;
        ColorMapSize = 1 << BitsPerPixel;
    }

    if (ColorMapSize == 0) {
        fprintf(stderr, "cfltk: Fl_GIF_Image: %s has no color table, using default\n", infname);
        BitsPerPixel = CodeSize - 1;
        ColorMapSize = 1 << BitsPerPixel;
        Red[0] = Green[0] = Blue[0] = 0;
        Red[1] = Green[1] = Blue[1] = 255;
        for (i = 2; i < ColorMapSize; i++) Red[i] = Green[i] = Blue[i] = (uchar)(255 * i / (ColorMapSize - 1));
    }

    Image = (uchar *)malloc((size_t)Width * (size_t)Height);

    YC = 0; Pass = 0;
    p = Image;
    eol = p + Width;

    InitCodeSize = CodeSize;
    ClearCode = (1 << (CodeSize - 1));
    EOFCode = ClearCode + 1;
    FirstFree = ClearCode + 2;
    FinChar = 0;
    ReadMask = (1 << CodeSize) - 1;
    FreeCode = FirstFree;
    OldCode = ClearCode;

    blocklen = NEXTBYTE;
    thisbyte = NEXTBYTE; blocklen--;
    frombit = 0;

    for (;;) {
        int CurCode = thisbyte;
        uchar *tp;

        if (frombit + CodeSize > 7) {
            if (blocklen <= 0) { blocklen = NEXTBYTE; if (blocklen <= 0) break; }
            thisbyte = NEXTBYTE; blocklen--;
            CurCode |= thisbyte << 8;
        }
        if (frombit + CodeSize > 15) {
            if (blocklen <= 0) { blocklen = NEXTBYTE; if (blocklen <= 0) break; }
            thisbyte = NEXTBYTE; blocklen--;
            CurCode |= thisbyte << 16;
        }
        CurCode = (CurCode >> frombit) & ReadMask;
        frombit = (frombit + CodeSize) % 8;

        if (CurCode == ClearCode) {
            CodeSize = InitCodeSize;
            ReadMask = (1 << CodeSize) - 1;
            FreeCode = FirstFree;
            OldCode = ClearCode;
            continue;
        }
        if (CurCode == EOFCode) break;

        tp = OutCode;
        if (CurCode < FreeCode) i = CurCode;
        else if (CurCode == FreeCode) { *tp++ = (uchar)FinChar; i = OldCode; }
        else { fprintf(stderr, "cfltk: Fl_GIF_Image: %s - LZW barf\n", infname); break; }

        while (i >= ColorMapSize) { *tp++ = Suffix[i]; i = Prefix[i]; }
        *tp++ = (uchar)(FinChar = i);
        do {
            *p++ = *--tp;
            if (p >= eol) {
                if (!Interlace) YC++;
                else switch (Pass) {
                    case 0: YC += 8; if (YC >= Height) { Pass++; YC = 4; } break;
                    case 1: YC += 8; if (YC >= Height) { Pass++; YC = 2; } break;
                    case 2: YC += 4; if (YC >= Height) { Pass++; YC = 1; } break;
                    case 3: YC += 2; break;
                    default: break;
                }
                if (YC >= Height) YC = 0;
                p = Image + YC * Width;
                eol = p + Width;
            }
        } while (tp > OutCode);

        if (OldCode != ClearCode) {
            Prefix[FreeCode] = (short)OldCode;
            Suffix[FreeCode] = (uchar)FinChar;
            FreeCode++;
            if (FreeCode > ReadMask) {
                if (CodeSize < 12) { CodeSize++; ReadMask = (1 << CodeSize) - 1; }
                else FreeCode--;
            }
        }
        OldCode = CurCode;
    }

    /* Done reading; convert to the compressed-colormap XPM dialect
     * Fl_Pixmap already understands. */
    self->pixmap.image.w = Width;
    self->pixmap.image.h = Height;
    self->pixmap.image.d = 1;
    new_data = (char **)malloc(sizeof(char *) * (size_t)(Height + 2));

    if (has_transparent && transparent_pixel != 0) {
        uchar t;
        p = Image + (size_t)Width * (size_t)Height;
        while (p-- > Image) {
            if (*p == transparent_pixel) *p = 0;
            else if (!*p) *p = transparent_pixel;
        }
        t = Red[0]; Red[0] = Red[transparent_pixel]; Red[transparent_pixel] = t;
        t = Green[0]; Green[0] = Green[transparent_pixel]; Green[transparent_pixel] = t;
        t = Blue[0]; Blue[0] = Blue[transparent_pixel]; Blue[transparent_pixel] = t;
    }

    {
        uchar used[256], remap[256];
        int base, numcolors, length;

        for (i = 0; i < ColorMapSize; i++) used[i] = 0;
        p = Image + (size_t)Width * (size_t)Height;
        while (p-- > Image) used[*p] = 1;

        base = (has_transparent && used[0]) ? ' ' : ' ' + 1;
        numcolors = 0;
        for (i = 0; i < ColorMapSize; i++) if (used[i]) { remap[i] = (uchar)(base++); numcolors++; }

        length = sprintf((char *)Suffix, "%d %d %d %d", Width, Height, -numcolors, 1);
        new_data[0] = (char *)malloc((size_t)length + 1);
        strcpy(new_data[0], (char *)Suffix);

        new_data[1] = (char *)malloc((size_t)4 * (size_t)numcolors);
        p = (uchar *)new_data[1];
        for (i = 0; i < ColorMapSize; i++) if (used[i]) {
            *p++ = remap[i]; *p++ = Red[i]; *p++ = Green[i]; *p++ = Blue[i];
        }

        p = Image + (size_t)Width * (size_t)Height;
        while (p-- > Image) *p = remap[*p];

        for (i = 0; i < Height; i++) {
            new_data[i + 2] = (char *)malloc((size_t)Width + 1);
            memcpy(new_data[i + 2], (char *)(Image + (size_t)i * (size_t)Width), (size_t)Width);
            new_data[i + 2][Width] = 0;
        }
    }

    self->pixmap.image.data = (const char *const *)new_data;
    self->pixmap.image.count = Height + 2;
    self->pixmap.alloc_data = 1;

    free(Image);
    fclose(gif_file);
}

Fl_GIF_Image *Fl_GIF_Image_new(const char *filename) {
    Fl_GIF_Image *self = (Fl_GIF_Image *)malloc(sizeof(Fl_GIF_Image));
    Fl_GIF_Image_init(self, filename);
    return self;
}
