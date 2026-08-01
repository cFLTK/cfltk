/*
 * cfltk - Fl_Menu_Button.c
 * See include/cfltk/Fl_Menu_Button.h for the class-conversion notes.
 * Translated from src/Fl_Menu_Button.cxx.
 */
#include <stdlib.h>

#include "cfltk/Fl_Menu_Button.h"
#include "cfltk/Fl.h"
#include "cfltk/fl_draw.h"

const Fl_WidgetOps fl_menu_button_ops = {
    Fl_Menu_Button_draw,
    Fl_Menu_Button_handle,
    NULL, NULL, NULL,
    Fl_Menu__destroy,
    NULL, NULL
};

static Fl_Menu_ *g_pressed_menu_button = NULL;

void Fl_Menu_Button_init(Fl_Menu_ *self, int x, int y, int w, int h, const char *label) {
    Fl_Menu__init(self, &fl_menu_button_ops, x, y, w, h, label);
    Fl_Menu_set_down_box(self, FL_NO_BOX);
}

Fl_Menu_ *Fl_Menu_Button_new(int x, int y, int w, int h, const char *label) {
    Fl_Menu_ *self = (Fl_Menu_ *)malloc(sizeof(Fl_Menu_));
    Fl_Menu_Button_init(self, x, y, w, h, label);
    return self;
}

void Fl_Menu_Button_draw(Fl_Widget *self_w) {
    int H, X, Y;
    if (!self_w->box || Fl_Widget_type(self_w)) return;

    H = (Fl_Widget_labelsize(self_w) - 3) & ~1;
    X = self_w->x + self_w->w - H - fl_box_dx(self_w->box) - fl_box_dw(self_w->box) - 1;
    Y = self_w->y + (self_w->h - H) / 2;

    fl_draw_box(g_pressed_menu_button == (Fl_Menu_ *)self_w ? fl_down(self_w->box) : self_w->box,
                self_w->x, self_w->y, self_w->w, self_w->h, self_w->color);
    Fl_Widget_draw_label_in(self_w, self_w->x + fl_box_dx(self_w->box), self_w->y, X - self_w->x + 2, self_w->h);
    if (Fl_focus() == self_w) Fl_Widget_draw_focus(self_w, self_w->box, self_w->x, self_w->y, self_w->w, self_w->h);

    fl_color(Fl_Widget_active_r(self_w) ? FL_DARK3 : fl_inactive(FL_DARK3));
    fl_line3(X + H / 2, Y + H, X, Y, X + H, Y);
    fl_color(Fl_Widget_active_r(self_w) ? FL_LIGHT3 : fl_inactive(FL_LIGHT3));
    fl_line(X + H, Y, X + H / 2, Y + H);
}

const Fl_Menu_Item *Fl_Menu_Button_popup(Fl_Menu_ *self) {
    Fl_Widget *self_w = &self->widget;
    const Fl_Menu_Item *m;
    Fl_Widget_Tracker wp;

    g_pressed_menu_button = self;
    Fl_Widget_redraw(self_w);
    Fl_Widget_Tracker_watch(&wp, self_w);

    if (!self_w->box || Fl_Widget_type(self_w)) {
        m = Fl_Menu_Item_popup(self->menu_, Fl_event_x(), Fl_event_y(), self, Fl_Menu_mvalue(self));
    } else {
        m = Fl_Menu_Item_pulldown(self->menu_, self_w->x, self_w->y, self_w->w, self_w->h, self, NULL, 0);
    }
    Fl_Menu_picked(self, m);
    g_pressed_menu_button = NULL;
    if (Fl_Widget_Tracker_exists(&wp)) Fl_Widget_redraw(self_w);
    Fl_Widget_Tracker_release(&wp);
    return m;
}

int Fl_Menu_Button_handle(Fl_Widget *self_w, int e) {
    Fl_Menu_ *self = (Fl_Menu_ *)self_w;
    if (!self->menu_ || !self->menu_->text) return 0;

    switch (e) {
        case FL_ENTER:
        case FL_LEAVE:
            return (self_w->box && !Fl_Widget_type(self_w)) ? 1 : 0;

        case FL_PUSH:
            if (!self_w->box) {
                if (Fl_event_button() != 3) return 0;
            } else if (Fl_Widget_type(self_w)) {
                if (!(Fl_Widget_type(self_w) & (1 << (Fl_event_button() - 1)))) return 0;
            }
            if (Fl_visible_focus()) Fl_set_focus(self_w);
            Fl_Menu_Button_popup(self);
            return 1;

        case FL_KEYBOARD:
            if (!self_w->box) return 0;
            if (Fl_event_key() == ' ' && !Fl_event_state_of(FL_SHIFT | FL_CTRL | FL_ALT | FL_META)) {
                Fl_Menu_Button_popup(self);
                return 1;
            }
            return 0;

        case FL_SHORTCUT:
            if (Fl_Widget_test_shortcut(self_w)) {
                Fl_Menu_Button_popup(self);
                return 1;
            }
            return Fl_Menu_test_shortcut(self) != NULL;

        case FL_FOCUS:
        case FL_UNFOCUS:
            if (self_w->box && Fl_visible_focus()) {
                Fl_Widget_redraw(self_w);
                return 1;
            }
            return 0;

        default:
            return 0;
    }
}
