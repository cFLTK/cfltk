/*
 * cfltk - Fl_File_Browser.c
 * See include/cfltk/Fl_File_Browser.h for the class-conversion notes.
 * Translated from src/Fl_File_Browser.cxx (icon-related branches
 * dropped, see header; Linux/mtab-fallback-chain-only "list mounted
 * filesystems" branch, other platforms' branches dropped).
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cfltk/Fl_File_Browser.h"
#include "cfltk/fl_draw.h"

/* Matches FL_BLINE::flags bit values -- private to the browser
 * implementation (see Fl_Browser.c), redefined here because upstream
 * itself redefines them in Fl_File_Browser.cxx rather than exposing
 * them (its own acknowledged wart, ported faithfully: FL_BLINE's
 * `flags` byte is an internal encoding, not part of the public
 * struct's documented contract). */
#define SELECTED 1

static const int no_columns[1] = { 0 };

static int file_item_height(Fl_Browser_ *base, void *p) {
    FL_BLINE *line = (FL_BLINE *)p;
    int textheight;
    int height;
    char *t;

    fl_font(Fl_Browser__textfont(base), Fl_Browser__textsize(base));
    textheight = fl_height();
    height = textheight;

    if (line) for (t = line->txt; *t != '\0'; t++) if (*t == '\n') height += textheight;

    return height + 2;
}

static int file_item_width(Fl_Browser_ *base, void *p) {
    Fl_Browser *br = (Fl_Browser *)base;
    FL_BLINE *line = (FL_BLINE *)p;
    char fragment[10240], *ptr;
    const char *t;
    int width = 0, tempwidth = 0, column = 0, i;
    const int *columns = Fl_Browser_column_widths(br);

    if (line->length > 0 && line->txt[line->length - 1] == '/')
        fl_font((Fl_Font)(Fl_Browser__textfont(base) | FL_BOLD), Fl_Browser__textsize(base));
    else
        fl_font(Fl_Browser__textfont(base), Fl_Browser__textsize(base));

    if (!strchr(line->txt, '\n') && !strchr(line->txt, Fl_Browser_column_char(br))) {
        width = (int)fl_width_str(line->txt);
    } else {
        for (t = line->txt, ptr = fragment; *t != '\0'; t++) {
            if (*t == '\n') {
                *ptr = '\0';
                tempwidth += (int)fl_width_str(fragment);
                if (tempwidth > width) width = tempwidth;
                ptr = fragment; tempwidth = 0; column = 0;
            } else if (*t == Fl_Browser_column_char(br)) {
                column++;
                if (columns) { for (i = 0, tempwidth = 0; i < column && columns[i]; i++) tempwidth += columns[i]; }
                else tempwidth = column * (int)(fl_height() * 0.6 * 8.0);
                if (tempwidth > width) width = tempwidth;
                ptr = fragment;
            } else {
                *ptr++ = *t;
            }
        }
        if (ptr > fragment) {
            *ptr = '\0';
            tempwidth += (int)fl_width_str(fragment);
            if (tempwidth > width) width = tempwidth;
        }
    }

    return width + 2;
}

static void draw_fragment(const char *fragment, int X, int Y, int W, int H, Fl_Font font, Fl_Fontsize size, Fl_Color color) {
    Fl_Label lbl;
    lbl.value = fragment; lbl.image = NULL; lbl.deimage = NULL; lbl.type = FL_NORMAL_LABEL;
    lbl.font = font; lbl.size = size; lbl.color = color; lbl.align = FL_ALIGN_LEFT;
    fl_label_draw(&lbl, X, Y, W, H, (Fl_Align)(FL_ALIGN_LEFT | FL_ALIGN_CLIP));
}

static void file_item_draw(Fl_Browser_ *base, void *p, int X, int Y, int W, int H) {
    Fl_Browser *br = (Fl_Browser *)base;
    Fl_Widget *bw = &base->group.widget;
    FL_BLINE *line = (FL_BLINE *)p;
    Fl_Color c;
    Fl_Font font;
    Fl_Fontsize size;
    char fragment[10240], *ptr;
    const char *t;
    const int *columns = Fl_Browser_column_widths(br);
    int width = 0, column = 0, i;
    (void)H;

    if (line->length > 0 && line->txt[line->length - 1] == '/') font = (Fl_Font)(Fl_Browser__textfont(base) | FL_BOLD);
    else font = Fl_Browser__textfont(base);
    size = Fl_Browser__textsize(base);
    fl_font(font, size);

    c = (line->flags & SELECTED) ? fl_contrast(Fl_Browser__textcolor(base), Fl_Widget_selection_color(bw)) : Fl_Browser__textcolor(base);
    if (!Fl_Widget_active_r(bw)) c = fl_inactive(c);

    X += 1;
    W -= 2;

    for (t = line->txt, ptr = fragment; *t != '\0'; t++) {
        if (*t == '\n') {
            *ptr = '\0';
            draw_fragment(fragment, X + width, Y, W - width, fl_height(), font, size, c);
            ptr = fragment; width = 0; column = 0; Y += fl_height();
        } else if (*t == Fl_Browser_column_char(br)) {
            int cW = W - width;
            *ptr = '\0';
            if (columns) {
                for (i = 0; i < column && columns[i]; i++) { /* find current column's declared width, if any */ }
                if (columns[i]) cW = columns[i];
            }
            draw_fragment(fragment, X + width, Y, cW, fl_height(), font, size, c);
            column++;
            if (columns) { for (i = 0, width = 0; i < column && columns[i]; i++) width += columns[i]; }
            else width = column * (int)(fl_height() * 0.6 * 8.0);
            ptr = fragment;
        } else {
            *ptr++ = *t;
        }
    }
    if (ptr > fragment) {
        *ptr = '\0';
        draw_fragment(fragment, X + width, Y, W - width, fl_height(), font, size, c);
    }
}

static int file_full_height(Fl_Browser_ *base) { return ((Fl_Browser *)base)->full_height_; }

static Fl_Browser_ItemOps g_file_browser_item_ops;
static int g_item_ops_ready = 0;

static const Fl_Browser_ItemOps *file_browser_item_ops(void) {
    if (!g_item_ops_ready) {
        g_file_browser_item_ops = fl_browser_item_ops; /* struct copy: not a constant expression, so this can't be a static initializer -- see header */
        g_file_browser_item_ops.item_height = file_item_height;
        g_file_browser_item_ops.item_width = file_item_width;
        g_file_browser_item_ops.item_draw = file_item_draw;
        g_file_browser_item_ops.full_height = file_full_height;
        g_item_ops_ready = 1;
    }
    return &g_file_browser_item_ops;
}

void Fl_File_Browser_init(Fl_File_Browser *self, int x, int y, int w, int h, const char *label) {
    Fl_Browser *br = &self->browser;

    Fl_Browser__init(&br->browser_, &fl_browser_ops, file_browser_item_ops(), x, y, w, h, label);
    br->column_widths_ = no_columns;
    br->lines = 0;
    br->full_height_ = 0;
    br->cacheline = 0;
    br->format_char_ = '@';
    br->column_char_ = '\t';
    br->first = br->last = br->cache = NULL;

    self->pattern_ = "*";
    self->directory_ = "";
    self->iconsize_ = (uchar)(3 * Fl_Browser__textsize(&br->browser_) / 2);
    self->filetype_ = FL_FILE_BROWSER_FILES;
}

Fl_File_Browser *Fl_File_Browser_new(int x, int y, int w, int h, const char *label) {
    Fl_File_Browser *self = (Fl_File_Browser *)malloc(sizeof(Fl_File_Browser));
    Fl_File_Browser_init(self, x, y, w, h, label);
    return self;
}

void Fl_File_Browser_set_iconsize(Fl_File_Browser *self, uchar s) {
    self->iconsize_ = s;
    Fl_Widget_redraw(&self->browser.browser_.group.widget);
}

void Fl_File_Browser_set_filter(Fl_File_Browser *self, const char *pattern) {
    self->pattern_ = pattern ? pattern : "*";
}

int Fl_File_Browser_load(Fl_File_Browser *self, const char *directory, Fl_File_Sort_F *sort) {
    int num_files = 0, num_dirs;
    char filename[4096];

    if (!sort) sort = fl_numericsort;

    Fl_Browser_clear(&self->browser);
    self->directory_ = directory;
    if (!directory) return 0;

    if (directory[0] == '\0') {
        FILE *mtab;
        char line[FL_PATH_MAX];

        mtab = fopen("/etc/mnttab", "r");
        if (!mtab) mtab = fopen("/etc/mtab", "r");
        if (!mtab) mtab = fopen("/etc/fstab", "r");
        if (!mtab) mtab = fopen("/etc/vfstab", "r");

        if (mtab) {
            Fl_Browser_add(&self->browser, "/", NULL);
            num_files++;
            while (fgets(line, sizeof(line), mtab)) {
                if (line[0] == '#' || line[0] == '\n') continue;
                if (sscanf(line, "%*s%4095s", filename) != 1) continue;
                if (strcmp("/", filename) == 0) continue;
                strncat(filename, "/", sizeof(filename) - strlen(filename) - 1);
                Fl_Browser_add(&self->browser, filename, NULL);
                num_files++;
            }
            fclose(mtab);
        } else {
            Fl_Browser_add(&self->browser, "/", NULL);
        }
        return num_files;
    }

    {
        struct dirent **files;
        int i;

        num_files = fl_filename_list(directory, &files, sort);
        if (num_files <= 0) return 0;

        for (i = 0, num_dirs = 0; i < num_files; i++) {
            if (strcmp(files[i]->d_name, "./") != 0) {
                snprintf(filename, sizeof(filename), "%s/%s", directory, files[i]->d_name);

                if (fl_filename_isdir_quick(filename)) {
                    num_dirs++;
                    Fl_Browser_insert(&self->browser, num_dirs, files[i]->d_name, NULL);
                } else if (self->filetype_ == FL_FILE_BROWSER_FILES && fl_filename_match(files[i]->d_name, self->pattern_)) {
                    Fl_Browser_add(&self->browser, files[i]->d_name, NULL);
                }
            }
            free(files[i]);
        }
        free(files);
    }

    return num_files;
}
