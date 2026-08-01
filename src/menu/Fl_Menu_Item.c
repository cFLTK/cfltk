/*
 * cfltk - Fl_Menu_Item.c
 * See include/cfltk/Fl_Menu_Item.h for the class-conversion notes.
 * Translated from src/Fl_Menu.cxx (the Fl_Menu_Item:: methods) and
 * src/fl_shortcut.cxx.
 */
#include <string.h>

#include "cfltk/Fl_Menu_Item.h"
#include "cfltk/Fl_Menu_.h"
#include "cfltk/Fl.h"
#include "cfltk/fl_draw.h"

static Fl_Font item_font_of(const Fl_Menu_Item *self, const Fl_Menu_ *owner) {
    if (self->labelfont_ || self->labelsize_) return self->labelfont_;
    return owner ? Fl_Menu_textfont(owner) : FL_HELVETICA;
}
static Fl_Fontsize item_size_of(const Fl_Menu_Item *self, const Fl_Menu_ *owner) {
    return self->labelsize_ ? self->labelsize_ : (owner ? Fl_Menu_textsize(owner) : FL_NORMAL_SIZE);
}

int Fl_Menu_Item_size(const Fl_Menu_Item *self) {
    const Fl_Menu_Item *m = self;
    int nest = 0;
    for (;;) {
        if (!m->text) {
            if (!nest) return (int)(m - self + 1);
            nest--;
        } else if (m->flags & FL_SUBMENU) {
            nest++;
        }
        m++;
    }
}

/* Advances to the next visible-or-not item at the current nesting
 * level, skipping over an entire submenu's contents in one step. */
static const Fl_Menu_Item *next_visible_or_not(const Fl_Menu_Item *m) {
    int nest = 0;
    do {
        if (!m->text) {
            if (!nest) return m;
            nest--;
        } else if (m->flags & FL_SUBMENU) {
            nest++;
        }
        m++;
    } while (nest);
    return m;
}

const Fl_Menu_Item *Fl_Menu_Item_next(const Fl_Menu_Item *self, int n) {
    const Fl_Menu_Item *m = self;
    if (n < 0) return NULL;
    if (!Fl_Menu_Item_visible(m)) n++;
    while (n) {
        m = next_visible_or_not(m);
        if (Fl_Menu_Item_visible(m) || !m->text) n--;
    }
    return m;
}

void Fl_Menu_Item_setonly(Fl_Menu_Item *self) {
    Fl_Menu_Item *j;
    self->flags |= FL_MENU_RADIO | FL_MENU_VALUE;
    for (j = self;;) {
        if (j->flags & FL_MENU_DIVIDER) break;
        j++;
        if (!j->text || !Fl_Menu_Item_radio(j)) break;
        Fl_Menu_Item_clear(j);
    }
    for (j = self - 1;; j--) {
        if (!j->text || (j->flags & FL_MENU_DIVIDER) || !Fl_Menu_Item_radio(j)) break;
        Fl_Menu_Item_clear(j);
    }
}

unsigned int Fl_Menu_Item_label_shortcut(const Fl_Menu_Item *self) {
    return Fl_Widget_label_shortcut(self->text);
}

int Fl_Menu_Item_test_shortcut_one(const Fl_Menu_Item *self) {
    if (!self->text) return 0;
    if (self->shortcut_ && Fl_test_shortcut((Fl_Shortcut)self->shortcut_)) return 1;
    return Fl_Widget_test_shortcut_str(self->text, 0);
}

const Fl_Menu_Item *Fl_Menu_Item_test_shortcut(const Fl_Menu_Item *self) {
    const Fl_Menu_Item *m;
    if (!self) return NULL;
    for (m = self; m->text; m = Fl_Menu_Item_next(m, 1)) {
        if (Fl_Menu_Item_activevisible(m) && Fl_Menu_Item_test_shortcut_one(m)) return m;
    }
    return NULL;
}

int Fl_Menu_Item_measure(const Fl_Menu_Item *self, int *h_out, const Fl_Menu_ *owner) {
    Fl_Label l;
    int w;

    l.value = self->text;
    l.image = NULL;
    l.deimage = NULL;
    l.type = self->labeltype_;
    l.font = item_font_of(self, owner);
    l.size = item_size_of(self, owner);
    l.color = FL_FOREGROUND_COLOR;
    l.align = FL_ALIGN_LEFT;

    fl_draw_shortcut = 1;
    fl_label_measure(&l, &w, h_out ? h_out : &(int){0});
    fl_draw_shortcut = 0;

    if (self->flags & (FL_MENU_TOGGLE | FL_MENU_RADIO)) w += item_size_of(self, owner) + 6;
    if (Fl_Menu_Item_submenu(self)) w += item_size_of(self, owner);
    return w;
}

void Fl_Menu_Item_draw(const Fl_Menu_Item *self, int x, int y, int w, int h, const Fl_Menu_ *owner, int selected) {
    Fl_Color textcol = self->labelcolor_ ? self->labelcolor_ : (owner ? Fl_Menu_textcolor(owner) : FL_FOREGROUND_COLOR);
    int active = Fl_Menu_Item_active(self);
    int lx = x, lw = w;

    if (!active) textcol = fl_inactive(textcol);

    if (selected && active) {
        Fl_Color sel = owner ? Fl_Widget_selection_color(&owner->widget) : FL_SELECTION_COLOR;
        uchar box = (owner && Fl_Menu_down_box(owner)) ? Fl_Menu_down_box(owner) : FL_FLAT_BOX;
        fl_draw_box(box, x + 1, y, w - 2, h, sel);
        textcol = fl_contrast(textcol, sel);
    }

    if (self->flags & (FL_MENU_TOGGLE | FL_MENU_RADIO)) {
        Fl_Fontsize sz = item_size_of(self, owner);
        int box_sz = sz > 6 ? sz - 4 : 8;
        int bx = lx + 3, by = y + (h - box_sz) / 2;
        if (self->flags & FL_MENU_RADIO) {
            fl_draw_box(FL_ROUND_DOWN_BOX, bx, by, box_sz, box_sz, FL_BACKGROUND2_COLOR);
            if (Fl_Menu_Item_value(self)) {
                fl_color(textcol);
                fl_pie(bx + 2, by + 2, box_sz - 4, box_sz - 4, 0.0, 360.0);
            }
        } else {
            fl_draw_box(FL_DOWN_BOX, bx, by, box_sz, box_sz, FL_BACKGROUND2_COLOR);
            if (Fl_Menu_Item_value(self)) {
                fl_color(textcol);
                fl_line(bx + 2, by + box_sz / 2, bx + box_sz / 2 - 1, by + box_sz - 3);
                fl_line(bx + box_sz / 2 - 1, by + box_sz - 3, bx + box_sz - 2, by + 2);
            }
        }
        lx += box_sz + 6;
        lw -= box_sz + 6;
    }

    if (Fl_Menu_Item_submenu(self)) lw -= item_size_of(self, owner);

    {
        Fl_Label l;
        l.value = self->text;
        l.image = NULL;
        l.deimage = NULL;
        l.type = self->labeltype_;
        l.font = item_font_of(self, owner);
        l.size = item_size_of(self, owner);
        l.color = textcol;
        l.align = FL_ALIGN_LEFT;
        fl_draw_shortcut = 1;
        fl_label_draw(&l, lx + 4, y, lw > 4 ? lw - 4 : 0, h, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
        fl_draw_shortcut = 0;
    }

    if (Fl_Menu_Item_submenu(self)) {
        int ax = x + w - 8, ay = y + h / 2;
        fl_color(textcol);
        fl_polygon3(ax - 4, ay - 4, ax - 4, ay + 4, ax + 2, ay);
    }

    if (self->flags & FL_MENU_DIVIDER) {
        fl_color(FL_DARK3);
        fl_xyline(x, y + h, x + w);
    }
}

const Fl_Menu_Item *Fl_Menu_Item_find_shortcut(const Fl_Menu_Item *self, int *index_out, int require_alt) {
    const Fl_Menu_Item *m;
    int i;
    if (!self) return NULL;
    for (m = self, i = 0; m->text; m = Fl_Menu_Item_next(m, 1), i++) {
        if (!Fl_Menu_Item_activevisible(m)) continue;
        if (Fl_Widget_test_shortcut_str(m->text, require_alt)) {
            if (index_out) *index_out = i;
            return m;
        }
    }
    return NULL;
}
