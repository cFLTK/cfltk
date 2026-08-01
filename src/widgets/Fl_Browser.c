/*
 * cfltk - Fl_Browser.c
 * See include/cfltk/Fl_Browser.h for the class-conversion notes.
 * Translated from src/Fl_Browser.cxx (icon support dropped, no
 * Fl_Image yet -- see header).
 */
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "cfltk/Fl_Browser.h"
#include "cfltk/Fl.h"
#include "cfltk/fl_draw.h"

#define SELECTED 1
#define NOTDISPLAYED 2

static Fl_Group *Fl_Browser_as_group(Fl_Widget *self) { return Fl_Browser__as_group(self); }

static int browser_item_height(Fl_Browser_ *base, void *item);

/* -------------------------------------------------------------------
 * Item-ops: FL_BLINE linked list
 * ---------------------------------------------------------------- */

static void *browser_item_first(Fl_Browser_ *base) { return ((Fl_Browser *)base)->first; }
static void *browser_item_next(Fl_Browser_ *base, void *item) { (void)base; return ((FL_BLINE *)item)->next; }
static void *browser_item_prev(Fl_Browser_ *base, void *item) { (void)base; return ((FL_BLINE *)item)->prev; }
static void *browser_item_last(Fl_Browser_ *base) { return ((Fl_Browser *)base)->last; }

static int browser_item_selected(Fl_Browser_ *base, void *item) {
    (void)base;
    return ((FL_BLINE *)item)->flags & SELECTED;
}
static void browser_item_select(Fl_Browser_ *base, void *item, int val) {
    (void)base;
    if (val) ((FL_BLINE *)item)->flags |= SELECTED;
    else ((FL_BLINE *)item)->flags &= (char)~SELECTED;
}

static const char *browser_item_text(Fl_Browser_ *base, void *item) {
    (void)base;
    return ((FL_BLINE *)item)->txt;
}

/* Returns line for `line` (1-based), using a one-entry cache like
 * upstream to make sequential/nearby access fast; see the "This call is
 * slow" efficiency note upstream carries on find_line(). */
static FL_BLINE *find_line(Fl_Browser *self, int line) {
    int n;
    FL_BLINE *l;
    if (line == self->cacheline) return self->cache;
    if (self->cacheline && line > (self->cacheline / 2) && line < ((self->cacheline + self->lines) / 2)) {
        n = self->cacheline;
        l = self->cache;
    } else if (line <= (self->lines / 2)) {
        n = 1;
        l = self->first;
    } else {
        n = self->lines;
        l = self->last;
    }
    for (; n < line && l; n++) l = l->next;
    for (; n > line && l; n--) l = l->prev;
    self->cacheline = line;
    self->cache = l;
    return l;
}

static int lineno(Fl_Browser *self, void *item) {
    FL_BLINE *l = (FL_BLINE *)item;
    FL_BLINE *b, *f;
    int bnum, fnum, n;
    if (!l) return 0;
    if (l == self->cache) return self->cacheline;
    if (l == self->first) return 1;
    if (l == self->last) return self->lines;
    if (!self->cache) { self->cache = self->first; self->cacheline = 1; }
    b = self->cache->prev;
    bnum = self->cacheline - 1;
    f = self->cache->next;
    fnum = self->cacheline + 1;
    n = 0;
    for (;;) {
        if (b == l) { n = bnum; break; }
        if (f == l) { n = fnum; break; }
        if (b) { b = b->prev; bnum--; }
        if (f) { f = f->next; fnum++; }
    }
    self->cache = l;
    self->cacheline = n;
    return n;
}

static void insert_line(Fl_Browser *self, int line, FL_BLINE *item) {
    if (!self->first) {
        item->prev = item->next = NULL;
        self->first = self->last = item;
    } else if (line <= 1) {
        Fl_Browser__inserting(&self->browser_, self->first, item);
        item->prev = NULL;
        item->next = self->first;
        item->next->prev = item;
        self->first = item;
    } else if (line > self->lines) {
        item->prev = self->last;
        item->prev->next = item;
        item->next = NULL;
        self->last = item;
    } else {
        FL_BLINE *n = find_line(self, line);
        Fl_Browser__inserting(&self->browser_, n, item);
        item->next = n;
        item->prev = n->prev;
        item->prev->next = item;
        n->prev = item;
    }
    self->cacheline = line;
    self->cache = item;
    self->lines++;
    self->full_height_ += browser_item_height(&self->browser_, item);
    Fl_Browser__redraw_line(&self->browser_, item);
}

static FL_BLINE *remove_line(Fl_Browser *self, int line) {
    FL_BLINE *ttt = find_line(self, line);
    Fl_Browser__deleting(&self->browser_, ttt);

    self->cacheline = line - 1;
    self->cache = ttt->prev;
    self->lines--;
    self->full_height_ -= browser_item_height(&self->browser_, ttt);
    if (ttt->prev) ttt->prev->next = ttt->next; else self->first = ttt->next;
    if (ttt->next) ttt->next->prev = ttt->prev; else self->last = ttt->prev;
    return ttt;
}

void Fl_Browser_remove(Fl_Browser *self, int line) {
    if (line < 1 || line > self->lines) return;
    free(remove_line(self, line));
}

void Fl_Browser_insert(Fl_Browser *self, int line, const char *newtext, void *d) {
    FL_BLINE *t;
    int l;
    if (!newtext) newtext = "";
    l = (int)strlen(newtext);
    t = (FL_BLINE *)malloc(sizeof(FL_BLINE) + (size_t)l + 1);
    t->length = (short)l;
    t->flags = 0;
    strcpy(t->txt, newtext);
    t->data = d;
    insert_line(self, line, t);
}

void Fl_Browser_add(Fl_Browser *self, const char *newtext, void *d) {
    Fl_Browser_insert(self, self->lines + 1, newtext, d);
}

void Fl_Browser_move(Fl_Browser *self, int to, int from) {
    FL_BLINE *item;
    if (from < 1 || from > self->lines) return;
    item = remove_line(self, from);
    insert_line(self, to, item);
}

void Fl_Browser_set_text(Fl_Browser *self, int line, const char *newtext) {
    FL_BLINE *t;
    int l;
    if (line < 1 || line > self->lines) return;
    t = find_line(self, line);
    if (!newtext) newtext = "";
    l = (int)strlen(newtext);
    if (l > t->length) {
        FL_BLINE *n = (FL_BLINE *)malloc(sizeof(FL_BLINE) + (size_t)l + 1);
        Fl_Browser__replacing(&self->browser_, t, n);
        self->cache = n;
        n->data = t->data;
        n->length = (short)l;
        n->flags = t->flags;
        n->prev = t->prev;
        if (n->prev) n->prev->next = n; else self->first = n;
        n->next = t->next;
        if (n->next) n->next->prev = n; else self->last = n;
        free(t);
        t = n;
    }
    strcpy(t->txt, newtext);
    Fl_Browser__redraw_line(&self->browser_, t);
}

void Fl_Browser_set_data(Fl_Browser *self, int line, void *d) {
    if (line < 1 || line > self->lines) return;
    find_line(self, line)->data = d;
}

/* -------------------------------------------------------------------
 * '@'-format-code parsing shared by item_height/item_width/item_draw.
 * See Fl_Browser_format_char()'s doc comment in the upstream header
 * (reproduced in Fl_Browser.h) for the code list.
 * ---------------------------------------------------------------- */

static int browser_item_height(Fl_Browser_ *base, void *item) {
    Fl_Browser *self = (Fl_Browser *)base;
    FL_BLINE *l = (FL_BLINE *)item;
    int hmax = 2;

    if (l->flags & NOTDISPLAYED) return 0;

    if (!l->txt[0]) {
        int hh;
        fl_font(Fl_Browser__textfont(base), Fl_Browser__textsize(base));
        hh = fl_height();
        if (hh > hmax) hmax = hh;
    } else {
        const int *i = self->column_widths_;
        char *str;
        for (str = l->txt; str && *str; str++) {
            Fl_Font font = Fl_Browser__textfont(base);
            Fl_Fontsize tsize = Fl_Browser__textsize(base);
            char *ptr;
            while (*str == self->format_char_) {
                str++;
                switch (*str++) {
                    case 'l': case 'L': tsize = 24; break;
                    case 'm': case 'M': tsize = 18; break;
                    case 's': tsize = 11; break;
                    case 'b': font = (Fl_Font)(font | FL_BOLD); break;
                    case 'i': font = (Fl_Font)(font | FL_ITALIC); break;
                    case 'f': case 't': font = FL_COURIER; break;
                    case 'B': case 'C': while (isdigit((unsigned char)*str)) str++; break;
                    case 'F': font = (Fl_Font)strtol(str, &str, 10); break;
                    case 'S': tsize = (Fl_Fontsize)strtol(str, &str, 10); break;
                    case 0: case '@': str--; /* fall through */
                    case '.': goto END_FORMAT;
                    default: break;
                }
            }
        END_FORMAT:
            ptr = str;
            if (ptr && *i) { str = strchr(str, self->column_char_); i++; }
            else str = NULL;
            if ((!str && *ptr) || (str && ptr < str)) {
                int hh;
                fl_font(font, tsize);
                hh = fl_height();
                if (hh > hmax) hmax = hh;
            }
            if (!str || !*str) break;
        }
    }
    return hmax;
}

static int browser_item_width(Fl_Browser_ *base, void *item) {
    Fl_Browser *self = (Fl_Browser *)base;
    FL_BLINE *l = (FL_BLINE *)item;
    char *str = l->txt;
    const int *i = self->column_widths_;
    int ww = 0;
    Fl_Fontsize tsize = Fl_Browser__textsize(base);
    Fl_Font font = Fl_Browser__textfont(base);
    int done = 0;

    while (*i) {
        char *e = strchr(str, self->column_char_);
        if (!e) break;
        str = e + 1;
        ww += *i++;
    }

    while (*str == self->format_char_ && str[1] && str[1] != self->format_char_) {
        str++;
        switch (*str++) {
            case 'l': case 'L': tsize = 24; break;
            case 'm': case 'M': tsize = 18; break;
            case 's': tsize = 11; break;
            case 'b': font = (Fl_Font)(font | FL_BOLD); break;
            case 'i': font = (Fl_Font)(font | FL_ITALIC); break;
            case 'f': case 't': font = FL_COURIER; break;
            case 'B': case 'C': while (isdigit((unsigned char)*str)) str++; break;
            case 'F': font = (Fl_Font)strtol(str, &str, 10); break;
            case 'S': tsize = (Fl_Fontsize)strtol(str, &str, 10); break;
            case '.': done = 1; break;
            case '@': str--; done = 1; break;
            default: break;
        }
        if (done) break;
    }

    if (*str == self->format_char_ && str[1]) str++;

    fl_font(font, tsize);
    return ww + (int)fl_width_str(str) + 6;
}

static void browser_item_draw(Fl_Browser_ *base, void *item, int X, int Y, int W, int H) {
    Fl_Browser *self = (Fl_Browser *)base;
    Fl_Widget *bw = &base->group.widget;
    FL_BLINE *l = (FL_BLINE *)item;
    char *str = l->txt;
    const int *i = self->column_widths_;

    while (W > 6) {
        int w1 = W;
        char *e = NULL;
        Fl_Fontsize tsize = Fl_Browser__textsize(base);
        Fl_Font font = Fl_Browser__textfont(base);
        Fl_Color lcol = Fl_Browser__textcolor(base);
        Fl_Align talign = FL_ALIGN_LEFT;

        if (*i) {
            e = strchr(str, self->column_char_);
            if (e) { *e = 0; w1 = *i++; }
        }

        while (*str == self->format_char_ && *(++str) && *str != self->format_char_) {
            switch (*str++) {
                case 'l': case 'L': tsize = 24; break;
                case 'm': case 'M': tsize = 18; break;
                case 's': tsize = 11; break;
                case 'b': font = (Fl_Font)(font | FL_BOLD); break;
                case 'i': font = (Fl_Font)(font | FL_ITALIC); break;
                case 'f': case 't': font = FL_COURIER; break;
                case 'c': talign = FL_ALIGN_CENTER; break;
                case 'r': talign = FL_ALIGN_RIGHT; break;
                case 'B':
                    if (!(l->flags & SELECTED)) {
                        fl_color((Fl_Color)strtoul(str, &str, 10));
                        fl_rectf(X, Y, w1, H);
                    } else {
                        while (isdigit((unsigned char)*str)) str++;
                    }
                    break;
                case 'C': lcol = (Fl_Color)strtoul(str, &str, 10); break;
                case 'F': font = (Fl_Font)strtol(str, &str, 10); break;
                case 'N': lcol = FL_INACTIVE_COLOR; break;
                case 'S': tsize = (Fl_Fontsize)strtol(str, &str, 10); break;
                case '-':
                    fl_color(FL_DARK3);
                    fl_line(X + 3, Y + H / 2, X + w1 - 3, Y + H / 2);
                    fl_color(FL_LIGHT3);
                    fl_line(X + 3, Y + H / 2 + 1, X + w1 - 3, Y + H / 2 + 1);
                    break;
                case 'u': case '_':
                    fl_color(lcol);
                    fl_line(X + 3, Y + H - 1, X + w1 - 3, Y + H - 1);
                    break;
                case '.': goto BREAK;
                case '@': str--; goto BREAK;
                default: break;
            }
        }
    BREAK:
        fl_font(font, tsize);
        if (l->flags & SELECTED) lcol = fl_contrast(lcol, Fl_Widget_selection_color(bw));
        if (!Fl_Widget_active_r(bw)) lcol = fl_inactive(lcol);
        {
            Fl_Label lbl;
            Fl_Align align = e ? (Fl_Align)(talign | FL_ALIGN_CLIP) : talign;
            lbl.value = str;
            lbl.image = NULL;
            lbl.deimage = NULL;
            lbl.type = FL_NORMAL_LABEL;
            lbl.font = font;
            lbl.size = tsize;
            lbl.color = lcol;
            lbl.align = align;
            fl_label_draw(&lbl, X + 3, Y, w1 - 6, H, align);
        }
        if (!e) break;
        *e = self->column_char_;
        X += w1;
        W -= w1;
        str = e + 1;
    }
}

static int browser_full_height(Fl_Browser_ *base) { return ((Fl_Browser *)base)->full_height_; }
static int browser_incr_height(Fl_Browser_ *base) { return Fl_Browser__textsize(base) + 2; }

static void browser_item_swap(Fl_Browser_ *base, void *a, void *b);
static void *browser_item_at(Fl_Browser_ *base, int index) { return find_line((Fl_Browser *)base, index); }

const Fl_Browser_ItemOps fl_browser_item_ops = {
    browser_item_first,
    browser_item_next,
    browser_item_prev,
    browser_item_last,
    browser_item_height,
    browser_item_width,
    NULL, /* item_quick_height: default (== item_height) is fine, upstream doesn't override it either */
    browser_item_draw,
    browser_item_text,
    browser_item_swap,
    browser_item_at,
    browser_item_select,
    browser_item_selected,
    NULL, /* full_width: default (tracked max_width) matches upstream's own default */
    browser_full_height,
    browser_incr_height
};

const Fl_WidgetOps fl_browser_ops = {
    Fl_Browser__draw,
    Fl_Browser__handle,
    Fl_Browser__resize,
    NULL, NULL,
    Fl_Browser_destroy,
    Fl_Browser_as_group,
    NULL
};

static const int no_columns[1] = { 0 };

void Fl_Browser_init(Fl_Browser *self, int x, int y, int w, int h, const char *label) {
    Fl_Browser__init(&self->browser_, &fl_browser_ops, &fl_browser_item_ops, x, y, w, h, label);
    self->column_widths_ = no_columns;
    self->lines = 0;
    self->full_height_ = 0;
    self->cacheline = 0;
    self->format_char_ = '@';
    self->column_char_ = '\t';
    self->first = self->last = self->cache = NULL;
}

Fl_Browser *Fl_Browser_new(int x, int y, int w, int h, const char *label) {
    Fl_Browser *self = (Fl_Browser *)malloc(sizeof(Fl_Browser));
    Fl_Browser_init(self, x, y, w, h, label);
    return self;
}

void Fl_Browser_clear(Fl_Browser *self) {
    FL_BLINE *l = self->first;
    while (l) {
        FL_BLINE *n = l->next;
        free(l);
        l = n;
    }
    self->full_height_ = 0;
    self->first = NULL;
    self->last = NULL;
    self->lines = 0;
    Fl_Browser__new_list(&self->browser_);
}

void Fl_Browser_destroy(Fl_Widget *self_w) {
    Fl_Browser *self = (Fl_Browser *)self_w;
    FL_BLINE *l = self->first;
    while (l) {
        FL_BLINE *n = l->next;
        free(l);
        l = n;
    }
    Fl_Browser__destroy(self_w);
}

static void browser_item_swap(Fl_Browser_ *base, void *va, void *vb) {
    Fl_Browser *self = (Fl_Browser *)base;
    FL_BLINE *a = (FL_BLINE *)va;
    FL_BLINE *b = (FL_BLINE *)vb;
    FL_BLINE *aprev, *anext, *bprev, *bnext;

    if (a == b || !a || !b) return;
    Fl_Browser__swapping(base, a, b);
    aprev = a->prev; anext = a->next;
    bprev = b->prev; bnext = b->next;
    if (b->prev == a) {
        if (aprev) aprev->next = b; else self->first = b;
        b->next = a;
        a->next = bnext;
        b->prev = aprev;
        a->prev = b;
        if (bnext) bnext->prev = a; else self->last = a;
    } else if (a->prev == b) {
        if (bprev) bprev->next = a; else self->first = a;
        a->next = b;
        b->next = anext;
        a->prev = bprev;
        b->prev = a;
        if (anext) anext->prev = b; else self->last = b;
    } else {
        b->prev = aprev;
        if (anext) anext->prev = b; else self->last = b;
        a->prev = bprev;
        if (bnext) bnext->prev = a; else self->last = a;
        if (aprev) aprev->next = b; else self->first = b;
        b->next = anext;
        if (bprev) bprev->next = a; else self->first = a;
        a->next = bnext;
    }
    self->cacheline = 0;
    self->cache = NULL;
}

void Fl_Browser_swap(Fl_Browser *self, int a, int b) {
    if (a < 1 || a > self->lines || b < 1 || b > self->lines) return;
    browser_item_swap(&self->browser_, find_line(self, a), find_line(self, b));
}

void Fl_Browser_lineposition(Fl_Browser *self, int line, Fl_Browser_Line_Position pos) {
    FL_BLINE *l;
    int p = 0;
    int final_pos, X, Y, W, H;

    if (line < 1) line = 1;
    if (line > self->lines) line = self->lines;

    for (l = self->first; l && line > 1; l = l->next) {
        line--;
        p += browser_item_height(&self->browser_, l);
    }
    if (l && pos == FL_BROWSER_LINE_BOTTOM) p += browser_item_height(&self->browser_, l);

    final_pos = p;
    Fl_Browser__bbox(&self->browser_, &X, &Y, &W, &H);

    switch (pos) {
        case FL_BROWSER_LINE_TOP: break;
        case FL_BROWSER_LINE_BOTTOM: final_pos -= H; break;
        case FL_BROWSER_LINE_MIDDLE: final_pos -= H / 2; break;
    }

    if (final_pos > (browser_full_height(&self->browser_) - H)) final_pos = browser_full_height(&self->browser_) - H;
    Fl_Browser__set_position(&self->browser_, final_pos);
}

int Fl_Browser_topline(const Fl_Browser *self) { return lineno((Fl_Browser *)self, Fl_Browser__top(&self->browser_)); }

void Fl_Browser_set_textsize(Fl_Browser *self, Fl_Fontsize new_size) {
    FL_BLINE *itm;
    if (new_size == Fl_Browser__textsize(&self->browser_)) return;
    Fl_Browser__set_textsize(&self->browser_, new_size);
    Fl_Browser__new_list(&self->browser_);
    self->full_height_ = 0;
    if (self->lines == 0) return;
    for (itm = self->first; itm; itm = itm->next) self->full_height_ += browser_item_height(&self->browser_, itm);
}

int Fl_Browser_select(Fl_Browser *self, int line, int val) {
    if (line < 1 || line > self->lines) return 0;
    return Fl_Browser__select(&self->browser_, find_line(self, line), val, 0);
}

int Fl_Browser_selected(const Fl_Browser *self, int line) {
    if (line < 1 || line > self->lines) return 0;
    return find_line((Fl_Browser *)self, line)->flags & SELECTED;
}

void Fl_Browser_show_line(Fl_Browser *self, int line) {
    FL_BLINE *t = find_line(self, line);
    if (!t) return;
    if (t->flags & NOTDISPLAYED) {
        t->flags &= (char)~NOTDISPLAYED;
        self->full_height_ += browser_item_height(&self->browser_, t);
        if (Fl_Browser__displayed(&self->browser_, t)) Fl_Widget_redraw(&self->browser_.group.widget);
    }
}

void Fl_Browser_hide_line(Fl_Browser *self, int line) {
    FL_BLINE *t = find_line(self, line);
    if (!t) return;
    if (!(t->flags & NOTDISPLAYED)) {
        self->full_height_ -= browser_item_height(&self->browser_, t);
        t->flags |= NOTDISPLAYED;
        if (Fl_Browser__displayed(&self->browser_, t)) Fl_Widget_redraw(&self->browser_.group.widget);
    }
}

void Fl_Browser_display_line(Fl_Browser *self, int line, int val) {
    if (line < 1 || line > self->lines) return;
    if (val) Fl_Browser_show_line(self, line);
    else Fl_Browser_hide_line(self, line);
}

int Fl_Browser_visible_line(const Fl_Browser *self, int line) {
    if (line < 1 || line > self->lines) return 0;
    return !(find_line((Fl_Browser *)self, line)->flags & NOTDISPLAYED);
}

int Fl_Browser_value(const Fl_Browser *self) { return lineno((Fl_Browser *)self, Fl_Browser__selection(&self->browser_)); }

const char *Fl_Browser_text(const Fl_Browser *self, int line) {
    if (line < 1 || line > self->lines) return NULL;
    return find_line((Fl_Browser *)self, line)->txt;
}

void *Fl_Browser_data(const Fl_Browser *self, int line) {
    if (line < 1 || line > self->lines) return NULL;
    return find_line((Fl_Browser *)self, line)->data;
}

int Fl_Browser_displayed_line(const Fl_Browser *self, int line) {
    return Fl_Browser__displayed(&self->browser_, find_line((Fl_Browser *)self, line));
}

void Fl_Browser_make_visible(Fl_Browser *self, int line) {
    if (line < 1) Fl_Browser__display(&self->browser_, find_line(self, 1));
    else if (line > self->lines) Fl_Browser__display(&self->browser_, find_line(self, self->lines));
    else Fl_Browser__display(&self->browser_, find_line(self, line));
}
