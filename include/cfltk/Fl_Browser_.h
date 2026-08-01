/*
 * cfltk - Fl_Browser_.h
 *
 * C translation of FLTK 1.3 FL/Fl_Browser_.H.
 *
 * Original class : Fl_Browser_ : public Fl_Group (abstract; own draw()/
 *                   handle()/resize(). Provides scrolling, selection,
 *                   and layout for a list of opaque `void *` items, but
 *                   has no idea how items are stored -- a subclass
 *                   supplies that via a set of pure-virtual item_*()
 *                   methods it must implement, most importantly
 *                   item_first()/item_next()/item_prev()/item_height()/
 *                   item_width()/item_draw()).
 * New C structure : struct Fl_Browser_ { Fl_Group group; const
 *                    Fl_Browser_ItemOps *item_ops; Fl_Scrollbar
 *                    scrollbar; Fl_Scrollbar hscrollbar; ...scroll
 *                    position/selection bookkeeping fields... };
 *                    embedding Fl_Group as its first member, same
 *                    layering as Fl_Window/Fl_Tabs/Fl_Scroll.
 * Inheritance     : Fl_Browser_ IS-A Fl_Group IS-A Fl_Widget through
 *                    embedding.
 * Vtbl            : Fl_Browser_'s pure-virtual item_*() methods become
 *                    a second, item-level vtable (`Fl_Browser_ItemOps`)
 *                    orthogonal to the widget-level `Fl_WidgetOps` --
 *                    this class genuinely has two independent axes of
 *                    virtual behavior (how it draws/handles as a
 *                    *widget*, inherited from Fl_Group and implemented
 *                    once here for every subclass to share; and how its
 *                    *items* are stored/measured/drawn, which is what
 *                    each subclass -- Fl_Browser, later Fl_Check_Browser
 *                    -- actually varies). Optional item-ops entries
 *                    (item_last/item_quick_height/item_text/item_swap/
 *                    item_at/item_select/item_selected/full_width/
 *                    full_height/incr_height) may be NULL, matching
 *                    upstream's virtuals that have a default body --
 *                    Fl_Browser_draw()/etc. fall back to the same
 *                    default behavior when the pointer is NULL.
 * Ownership       : owns the two embedded scrollbars exactly like
 *                    Fl_Scroll (see Fl_Scroll.h for why that's safe);
 *                    item storage is entirely the subclass's problem.
 * Known differences: same "no fl_scroll() blit, no color-scheme
 *                    background" notes as Fl_Scroll apply to nothing
 *                    here directly (Fl_Browser_ never had either), but
 *                    see Fl_Browser.h for what a concrete subclass
 *                    drops (icons -- no Fl_Image yet).
 */
#ifndef CFLTK_FL_BROWSER__H
#define CFLTK_FL_BROWSER__H

#include "cfltk/Fl_Group.h"
#include "cfltk/Fl_Scrollbar.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FL_NORMAL_BROWSER 0
#define FL_SELECT_BROWSER 1
#define FL_HOLD_BROWSER   2
#define FL_MULTI_BROWSER  3

#define FL_SORT_ASCENDING  0
#define FL_SORT_DESCENDING 1

/* has_scrollbar() values -- bit flags, same numeric values as upstream. */
#define FL_BROWSER_HORIZONTAL        1
#define FL_BROWSER_VERTICAL          2
#define FL_BROWSER_BOTH              3
#define FL_BROWSER_ALWAYS_ON         4
#define FL_BROWSER_HORIZONTAL_ALWAYS 5
#define FL_BROWSER_VERTICAL_ALWAYS   6
#define FL_BROWSER_BOTH_ALWAYS       7

typedef struct Fl_Browser_ Fl_Browser_;

/* The item-level vtable every concrete browser subclass supplies (see
 * class-conversion note above). `self` is always the concrete widget,
 * upcast to Fl_Browser_* -- exactly like Fl_WidgetOps callbacks receive
 * the concrete widget upcast to Fl_Widget*; implementations downcast
 * back to their own type the same way ops->draw() implementations do. */
typedef struct Fl_Browser_ItemOps {
    void *(*item_first)(Fl_Browser_ *self);
    void *(*item_next)(Fl_Browser_ *self, void *item);
    void *(*item_prev)(Fl_Browser_ *self, void *item);
    void *(*item_last)(Fl_Browser_ *self);                         /* optional, NULL -> 0 */
    int (*item_height)(Fl_Browser_ *self, void *item);
    int (*item_width)(Fl_Browser_ *self, void *item);
    int (*item_quick_height)(Fl_Browser_ *self, void *item);       /* optional -> item_height() */
    void (*item_draw)(Fl_Browser_ *self, void *item, int X, int Y, int W, int H);
    const char *(*item_text)(Fl_Browser_ *self, void *item);       /* optional -> NULL */
    void (*item_swap)(Fl_Browser_ *self, void *a, void *b);        /* optional -> no-op */
    void *(*item_at)(Fl_Browser_ *self, int index);                /* optional -> NULL */
    void (*item_select)(Fl_Browser_ *self, void *item, int val);   /* optional -> no-op */
    int (*item_selected)(Fl_Browser_ *self, void *item);           /* optional -> item==selection() */
    int (*full_width)(Fl_Browser_ *self);                          /* optional -> tracked max_width */
    int (*full_height)(Fl_Browser_ *self);                         /* optional -> sum of quick heights */
    int (*incr_height)(Fl_Browser_ *self);                         /* optional -> quick_height(item_first()) */
} Fl_Browser_ItemOps;

struct Fl_Browser_ {
    Fl_Group group;
    const Fl_Browser_ItemOps *item_ops;

    Fl_Scrollbar scrollbar;  /* vertical */
    Fl_Scrollbar hscrollbar; /* horizontal */

    int position_, real_position_;   /* vertical scroll, pixels */
    int hposition_, real_hposition_; /* horizontal scroll, pixels */
    int offset_;      /* how far down top_ the real_position is */
    int max_width;    /* widest item seen so far (cache) */
    void *max_width_item;
    uchar has_scrollbar_;
    Fl_Font textfont_;
    Fl_Fontsize textsize_;
    Fl_Color textcolor_;
    void *top_;       /* item scrolling position is relative to */
    void *selection_; /* selected item (or focus item for MULTI) */
    void *redraw1, *redraw2; /* minimal-update pointers */
    int scrollbar_size_;
};

/* Called by a concrete subclass's own _init() after Fl_Group_init();
 * `ops` must stay valid for the widget's lifetime (a static const table,
 * per the Fl_WidgetOps convention). */
void Fl_Browser__init(Fl_Browser_ *self, const Fl_WidgetOps *widget_ops, const Fl_Browser_ItemOps *item_ops,
                       int x, int y, int w, int h, const char *label);
/* Common destroy body every concrete subclass's own destroy() must call
 * (frees nothing of the subclass's own item storage -- that's on the
 * subclass -- just tears down the two embedded scrollbars and the
 * Fl_Group base, exactly like Fl_Scroll_destroy()). */
void Fl_Browser__destroy(Fl_Widget *self);

/* Shared draw()/handle()/resize() every concrete subclass's Fl_WidgetOps
 * points at directly (there is no per-subclass widget-level behavior to
 * override -- only the item-ops vary). */
void Fl_Browser__draw(Fl_Widget *self);
int Fl_Browser__handle(Fl_Widget *self, int event);
void Fl_Browser__resize(Fl_Widget *self, int x, int y, int w, int h);
Fl_Group *Fl_Browser__as_group(Fl_Widget *self);

void Fl_Browser__bbox(const Fl_Browser_ *self, int *X, int *Y, int *W, int *H);
int Fl_Browser__leftedge(const Fl_Browser_ *self);

int Fl_Browser__select(Fl_Browser_ *self, void *item, int val, int docallbacks);
int Fl_Browser__select_only(Fl_Browser_ *self, void *item, int docallbacks);
int Fl_Browser__deselect(Fl_Browser_ *self, int docallbacks);

static inline void *Fl_Browser__top(const Fl_Browser_ *self) { return self->top_; }
static inline void *Fl_Browser__selection(const Fl_Browser_ *self) { return self->selection_; }

void Fl_Browser__new_list(Fl_Browser_ *self);
void Fl_Browser__deleting(Fl_Browser_ *self, void *item);
void Fl_Browser__replacing(Fl_Browser_ *self, void *a, void *b);
void Fl_Browser__swapping(Fl_Browser_ *self, void *a, void *b);
void Fl_Browser__inserting(Fl_Browser_ *self, void *a, void *b);
int Fl_Browser__displayed(const Fl_Browser_ *self, void *item);
void Fl_Browser__redraw_line(Fl_Browser_ *self, void *item);
static inline void Fl_Browser__redraw_lines(Fl_Browser_ *self) { Fl_Widget_set_damage(&self->group.widget, FL_DAMAGE_SCROLL); }
void *Fl_Browser__find_item(Fl_Browser_ *self, int ypos);

static inline int Fl_Browser__position(const Fl_Browser_ *self) { return self->position_; }
void Fl_Browser__set_position(Fl_Browser_ *self, int pos);
static inline int Fl_Browser__hposition(const Fl_Browser_ *self) { return self->hposition_; }
void Fl_Browser__set_hposition(Fl_Browser_ *self, int pos);
void Fl_Browser__display(Fl_Browser_ *self, void *item);

static inline uchar Fl_Browser__has_scrollbar(const Fl_Browser_ *self) { return self->has_scrollbar_; }
static inline void Fl_Browser__set_has_scrollbar(Fl_Browser_ *self, uchar mode) { self->has_scrollbar_ = mode; }

static inline Fl_Font Fl_Browser__textfont(const Fl_Browser_ *self) { return self->textfont_; }
static inline void Fl_Browser__set_textfont(Fl_Browser_ *self, Fl_Font f) { self->textfont_ = f; }
static inline Fl_Fontsize Fl_Browser__textsize(const Fl_Browser_ *self) { return self->textsize_; }
static inline void Fl_Browser__set_textsize(Fl_Browser_ *self, Fl_Fontsize s) { self->textsize_ = s; }
static inline Fl_Color Fl_Browser__textcolor(const Fl_Browser_ *self) { return self->textcolor_; }
static inline void Fl_Browser__set_textcolor(Fl_Browser_ *self, Fl_Color c) { self->textcolor_ = c; }

static inline int Fl_Browser__scrollbar_size(const Fl_Browser_ *self) { return self->scrollbar_size_; }
static inline void Fl_Browser__set_scrollbar_size(Fl_Browser_ *self, int n) { self->scrollbar_size_ = n; }

static inline void Fl_Browser__scrollbar_right(Fl_Browser_ *self) { Fl_Widget_set_align(&self->scrollbar.slider.valuator.widget, FL_ALIGN_RIGHT); }
static inline void Fl_Browser__scrollbar_left(Fl_Browser_ *self) { Fl_Widget_set_align(&self->scrollbar.slider.valuator.widget, FL_ALIGN_LEFT); }

void Fl_Browser__sort(Fl_Browser_ *self, int flags);

/* Generic default item-ops bodies (used when an optional entry is NULL);
 * exposed in case a subclass wants to call "the base version" from its
 * own override, matching upstream's ability to call e.g.
 * Fl_Browser_::full_height() from an overriding subclass. */
int Fl_Browser__default_item_quick_height(Fl_Browser_ *self, void *item);
int Fl_Browser__default_full_width(Fl_Browser_ *self);
int Fl_Browser__default_full_height(Fl_Browser_ *self);
int Fl_Browser__default_incr_height(Fl_Browser_ *self);
int Fl_Browser__default_item_selected(Fl_Browser_ *self, void *item);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_BROWSER__H */
