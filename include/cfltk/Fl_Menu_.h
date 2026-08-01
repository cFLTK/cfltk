/*
 * cfltk - Fl_Menu_.h
 *
 * C translation of FLTK 1.3 FL/Fl_Menu_.H.
 *
 * Original class : Fl_Menu_ : public Fl_Widget (base of Fl_Menu_Button,
 *                   Fl_Choice, Fl_Menu_Bar; holds the Fl_Menu_Item
 *                   array pointer, the "last picked" item, and menu
 *                   rendering style -- no draw()/handle() of its own).
 * New C structure : struct Fl_Menu_ { Fl_Widget widget; Fl_Menu_Item
 *                   *menu_; const Fl_Menu_Item *value_; uchar alloc,
 *                   down_box_; Fl_Font textfont_; Fl_Fontsize textsize_;
 *                   Fl_Color textcolor_; }; reused as-is by
 *                   Fl_Menu_Button/Fl_Choice/Fl_Menu_Bar, which only
 *                   differ in draw()/handle() (own vtables) and
 *                   constructor defaults -- same "no fields added"
 *                   pattern as the button family.
 * Ownership       : `menu_` is caller-owned unless Fl_Menu_copy() was
 *                   used, in which case Fl_Menu_ owns a private
 *                   heap copy (freed in destroy()). Fl_Menu_add()
 *                   grows/owns a private copy the same way.
 * Known differences:
 *   - add()/insert() take a flat label (no "File/Open" hierarchical
 *     path parsing); build submenu arrays explicitly with
 *     FL_SUBMENU/FL_SUBMENU_POINTER via Fl_Menu_set_menu()/copy()
 *     instead, which is what upstream's path parser produces
 *     internally anyway.
 *   - No item_pathname()/find_index(pathname) (both depend on the path
 *     representation above).
 *   - No Fl::menu() (Mac OS system menu bar integration) -- not
 *     meaningful on the X11 backend.
 */
#ifndef CFLTK_FL_MENU__H
#define CFLTK_FL_MENU__H

#include "cfltk/Fl_Widget.h"
#include "cfltk/Fl_Menu_Item.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Fl_Menu_ {
    Fl_Widget widget;

    Fl_Menu_Item *menu_;
    const Fl_Menu_Item *value_;

    uchar alloc; /* 0 = caller-owned menu_, 1 = owned private copy (Fl_Menu_copy()/add()) */
    uchar down_box_;
    Fl_Font textfont_;
    Fl_Fontsize textsize_;
    Fl_Color textcolor_;
};

/* Shared init every concrete menu widget's own _init() calls first,
 * passing its own vtable (there is no single fl_menu__ops -- unlike
 * Fl_Button, each concrete class here has a different enough draw()
 * that sharing one table isn't worth it). */
void Fl_Menu__init(Fl_Menu_ *self, const Fl_WidgetOps *ops, int x, int y, int w, int h, const char *label);
void Fl_Menu__destroy(Fl_Widget *self);

static inline const Fl_Menu_Item *Fl_Menu_menu(const Fl_Menu_ *self) { return self->menu_; }
/* Caller retains ownership of m; Fl_Menu_ just stores the pointer.
 * Discards any previously-owned private copy first. */
void Fl_Menu_set_menu(Fl_Menu_ *self, const Fl_Menu_Item *m);
/* Stores a private heap copy of m (Fl_Menu_Item_size(m) items). If
 * user_data is non-NULL, every item with a callback gets its
 * user_data_ overwritten with it (convenience for binding a whole
 * static menu to one context pointer). */
void Fl_Menu_copy(Fl_Menu_ *self, const Fl_Menu_Item *m, void *user_data);

/* Appends one flat item (no submenu/path parsing -- see Known
 * differences) to a private, growable copy of the array, creating one
 * if menu_ was NULL or caller-owned. Returns the new item's index. */
int Fl_Menu_add(Fl_Menu_ *self, const char *label, Fl_Shortcut shortcut, Fl_Callback *cb, void *user_data, int flags);
void Fl_Menu_clear(Fl_Menu_ *self);
void Fl_Menu_remove(Fl_Menu_ *self, int index);

static inline int Fl_Menu_size(const Fl_Menu_ *self) { return self->menu_ ? Fl_Menu_Item_size(self->menu_) : 0; }

int Fl_Menu_find_index_item(const Fl_Menu_ *self, const Fl_Menu_Item *item);
int Fl_Menu_find_index_cb(const Fl_Menu_ *self, Fl_Callback *cb);

static inline const Fl_Menu_Item *Fl_Menu_mvalue(const Fl_Menu_ *self) { return self->value_; }
static inline int Fl_Menu_value(const Fl_Menu_ *self) { return self->value_ ? (int)(self->value_ - self->menu_) : -1; }
/* Returns non-zero if the value actually changed; does not run any
 * callback or toggle/radio bookkeeping (see Fl_Menu_picked() for that). */
int Fl_Menu_set_value(Fl_Menu_ *self, const Fl_Menu_Item *m);
static inline int Fl_Menu_set_value_index(Fl_Menu_ *self, int i) { return Fl_Menu_set_value(self, self->menu_ + i); }

/* What every concrete menu widget's popup-dismissal path calls with
 * the item the user picked: applies toggle/radio state changes, sets
 * mvalue(), and fires the item's callback (or this widget's, if the
 * item has none) according to when(). Returns v unchanged, for
 * convenient chaining. */
const Fl_Menu_Item *Fl_Menu_picked(Fl_Menu_ *self, const Fl_Menu_Item *v);
/* Sets item on and clears adjacent FL_MENU_RADIO siblings, verifying
 * item actually belongs to this widget's menu() first (unlike
 * Fl_Menu_Item_setonly(), which trusts the caller). */
void Fl_Menu_setonly(Fl_Menu_ *self, Fl_Menu_Item *item);

static inline const Fl_Menu_Item *Fl_Menu_test_shortcut(Fl_Menu_ *self) {
    return Fl_Menu_picked(self, Fl_Menu_Item_test_shortcut(self->menu_));
}

static inline Fl_Font Fl_Menu_textfont(const Fl_Menu_ *self) { return self->textfont_; }
static inline void Fl_Menu_set_textfont(Fl_Menu_ *self, Fl_Font f) { self->textfont_ = f; }
static inline Fl_Fontsize Fl_Menu_textsize(const Fl_Menu_ *self) { return self->textsize_; }
static inline void Fl_Menu_set_textsize(Fl_Menu_ *self, Fl_Fontsize s) { self->textsize_ = s; }
static inline Fl_Color Fl_Menu_textcolor(const Fl_Menu_ *self) { return self->textcolor_; }
static inline void Fl_Menu_set_textcolor(Fl_Menu_ *self, Fl_Color c) { self->textcolor_ = c; }

static inline uchar Fl_Menu_down_box(const Fl_Menu_ *self) { return self->down_box_; }
static inline void Fl_Menu_set_down_box(Fl_Menu_ *self, uchar b) { self->down_box_ = b; }
static inline Fl_Color Fl_Menu_down_color(const Fl_Menu_ *self) { return Fl_Widget_selection_color(&self->widget); }
static inline void Fl_Menu_set_down_color(Fl_Menu_ *self, Fl_Color c) { Fl_Widget_set_selection_color(&self->widget, c); }

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_MENU__H */
