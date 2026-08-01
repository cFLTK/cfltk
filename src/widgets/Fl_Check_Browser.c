/*
 * cfltk - Fl_Check_Browser.c
 * See include/cfltk/Fl_Check_Browser.h for the class-conversion notes.
 * Translated from src/Fl_Check_Browser.cxx.
 */
#include <stdlib.h>
#include <string.h>

#include "cfltk/Fl_Check_Browser.h"
#include "cfltk/Fl.h"
#include "cfltk/fl_draw.h"

static Fl_Group *Fl_Check_Browser_as_group(Fl_Widget *self) { return Fl_Browser__as_group(self); }

/* Uses a one-entry cache for fast sequential/nearby access, same idea
 * as Fl_Browser's find_line(). */
static Fl_Check_Browser_Item *find_item(const Fl_Check_Browser *self, int n) {
    Fl_Check_Browser *mself = (Fl_Check_Browser *)self;
    int i = n;
    Fl_Check_Browser_Item *p = self->first;

    if (n <= 0 || n > self->nitems_ || p == NULL) return NULL;

    if (n == self->cached_item) { p = self->cache; n = 1; }
    else if (n == self->cached_item + 1) { p = self->cache->next; n = 1; }
    else if (n == self->cached_item - 1) { p = self->cache->prev; n = 1; }

    while (--n) p = p->next;

    mself->cache = p;
    mself->cached_item = i;
    return p;
}

static int lineno(const Fl_Check_Browser *self, Fl_Check_Browser_Item *p0) {
    Fl_Check_Browser_Item *p = self->first;
    int i = 1;
    if (!p) return 0;
    while (p) {
        if (p == p0) return i;
        i++;
        p = p->next;
    }
    return 0;
}

/* -------------------------------------------------------------------
 * Item-ops
 * ---------------------------------------------------------------- */

static void *cb_item_first(Fl_Browser_ *base) { return ((Fl_Check_Browser *)base)->first; }
static void *cb_item_next(Fl_Browser_ *base, void *item) { (void)base; return ((Fl_Check_Browser_Item *)item)->next; }
static void *cb_item_prev(Fl_Browser_ *base, void *item) { (void)base; return ((Fl_Check_Browser_Item *)item)->prev; }

static int cb_item_height(Fl_Browser_ *base, void *item) { (void)item; return Fl_Browser__textsize(base) + 2; }

static int cb_item_width(Fl_Browser_ *base, void *v) {
    int check_size = Fl_Browser__textsize(base) - 2;
    fl_font(Fl_Browser__textfont(base), Fl_Browser__textsize(base));
    return (int)fl_width_str(((Fl_Check_Browser_Item *)v)->text) + check_size + 8;
}

static void cb_item_draw(Fl_Browser_ *base, void *v, int X, int Y, int W, int H) {
    Fl_Widget *bw = &base->group.widget;
    Fl_Check_Browser_Item *i = (Fl_Check_Browser_Item *)v;
    char *s = i->text;
    int tsize = Fl_Browser__textsize(base);
    int check_size = tsize - 2;
    Fl_Color col = Fl_Widget_active_r(bw) ? Fl_Browser__textcolor(base) : fl_inactive(Fl_Browser__textcolor(base));
    int cy = Y + (tsize + 1 - check_size) / 2;
    (void)W;
    (void)H;
    X += 2;

    fl_color(Fl_Widget_active_r(bw) ? FL_FOREGROUND_COLOR : fl_inactive(FL_FOREGROUND_COLOR));
    fl_rect(X, cy, check_size, check_size);
    if (i->checked) {
        int tx = X + 3;
        int tw = check_size - 4;
        int d1 = tw / 3;
        int d2 = tw - d1;
        int ty = cy + (check_size + d2) / 2 - d1 - 2;
        int n;
        for (n = 0; n < 3; n++, ty++) {
            fl_line(tx, ty, tx + d1, ty + d1);
            fl_line(tx + d1, ty + d1, tx + tw - 1, ty + d1 - d2 + 1);
        }
    }
    fl_font(Fl_Browser__textfont(base), tsize);
    if (i->selected) col = fl_contrast(col, Fl_Widget_selection_color(bw));
    fl_color(col);
    fl_draw(s, X + check_size + 8, Y + tsize - 1);
}

static void cb_item_select(Fl_Browser_ *base, void *v, int state) {
    Fl_Check_Browser *self = (Fl_Check_Browser *)base;
    Fl_Check_Browser_Item *i = (Fl_Check_Browser_Item *)v;
    if (state) {
        if (i->checked) { i->checked = 0; self->nchecked_--; }
        else { i->checked = 1; self->nchecked_++; }
    }
}

static int cb_item_selected(Fl_Browser_ *base, void *v) { (void)base; return ((Fl_Check_Browser_Item *)v)->selected; }

const Fl_Browser_ItemOps fl_check_browser_item_ops = {
    cb_item_first,
    cb_item_next,
    cb_item_prev,
    NULL, /* item_last: default (NULL->0) matches upstream (not overridden) */
    cb_item_height,
    cb_item_width,
    NULL, /* item_quick_height: default == item_height, fine */
    cb_item_draw,
    NULL, /* item_text: not overridden upstream either (no sort() support) */
    NULL, /* item_swap: not overridden upstream */
    NULL, /* item_at: not overridden upstream */
    cb_item_select,
    cb_item_selected,
    NULL, /* full_width: default tracked max_width, matches upstream default */
    NULL, /* full_height: default sum-of-heights, matches upstream default */
    NULL  /* incr_height: default quick_height(first()), matches upstream default */
};

/* -------------------------------------------------------------------
 * Widget ops
 * ---------------------------------------------------------------- */

static int Fl_Check_Browser_handle(Fl_Widget *self_w, int event) {
    if (event == FL_PUSH) Fl_Browser__deselect((Fl_Browser_ *)self_w, 0);
    return Fl_Browser__handle(self_w, event);
}

void Fl_Check_Browser_destroy(Fl_Widget *self_w) {
    Fl_Check_Browser *self = (Fl_Check_Browser *)self_w;
    Fl_Check_Browser_Item *p = self->first;
    while (p) {
        Fl_Check_Browser_Item *next = p->next;
        free(p->text);
        free(p);
        p = next;
    }
    Fl_Browser__destroy(self_w);
}

const Fl_WidgetOps fl_check_browser_ops = {
    Fl_Browser__draw,
    Fl_Check_Browser_handle,
    Fl_Browser__resize,
    NULL, NULL,
    Fl_Check_Browser_destroy,
    Fl_Check_Browser_as_group,
    NULL
};

void Fl_Check_Browser_init(Fl_Check_Browser *self, int x, int y, int w, int h, const char *label) {
    Fl_Browser__init(&self->browser_, &fl_check_browser_ops, &fl_check_browser_item_ops, x, y, w, h, label);
    Fl_Widget_set_type(&self->browser_.group.widget, FL_SELECT_BROWSER);
    Fl_Widget_set_when(&self->browser_.group.widget, FL_WHEN_NEVER);
    self->first = self->last = NULL;
    self->nitems_ = self->nchecked_ = 0;
    self->cached_item = -1;
    self->cache = NULL;
}

Fl_Check_Browser *Fl_Check_Browser_new(int x, int y, int w, int h, const char *label) {
    Fl_Check_Browser *self = (Fl_Check_Browser *)malloc(sizeof(Fl_Check_Browser));
    Fl_Check_Browser_init(self, x, y, w, h, label);
    return self;
}

/* -------------------------------------------------------------------
 * Public line API
 * ---------------------------------------------------------------- */

int Fl_Check_Browser_add_checked(Fl_Check_Browser *self, const char *s, int b) {
    Fl_Check_Browser_Item *p = (Fl_Check_Browser_Item *)malloc(sizeof(Fl_Check_Browser_Item));
    size_t len;
    if (!s) s = "";
    len = strlen(s);
    p->next = NULL;
    p->prev = NULL;
    p->checked = (char)b;
    p->selected = 0;
    p->text = (char *)malloc(len + 1);
    memcpy(p->text, s, len + 1);

    if (b) self->nchecked_++;

    if (self->last == NULL) {
        self->first = self->last = p;
    } else {
        self->last->next = p;
        p->prev = self->last;
        self->last = p;
    }
    self->nitems_++;
    return self->nitems_;
}

int Fl_Check_Browser_add(Fl_Check_Browser *self, const char *s) { return Fl_Check_Browser_add_checked(self, s, 0); }

int Fl_Check_Browser_remove(Fl_Check_Browser *self, int item) {
    Fl_Check_Browser_Item *p = find_item(self, item);
    if (p) {
        Fl_Browser__deleting(&self->browser_, p);
        if (p->checked) self->nchecked_--;

        if (p->prev) p->prev->next = p->next; else self->first = p->next;
        if (p->next) p->next->prev = p->prev; else self->last = p->prev;

        free(p->text);
        free(p);

        self->nitems_--;
        self->cached_item = -1;
    }
    return self->nitems_;
}

void Fl_Check_Browser_clear(Fl_Check_Browser *self) {
    Fl_Check_Browser_Item *p = self->first;
    if (!p) return;
    Fl_Browser__new_list(&self->browser_);
    do {
        Fl_Check_Browser_Item *next = p->next;
        free(p->text);
        free(p);
        p = next;
    } while (p);
    self->first = self->last = NULL;
    self->nitems_ = self->nchecked_ = 0;
    self->cached_item = -1;
}

int Fl_Check_Browser_checked(const Fl_Check_Browser *self, int item) {
    Fl_Check_Browser_Item *p = find_item(self, item);
    return p ? p->checked : 0;
}

void Fl_Check_Browser_set_checked_val(Fl_Check_Browser *self, int item, int b) {
    Fl_Check_Browser_Item *p = find_item(self, item);
    if (p && (p->checked ^ b)) {
        p->checked = (char)b;
        if (b) self->nchecked_++; else self->nchecked_--;
        Fl_Widget_redraw(&self->browser_.group.widget);
    }
}

int Fl_Check_Browser_value(const Fl_Check_Browser *self) {
    return lineno(self, (Fl_Check_Browser_Item *)Fl_Browser__selection(&self->browser_));
}

const char *Fl_Check_Browser_text(const Fl_Check_Browser *self, int item) {
    Fl_Check_Browser_Item *p = find_item(self, item);
    return p ? p->text : NULL;
}

void Fl_Check_Browser_check_all(Fl_Check_Browser *self) {
    Fl_Check_Browser_Item *p;
    self->nchecked_ = self->nitems_;
    for (p = self->first; p; p = p->next) p->checked = 1;
    Fl_Widget_redraw(&self->browser_.group.widget);
}

void Fl_Check_Browser_check_none(Fl_Check_Browser *self) {
    Fl_Check_Browser_Item *p;
    self->nchecked_ = 0;
    for (p = self->first; p; p = p->next) p->checked = 0;
    Fl_Widget_redraw(&self->browser_.group.widget);
}
