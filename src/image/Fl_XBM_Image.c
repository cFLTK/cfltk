/*
 * cfltk - Fl_XBM_Image.c
 * See include/cfltk/Fl_XBM_Image.h for the class-conversion notes.
 * Translated from src/Fl_XBM_Image.cxx.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cfltk/Fl_XBM_Image.h"

Fl_Bitmap *Fl_XBM_Image_new(const char *name) {
    Fl_Bitmap *self = Fl_Bitmap_new(NULL, 0, 0);
    FILE *f;
    char buffer[1024];
    char junk[1024];
    int wh[2];
    int i, n;
    uchar *bits, *ptr;

    f = fopen(name, "rb");
    if (!f) return self;

    for (i = 0; i < 2; i++) {
        for (;;) {
            if (!fgets(buffer, sizeof(buffer), f)) { fclose(f); return self; }
            if (sscanf(buffer, "#define %1023s %d", junk, &wh[i]) >= 2) break;
        }
    }

    /* skip to the data array */
    for (;;) {
        if (!fgets(buffer, sizeof(buffer), f)) { fclose(f); return self; }
        if (!strncmp(buffer, "static ", 7)) break;
    }

    n = ((wh[0] + 7) / 8) * wh[1];
    bits = (uchar *)malloc((size_t)(n > 0 ? n : 1));

    for (i = 0, ptr = bits; i < n;) {
        const char *a;
        if (!fgets(buffer, sizeof(buffer), f)) { free(bits); fclose(f); return self; }
        a = buffer;
        while (*a && i < n) {
            unsigned int t;
            if (sscanf(a, " 0x%x", &t) > 0) {
                *ptr++ = (uchar)t;
                i++;
            }
            while (*a && *a++ != ',') { /* empty */ }
        }
    }

    fclose(f);

    Fl_Bitmap_init(self, bits, wh[0], wh[1]);
    self->alloc_array = 1;
    return self;
}
