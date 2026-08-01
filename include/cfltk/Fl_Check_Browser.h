/*
 * cfltk - Fl_Check_Browser.h
 *
 * C translation of FLTK 1.3 FL/Fl_Check_Browser.H.
 *
 * Original class : Fl_Check_Browser : public Fl_Browser_ (concrete; a
 *                   scrolling list of text lines each with its own
 *                   checkbox, toggled by clicking anywhere on the
 *                   line -- independent of Fl_Browser_'s own
 *                   selection() highlight, which this class never
 *                   actually shows: item_selected() always reports
 *                   false, since nothing in upstream ever sets a
 *                   line's `selected` field to true. Checking the box
 *                   *is* the interaction; there is no persistent
 *                   "current line" highlight bar. See item_select()
 *                   in Fl_Check_Browser.c for exactly how a click
 *                   ends up toggling `checked` without that no-op
 *                   `selected` field ever coming into it).
 * New C structure : struct Fl_Check_Browser { Fl_Browser_ browser_;
 *                    Fl_Check_Browser_Item *first, *last, *cache; int
 *                    cached_item, nitems_, nchecked_; }. Each line is a
 *                    separately malloc'd, strdup'd-text doubly-linked
 *                    node (Fl_Check_Browser_Item), simpler than
 *                    Fl_Browser's FL_BLINE since there's no tab-column/
 *                    '@'-format-code support to carry.
 * Vtbl            : fl_check_browser_ops -- reuses Fl_Browser__draw()/
 *                    _resize() verbatim, but wraps handle() (a plain
 *                    click always clears any leftover selection_ first,
 *                    per upstream) and supplies its own destroy() to
 *                    free every item.
 * Ownership       : owns every Fl_Check_Browser_Item node (and its
 *                    strdup'd text), freed in destroy()/clear()/
 *                    remove().
 */
#ifndef CFLTK_FL_CHECK_BROWSER_H
#define CFLTK_FL_CHECK_BROWSER_H

#include "cfltk/Fl_Browser_.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Fl_Check_Browser_Item {
    struct Fl_Check_Browser_Item *next, *prev;
    char checked;
    char selected; /* never set true by this class -- see header note */
    char *text;    /* owned, strdup()'d */
} Fl_Check_Browser_Item;

typedef struct Fl_Check_Browser {
    Fl_Browser_ browser_;
    Fl_Check_Browser_Item *first, *last, *cache;
    int cached_item;
    int nitems_, nchecked_;
} Fl_Check_Browser;

extern const Fl_WidgetOps fl_check_browser_ops;

void Fl_Check_Browser_init(Fl_Check_Browser *self, int x, int y, int w, int h, const char *label);
Fl_Check_Browser *Fl_Check_Browser_new(int x, int y, int w, int h, const char *label);
void Fl_Check_Browser_destroy(Fl_Widget *self);

/* Adds an (unchecked, or checked if b != 0) line at the end; text is
 * copied. Returns the new nitems(). */
int Fl_Check_Browser_add(Fl_Check_Browser *self, const char *s);
int Fl_Check_Browser_add_checked(Fl_Check_Browser *self, const char *s, int b);
/* Removes line `item` (1-based); returns nitems() afterward. */
int Fl_Check_Browser_remove(Fl_Check_Browser *self, int item);
void Fl_Check_Browser_clear(Fl_Check_Browser *self);

static inline int Fl_Check_Browser_nitems(const Fl_Check_Browser *self) { return self->nitems_; }
static inline int Fl_Check_Browser_nchecked(const Fl_Check_Browser *self) { return self->nchecked_; }

int Fl_Check_Browser_checked(const Fl_Check_Browser *self, int item);
void Fl_Check_Browser_set_checked_val(Fl_Check_Browser *self, int item, int b);
static inline void Fl_Check_Browser_set_checked(Fl_Check_Browser *self, int item) { Fl_Check_Browser_set_checked_val(self, item, 1); }
void Fl_Check_Browser_check_all(Fl_Check_Browser *self);
void Fl_Check_Browser_check_none(Fl_Check_Browser *self);

int Fl_Check_Browser_value(const Fl_Check_Browser *self);
const char *Fl_Check_Browser_text(const Fl_Check_Browser *self, int item);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_CHECK_BROWSER_H */
