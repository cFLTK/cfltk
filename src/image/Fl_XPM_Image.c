/*
 * cfltk - Fl_XPM_Image.c
 * See include/cfltk/Fl_XPM_Image.h for the class-conversion notes.
 * Translated from src/Fl_XPM_Image.cxx.
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cfltk/Fl_XPM_Image.h"

#define MAXSIZE 2048
#define INITIALLINES 256

static int hexdigit(int x) {
    if (isdigit(x)) return x - '0';
    if (isupper(x)) return x - 'A' + 10;
    if (islower(x)) return x - 'a' + 10;
    return 20;
}

Fl_Pixmap *Fl_XPM_Image_new(const char *name) {
    Fl_Pixmap *self = Fl_Pixmap_new(NULL);
    FILE *f;
    char **new_data, **temp_data;
    int malloc_size = INITIALLINES;
    char buffer[MAXSIZE + 20];
    int i = 0;
    int W = 0, H = 0, ncolors = 0, chars_per_pixel = 0;

    f = fopen(name, "rb");
    if (!f) return self;

    new_data = (char **)malloc(sizeof(char *) * (size_t)INITIALLINES);

    while (fgets(buffer, sizeof(buffer), f)) {
        char *myp, *q;

        if (buffer[0] != '"') continue;
        myp = buffer;
        q = buffer + 1;
        while (*q != '"' && myp < buffer + MAXSIZE) {
            if (*q == '\\') {
                switch (*++q) {
                    case '\r':
                    case '\n':
                        if (!fgets(q, (int)(buffer + MAXSIZE + 20 - q), f)) { /* ok at EOF */ }
                        break;
                    case 0:
                        break;
                    case 'x': {
                        int n = 0, x;
                        q++;
                        for (x = 0; x < 2; x++) {
                            int xd = hexdigit((unsigned char)*q);
                            if (xd > 15) break;
                            n = (n << 4) + xd;
                            q++;
                        }
                        *myp++ = (char)n;
                        break;
                    }
                    default: {
                        int c = (unsigned char)*q++;
                        if (c >= '0' && c <= '7') {
                            int x;
                            c -= '0';
                            for (x = 0; x < 2; x++) {
                                int xd = hexdigit((unsigned char)*q);
                                if (xd > 7) break;
                                c = (c << 3) + xd;
                                q++;
                            }
                        }
                        *myp++ = (char)c;
                        break;
                    }
                }
            } else {
                *myp++ = *q++;
            }
        }
        *myp++ = 0;

        if (i >= malloc_size) {
            temp_data = (char **)malloc(sizeof(char *) * (size_t)(malloc_size + INITIALLINES));
            memcpy(temp_data, new_data, sizeof(char *) * (size_t)malloc_size);
            free(new_data);
            new_data = temp_data;
            malloc_size += INITIALLINES;
        }

        /* first line: 4 ints (W H NCOLORS CPP); color-table lines; then
         * H pixel-grid lines -- same three-way validation upstream does. */
        if (!i && sscanf(buffer, "%d%d%d%d", &W, &H, &ncolors, &chars_per_pixel) < 4) goto bad_data;
        else if (i > (ncolors < 0 ? 1 : ncolors) && (myp - buffer - 1 < W * chars_per_pixel)) goto bad_data;
        else if (myp - buffer - 1 < (ncolors < 0 ? -ncolors * 4 : chars_per_pixel)) goto bad_data;

        new_data[i] = (char *)malloc((size_t)(myp - buffer + 1));
        memcpy(new_data[i], buffer, (size_t)(myp - buffer));
        new_data[i][myp - buffer] = 0;
        i++;
    }

    fclose(f);
    f = NULL;
    if (!i || i < 1 + (ncolors < 0 ? 1 : ncolors) + H) goto bad_data;

    Fl_Pixmap_init(self, (const char *const *)new_data);
    self->alloc_data = 1;
    return self;

bad_data:
    while (i > 0) free(new_data[--i]);
    free(new_data);
    if (f) fclose(f);
    return self;
}
