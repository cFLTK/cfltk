/*
 * cfltk - Fl_Menu_Item.h
 *
 * C translation of FLTK 1.3 FL/Fl_Menu_Item.H.
 *
 * Original class : struct Fl_Menu_Item (a plain data struct, not a
 *                   widget -- upstream itself notes "you should use the
 *                   method functions... to avoid compatibility problems",
 *                   but the fields are public and apps overwhelmingly
 *                   initialize static arrays with them directly, e.g.
 *                   `Fl_Menu_Item items[] = {{"&New", 0, cb}, ...};`).
 * New C structure : struct Fl_Menu_Item with the SAME field order as
 *                   upstream (text, shortcut_, callback_, user_data_,
 *                   flags, labeltype_, labelfont_, labelsize_,
 *                   labelcolor_), specifically so positional aggregate
 *                   initializers ported from C++ compile unchanged in C.
 * Inheritance     : none -- Fl_Menu_Item never was a widget.
 * Vtbl            : none -- there is nothing virtual about a menu item.
 * Ownership       : arrays of Fl_Menu_Item are almost always static/
 *                   caller-owned (see Fl_Menu_.h for the one case where
 *                   Fl_Menu_ owns a private copy: Fl_Menu_add()).
 * Known differences:
 *   - No hierarchical "File/Open" path parsing in add()/insert() (see
 *     Fl_Menu_.h) -- construct submenu arrays explicitly with
 *     FL_SUBMENU/FL_SUBMENU_POINTER instead, which is also how upstream
 *     represents them internally after parsing.
 *   - No Fl_Image label support (label images are part of the
 *     not-yet-ported Fl_Image subsystem).
 */
#ifndef CFLTK_FL_MENU_ITEM_H
#define CFLTK_FL_MENU_ITEM_H

#include "cfltk/Fl_Widget.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    FL_MENU_INACTIVE    = 1,
    FL_MENU_TOGGLE       = 2,
    FL_MENU_VALUE        = 4,
    FL_MENU_RADIO        = 8,
    FL_MENU_INVISIBLE    = 0x10,
    FL_SUBMENU_POINTER   = 0x20,
    FL_SUBMENU           = 0x40,
    FL_MENU_DIVIDER      = 0x80,
    FL_MENU_HORIZONTAL   = 0x100
};

typedef struct Fl_Menu_ Fl_Menu_; /* see Fl_Menu_.h; forward-declared here
                                      only so popup()/pulldown() below can
                                      take an optional style source. */

typedef struct Fl_Menu_Item {
    const char *text;
    int shortcut_;
    Fl_Callback *callback_;
    void *user_data_;
    int flags;
    uchar labeltype_;
    Fl_Font labelfont_;
    Fl_Fontsize labelsize_;
    Fl_Color labelcolor_;
} Fl_Menu_Item;

typedef Fl_Menu_Item Fl_Menu; /* back-compatibility alias, matches upstream */

/* Advances by n items, skipping submenu contents and invisible items
 * (n=0 is "first", matching upstream's next(0)==first()). */
const Fl_Menu_Item *Fl_Menu_Item_next(const Fl_Menu_Item *self, int n);
static inline const Fl_Menu_Item *Fl_Menu_Item_first(const Fl_Menu_Item *self) { return Fl_Menu_Item_next(self, 0); }

/* Count of items from self through (and including) the terminating
 * NULL-text item, counting nested FL_SUBMENU contents but not
 * FL_SUBMENU_POINTER targets. Multiply by sizeof(Fl_Menu_Item) to copy
 * a whole array (see Fl_Menu_copy() in Fl_Menu_.h). */
int Fl_Menu_Item_size(const Fl_Menu_Item *self);

static inline const char *Fl_Menu_Item_label(const Fl_Menu_Item *self) { return self->text; }
static inline void Fl_Menu_Item_set_label(Fl_Menu_Item *self, const char *a) { self->text = a; }

static inline uchar Fl_Menu_Item_labeltype(const Fl_Menu_Item *self) { return self->labeltype_; }
static inline void Fl_Menu_Item_set_labeltype(Fl_Menu_Item *self, uchar a) { self->labeltype_ = a; }
static inline Fl_Color Fl_Menu_Item_labelcolor(const Fl_Menu_Item *self) { return self->labelcolor_; }
static inline void Fl_Menu_Item_set_labelcolor(Fl_Menu_Item *self, Fl_Color a) { self->labelcolor_ = a; }
static inline Fl_Font Fl_Menu_Item_labelfont(const Fl_Menu_Item *self) { return self->labelfont_; }
static inline void Fl_Menu_Item_set_labelfont(Fl_Menu_Item *self, Fl_Font a) { self->labelfont_ = a; }
static inline Fl_Fontsize Fl_Menu_Item_labelsize(const Fl_Menu_Item *self) { return self->labelsize_; }
static inline void Fl_Menu_Item_set_labelsize(Fl_Menu_Item *self, Fl_Fontsize a) { self->labelsize_ = a; }

static inline Fl_Callback *Fl_Menu_Item_callback(const Fl_Menu_Item *self) { return self->callback_; }
static inline void Fl_Menu_Item_set_callback(Fl_Menu_Item *self, Fl_Callback *c, void *p) { self->callback_ = c; self->user_data_ = p; }
static inline void *Fl_Menu_Item_user_data(const Fl_Menu_Item *self) { return self->user_data_; }
static inline void Fl_Menu_Item_set_user_data(Fl_Menu_Item *self, void *v) { self->user_data_ = v; }

static inline Fl_Shortcut Fl_Menu_Item_shortcut(const Fl_Menu_Item *self) { return (Fl_Shortcut)self->shortcut_; }
static inline void Fl_Menu_Item_set_shortcut(Fl_Menu_Item *self, Fl_Shortcut s) { self->shortcut_ = (int)s; }

static inline int Fl_Menu_Item_submenu(const Fl_Menu_Item *self) { return self->flags & (FL_SUBMENU | FL_SUBMENU_POINTER); }
static inline int Fl_Menu_Item_checkbox(const Fl_Menu_Item *self) { return self->flags & FL_MENU_TOGGLE; }
static inline int Fl_Menu_Item_radio(const Fl_Menu_Item *self) { return self->flags & FL_MENU_RADIO; }
static inline int Fl_Menu_Item_value(const Fl_Menu_Item *self) { return self->flags & FL_MENU_VALUE; }
static inline void Fl_Menu_Item_set(Fl_Menu_Item *self) { self->flags |= FL_MENU_VALUE; }
static inline void Fl_Menu_Item_clear(Fl_Menu_Item *self) { self->flags &= ~(int)FL_MENU_VALUE; }
/* Sets this item on and clears every FL_MENU_RADIO sibling adjacent to
 * it (stopping at a divider or a non-radio item), matching upstream's
 * Fl_Menu_Item::setonly(). Fl_Menu_setonly() (Fl_Menu_.h) is the
 * widget-aware version most callers want. */
void Fl_Menu_Item_setonly(Fl_Menu_Item *self);

static inline int Fl_Menu_Item_visible(const Fl_Menu_Item *self) { return !(self->flags & FL_MENU_INVISIBLE); }
static inline void Fl_Menu_Item_show(Fl_Menu_Item *self) { self->flags &= ~(int)FL_MENU_INVISIBLE; }
static inline void Fl_Menu_Item_hide(Fl_Menu_Item *self) { self->flags |= FL_MENU_INVISIBLE; }
static inline int Fl_Menu_Item_active(const Fl_Menu_Item *self) { return !(self->flags & FL_MENU_INACTIVE); }
static inline void Fl_Menu_Item_activate(Fl_Menu_Item *self) { self->flags &= ~(int)FL_MENU_INACTIVE; }
static inline void Fl_Menu_Item_deactivate(Fl_Menu_Item *self) { self->flags |= FL_MENU_INACTIVE; }
static inline int Fl_Menu_Item_activevisible(const Fl_Menu_Item *self) { return !(self->flags & (FL_MENU_INACTIVE | FL_MENU_INVISIBLE)); }

/* Measures this item as it would be drawn for `owner` (or with built-in
 * defaults if owner is NULL): returns the width in pixels and, if
 * h_out is non-NULL, writes the row height. Shared by the popup engine
 * and Fl_Menu_Bar so layout and drawing can never disagree. */
int Fl_Menu_Item_measure(const Fl_Menu_Item *self, int *h_out, const Fl_Menu_ *owner);
/* Draws this item's checkbox/radio indicator, label, submenu arrow and
 * divider (if any) into the box x,y,w,h. `selected` highlights it with
 * owner's (or the default) selection_color()/down_box(). */
void Fl_Menu_Item_draw(const Fl_Menu_Item *self, int x, int y, int w, int h, const Fl_Menu_ *owner, int selected);

static inline void Fl_Menu_Item_do_callback(const Fl_Menu_Item *self, Fl_Widget *o) { self->callback_(o, self->user_data_); }
static inline void Fl_Menu_Item_do_callback_with(const Fl_Menu_Item *self, Fl_Widget *o, void *arg) { self->callback_(o, arg); }

/* Unicode code point of the item's '&x' label mnemonic, or 0 -- see the
 * ASCII-only note in Fl_Widget.h; same limitation here. */
unsigned int Fl_Menu_Item_label_shortcut(const Fl_Menu_Item *self);
/* True if the current event (must be FL_KEYBOARD/FL_SHORTCUT) matches
 * this item's shortcut_ or its label's '&x' mnemonic. */
int Fl_Menu_Item_test_shortcut_one(const Fl_Menu_Item *self);
/* Scans the whole array starting at self (descending into FL_SUBMENU
 * contents, matching upstream) for an item whose shortcut_ or label
 * mnemonic matches the current event. Returns NULL if none. */
const Fl_Menu_Item *Fl_Menu_Item_test_shortcut(const Fl_Menu_Item *self);
/* Like test_shortcut(), but only matches label mnemonics (not
 * shortcut_), and only while FL_ALT is held if require_alt is set --
 * used by Fl_Menu_Bar to find which top-level menu Alt+letter opens. */
const Fl_Menu_Item *Fl_Menu_Item_find_shortcut(const Fl_Menu_Item *self, int *index_out, int require_alt);

/* -------------------------------------------------------------------
 * Popup engine (translated in spirit, not verbatim, from src/Fl_Menu.cxx
 * -- see docs/DESIGN.md for how cfltk's single-popup-window design
 * differs from upstream's per-level nested windows).
 * ---------------------------------------------------------------- */

/* Pops up a free-floating menu at (x,y) (e.g. right-click context menu).
 * `owner` supplies textfont/textsize/textcolor/color/selection_color/
 * down_box for rendering and may be NULL (falls back to built-in
 * defaults). `picked` pre-highlights that item if non-NULL. Blocks
 * until the user picks an item or dismisses the menu (click outside,
 * Escape); returns the picked leaf item, or NULL if dismissed. */
const Fl_Menu_Item *Fl_Menu_Item_popup(const Fl_Menu_Item *self, int x, int y,
                                        const Fl_Menu_ *owner, const Fl_Menu_Item *picked);

/* Pops up a menu anchored to a rectangle (e.g. a button or menu bar
 * item): the first level drops down below (x,y,w,h) unless menubar is
 * set, in which case the first level is laid out horizontally across
 * (x,y,w,h) itself (used by Fl_Menu_Bar) and picking a top-level item
 * opens its vertical submenu below it. */
const Fl_Menu_Item *Fl_Menu_Item_pulldown(const Fl_Menu_Item *self, int x, int y, int w, int h,
                                           const Fl_Menu_ *owner, const Fl_Menu_Item *picked, int menubar);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_MENU_ITEM_H */
