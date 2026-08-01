/*
 * cfltk - Fl_Browser.h
 *
 * C translation of FLTK 1.3 FL/Fl_Browser.H.
 *
 * Original class : Fl_Browser : public Fl_Browser_ (concrete;
 *                   implements the item_*() virtuals over a doubly-
 *                   linked list of "FL_BLINE" nodes, each holding a
 *                   nul-terminated text string plus a user `void *`
 *                   data pointer; lines are 1-based). Supports tab-
 *                   separated multi-column layout via column_widths()/
 *                   column_char(), and a '@'-prefixed inline format-code
 *                   mini-language per column (font/size/color/align/
 *                   underline/strikethrough/background-fill) -- see
 *                   Fl_Browser_format_char() below for the code list.
 * New C structure : struct Fl_Browser { Fl_Browser_ browser_; FL_BLINE
 *                    *first, *last, *cache; int cacheline, lines,
 *                    full_height_; const int *column_widths_; char
 *                    format_char_, column_char_; }; embeds Fl_Browser_
 *                    as its first member. FL_BLINE itself uses a C99
 *                    flexible array member (`char txt[]`) in place of
 *                    upstream's `char txt[1]` over-allocation trick --
 *                    same allocation pattern (`malloc(sizeof(FL_BLINE) +
 *                    len + 1)`), just the standard-conforming spelling.
 * Vtbl            : fl_browser_ops, reusing Fl_Browser__draw/_handle/
 *                    _resize verbatim (see Fl_Browser_.h) with its own
 *                    destroy() to free every FL_BLINE. Item-ops table
 *                    fl_browser_item_ops implements the FL_BLINE list
 *                    operations.
 * Ownership       : owns every FL_BLINE node (freed in destroy()/
 *                    clear()/remove()/text()-replace/move()).
 * Known differences:
 *   - No icon support (`Fl_Browser::icon()`/upstream's `Fl_Image *icon`
 *     field on each line) -- cfltk has no Fl_Image yet (see
 *     docs/DESIGN.md's "No images" note). FL_BLINE has no icon field;
 *     item_height()/item_width()/item_draw() skip the icon-measurement/
 *     drawing branches entirely. Straightforward to add once Fl_Image
 *     exists -- the upstream logic to port back in is marked in
 *     Fl_Browser.c.
 *   - load(const char *filename) (upstream: reads a text file, one
 *     line per browser line) is not ported -- no client needs it yet
 *     and it's a thin wrapper over fopen()/fgets()/add() a caller can
 *     trivially write themselves against the public add() API.
 */
#ifndef CFLTK_FL_BROWSER_H
#define CFLTK_FL_BROWSER_H

#include "cfltk/Fl_Browser_.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FL_BLINE {
    struct FL_BLINE *prev, *next;
    void *data;
    short length; /* strlen(txt), may be less than the allocated capacity */
    char flags;   /* SELECTED | NOTDISPLAYED, see Fl_Browser.c */
    char txt[];   /* flexible array member: nul-terminated text */
} FL_BLINE;

typedef struct Fl_Browser {
    Fl_Browser_ browser_;
    FL_BLINE *first;
    FL_BLINE *last;
    FL_BLINE *cache;
    int cacheline;
    int lines;
    int full_height_;
    const int *column_widths_;
    char format_char_;
    char column_char_;
} Fl_Browser;

extern const Fl_WidgetOps fl_browser_ops;
extern const Fl_Browser_ItemOps fl_browser_item_ops;

void Fl_Browser_init(Fl_Browser *self, int x, int y, int w, int h, const char *label);
Fl_Browser *Fl_Browser_new(int x, int y, int w, int h, const char *label);
void Fl_Browser_destroy(Fl_Widget *self);

void Fl_Browser_add(Fl_Browser *self, const char *newtext, void *d);
void Fl_Browser_insert(Fl_Browser *self, int line, const char *newtext, void *d);
void Fl_Browser_remove(Fl_Browser *self, int line);
void Fl_Browser_move(Fl_Browser *self, int to, int from);
void Fl_Browser_swap(Fl_Browser *self, int a, int b);
void Fl_Browser_clear(Fl_Browser *self);

static inline int Fl_Browser_size(const Fl_Browser *self) { return self->lines; }

void Fl_Browser_set_textsize(Fl_Browser *self, Fl_Fontsize new_size);

typedef enum Fl_Browser_Line_Position { FL_BROWSER_LINE_TOP, FL_BROWSER_LINE_BOTTOM, FL_BROWSER_LINE_MIDDLE } Fl_Browser_Line_Position;

int Fl_Browser_topline(const Fl_Browser *self);
void Fl_Browser_lineposition(Fl_Browser *self, int line, Fl_Browser_Line_Position pos);
static inline void Fl_Browser_set_topline(Fl_Browser *self, int line) { Fl_Browser_lineposition(self, line, FL_BROWSER_LINE_TOP); }
static inline void Fl_Browser_set_bottomline(Fl_Browser *self, int line) { Fl_Browser_lineposition(self, line, FL_BROWSER_LINE_BOTTOM); }
static inline void Fl_Browser_set_middleline(Fl_Browser *self, int line) { Fl_Browser_lineposition(self, line, FL_BROWSER_LINE_MIDDLE); }

int Fl_Browser_select(Fl_Browser *self, int line, int val);
int Fl_Browser_selected(const Fl_Browser *self, int line);
void Fl_Browser_show_line(Fl_Browser *self, int line);
void Fl_Browser_hide_line(Fl_Browser *self, int line);
void Fl_Browser_display_line(Fl_Browser *self, int line, int val);
int Fl_Browser_visible_line(const Fl_Browser *self, int line);

int Fl_Browser_value(const Fl_Browser *self);
static inline void Fl_Browser_set_value(Fl_Browser *self, int line) { Fl_Browser_select(self, line, 1); }
const char *Fl_Browser_text(const Fl_Browser *self, int line);
void Fl_Browser_set_text(Fl_Browser *self, int line, const char *newtext);
void *Fl_Browser_data(const Fl_Browser *self, int line);
void Fl_Browser_set_data(Fl_Browser *self, int line, void *d);

int Fl_Browser_displayed_line(const Fl_Browser *self, int line);
void Fl_Browser_make_visible(Fl_Browser *self, int line);

static inline char Fl_Browser_format_char(const Fl_Browser *self) { return self->format_char_; }
static inline void Fl_Browser_set_format_char(Fl_Browser *self, char c) { self->format_char_ = c; }
static inline char Fl_Browser_column_char(const Fl_Browser *self) { return self->column_char_; }
static inline void Fl_Browser_set_column_char(Fl_Browser *self, char c) { self->column_char_ = c; }
static inline const int *Fl_Browser_column_widths(const Fl_Browser *self) { return self->column_widths_; }
static inline void Fl_Browser_set_column_widths(Fl_Browser *self, const int *arr) { self->column_widths_ = arr; }

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_BROWSER_H */
