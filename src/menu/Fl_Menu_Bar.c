/*
 * cfltk - Fl_Menu_Bar.c
 * See include/cfltk/Fl_Menu_Bar.h for the class-conversion notes.
 * Translated from src/Fl_Menu_Bar.cxx.
 */
#include <stdlib.h>

#include "cfltk/Fl_Menu_Bar.h"
#include "cfltk/Fl.h"
#include "cfltk/fl_draw.h"

const Fl_WidgetOps fl_menu_bar_ops = {
    Fl_Menu_Bar_draw,
    Fl_Menu_Bar_handle,
    NULL, NULL, NULL,
    Fl_Menu__destroy,
    NULL, NULL
};

void Fl_Menu_Bar_init(Fl_Menu_ *self, int x, int y, int w, int h, const char *label) {
    Fl_Menu__init(self, &fl_menu_bar_ops, x, y, w, h, label);
}

Fl_Menu_ *Fl_Menu_Bar_new(int x, int y, int w, int h, const char *label) {
    Fl_Menu_ *self = (Fl_Menu_ *)malloc(sizeof(Fl_Menu_));
    Fl_Menu_Bar_init(self, x, y, w, h, label);
    return self;
}

void Fl_Menu_Bar_draw(Fl_Widget *self_w) {
    Fl_Menu_ *self = (Fl_Menu_ *)self_w;
    const Fl_Menu_Item *m;
    int X = self_w->x + 6;

    fl_draw_box(self_w->box, self_w->x, self_w->y, self_w->w, self_w->h, self_w->color);
    if (!self->menu_ || !self->menu_->text) return;

    for (m = Fl_Menu_Item_first(self->menu_); m->text; m = Fl_Menu_Item_next(m, 1)) {
        int W = Fl_Menu_Item_measure(m, NULL, self) + 16;
        Fl_Menu_Item_draw(m, X, self_w->y, W, self_w->h, self, 0);
        X += W;
        if (m->flags & FL_MENU_DIVIDER) {
            int y1 = self_w->y + fl_box_dy(self_w->box);
            int y2 = y1 + self_w->h - fl_box_dh(self_w->box) - 1;
            fl_color(FL_DARK3);
            fl_yxline(X - 6, y1, y2);
            fl_color(FL_LIGHT3);
            fl_yxline(X - 5, y1, y2);
        }
    }
}

int Fl_Menu_Bar_handle(Fl_Widget *self_w, int event) {
    Fl_Menu_ *self = (Fl_Menu_ *)self_w;
    const Fl_Menu_Item *v;

    if (!self->menu_ || !self->menu_->text) return 0;

    switch (event) {
        case FL_ENTER:
        case FL_LEAVE:
            return 1;

        case FL_PUSH:
            v = Fl_Menu_Item_pulldown(self->menu_, self_w->x, self_w->y, self_w->w, self_w->h, self, NULL, 1);
            Fl_Menu_picked(self, v);
            return 1;

        case FL_SHORTCUT:
            if (Fl_Widget_visible_r(self_w)) {
                v = Fl_Menu_Item_find_shortcut(self->menu_, NULL, 1);
                if (v && Fl_Menu_Item_submenu(v)) {
                    v = Fl_Menu_Item_pulldown(self->menu_, self_w->x, self_w->y, self_w->w, self_w->h, self, NULL, 1);
                    Fl_Menu_picked(self, v);
                    return 1;
                }
            }
            return Fl_Menu_test_shortcut(self) != NULL;

        default:
            return 0;
    }
}
