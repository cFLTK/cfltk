/*
 * cfltk - Fl_Button.c
 * See include/cfltk/Fl_Button.h for the class-conversion notes.
 * Translated from src/Fl_Button.cxx.
 */
#include <stdlib.h>

#include "cfltk/Fl_Button.h"
#include "cfltk/Fl_Group.h"
#include "cfltk/Fl.h"
#include "cfltk/fl_draw.h"

const Fl_WidgetOps fl_button_ops = {
    Fl_Button_draw,
    Fl_Button_handle,
    NULL, /* resize: Fl_Widget's default */
    NULL, /* show: Fl_Widget's default */
    NULL, /* hide: Fl_Widget's default */
    Fl_Widget_base_destroy,
    NULL, /* as_group */
    NULL  /* as_window */
};

void Fl_Button_init(Fl_Button *self, int x, int y, int w, int h, const char *label) {
    Fl_Widget_init(&self->widget, &fl_button_ops, x, y, w, h, label);
    self->widget.box = FL_UP_BOX;
    self->down_box_ = FL_NO_BOX;
    self->value_ = 0;
    self->oldval = 0;
    self->shortcut_ = 0;
    self->widget.flags |= FL_WIDGET_SHORTCUT_LABEL;
}

Fl_Button *Fl_Button_new(int x, int y, int w, int h, const char *label) {
    Fl_Button *self = (Fl_Button *)malloc(sizeof(Fl_Button));
    Fl_Button_init(self, x, y, w, h, label);
    return self;
}

int Fl_Button_set_value(Fl_Button *self, int v) {
    v = v ? 1 : 0;
    self->oldval = (char)v;
    Fl_Widget_clear_changed(&self->widget);
    if (self->value_ != v) {
        self->value_ = (char)v;
        if (Fl_Widget_box(&self->widget)) Fl_Widget_redraw(&self->widget);
        else Fl_Widget_redraw_label(&self->widget);
        return 1;
    }
    return 0;
}

void Fl_Button_setonly(Fl_Button *self) {
    Fl_Group *g;
    int i;

    Fl_Button_set_value(self, 1);
    g = Fl_Widget_parent(&self->widget);
    if (!g) return;
    for (i = Fl_Group_children(g); i--;) {
        Fl_Widget *o = Fl_Group_child(g, i);
        if (o != &self->widget && Fl_Widget_type(o) == FL_RADIO_BUTTON) {
            Fl_Button_set_value((Fl_Button *)o, 0);
        }
    }
}

void Fl_Button_draw(Fl_Widget *self_w) {
    Fl_Button *self = (Fl_Button *)self_w;
    Fl_Color col;
    uchar box;

    if (Fl_Widget_type(self_w) == FL_HIDDEN_BUTTON) return;

    col = Fl_Button_value(self) ? Fl_Widget_selection_color(self_w) : Fl_Widget_color(self_w);
    box = Fl_Button_value(self)
              ? (self->down_box_ ? self->down_box_ : fl_down(Fl_Widget_box(self_w)))
              : Fl_Widget_box(self_w);
    fl_draw_box(box, self_w->x, self_w->y, self_w->w, self_w->h, col);
    Fl_Widget_draw_backdrop(self_w);

    if (Fl_Widget_labeltype(self_w) == FL_NORMAL_LABEL && Fl_Button_value(self)) {
        Fl_Color c = Fl_Widget_labelcolor(self_w);
        Fl_Widget_set_labelcolor(self_w, fl_contrast(c, col));
        Fl_Widget_draw_label(self_w);
        Fl_Widget_set_labelcolor(self_w, c);
    } else {
        Fl_Widget_draw_label(self_w);
    }

    if (Fl_focus() == self_w) Fl_Widget_draw_focus(self_w, box, self_w->x, self_w->y, self_w->w, self_w->h);
}

#define KEY_RELEASE_DELAY 0.15

/* At most one key-triggered "flash" pending at a time, matching
 * upstream's single static key_release_tracker. Heap-allocated because
 * it must outlive this call (the timer fires later); freed in
 * key_release_timeout(). */
static Fl_Widget_Tracker *g_key_release_tracker = NULL;

static void key_release_timeout(void *data) {
    Fl_Widget_Tracker *wt = (Fl_Widget_Tracker *)data;
    Fl_Button *btn;

    if (!wt) return;
    if (wt == g_key_release_tracker) g_key_release_tracker = NULL;
    if (wt->widget) {
        btn = (Fl_Button *)wt->widget;
        Fl_Button_set_value(btn, 0);
        Fl_Widget_redraw(&btn->widget);
    }
    Fl_Widget_Tracker_release(wt);
    free(wt);
}

void Fl_Button_simulate_key_action(Fl_Button *self) {
    if (g_key_release_tracker) {
        Fl_remove_timeout(key_release_timeout, g_key_release_tracker);
        key_release_timeout(g_key_release_tracker);
    }
    Fl_Button_set_value(self, 1);
    Fl_Widget_redraw(&self->widget);
    g_key_release_tracker = (Fl_Widget_Tracker *)malloc(sizeof(Fl_Widget_Tracker));
    Fl_Widget_Tracker_watch(g_key_release_tracker, &self->widget);
    Fl_add_timeout(KEY_RELEASE_DELAY, key_release_timeout, g_key_release_tracker);
}

int Fl_Button_handle(Fl_Widget *self_w, int event) {
    Fl_Button *self = (Fl_Button *)self_w;
    int newval;

    switch (event) {
        case FL_ENTER:
        case FL_LEAVE:
            return 1;

        case FL_PUSH:
            if (Fl_visible_focus() && Fl_Widget_handle(self_w, FL_FOCUS)) Fl_set_focus(self_w);
            /* fallthrough */
        case FL_DRAG:
            if (Fl_event_inside(self_w)) {
                newval = (Fl_Widget_type(self_w) == FL_RADIO_BUTTON) ? 1 : !self->oldval;
            } else {
                Fl_Widget_clear_changed(self_w);
                newval = self->oldval;
            }
            if (newval != self->value_) {
                self->value_ = (char)newval;
                Fl_Widget_set_changed(self_w);
                Fl_Widget_redraw(self_w);
                if (Fl_Widget_when(self_w) & FL_WHEN_CHANGED) Fl_Widget_do_callback(self_w);
            }
            return 1;

        case FL_RELEASE: {
            if (self->value_ == self->oldval) {
                if (Fl_Widget_when(self_w) & FL_WHEN_NOT_CHANGED) Fl_Widget_do_callback(self_w);
                return 1;
            }
            Fl_Widget_set_changed(self_w);
            if (Fl_Widget_type(self_w) == FL_RADIO_BUTTON) Fl_Button_setonly(self);
            else if (Fl_Widget_type(self_w) == FL_TOGGLE_BUTTON) self->oldval = self->value_;
            else {
                Fl_Widget_Tracker wp;
                Fl_Button_set_value(self, self->oldval);
                Fl_Widget_set_changed(self_w);
                if (Fl_Widget_when(self_w) & FL_WHEN_CHANGED) {
                    Fl_Widget_Tracker_watch(&wp, self_w);
                    Fl_Widget_do_callback(self_w);
                    if (!Fl_Widget_Tracker_exists(&wp)) return 1;
                    Fl_Widget_Tracker_release(&wp);
                }
            }
            if (Fl_Widget_when(self_w) & FL_WHEN_RELEASE) Fl_Widget_do_callback(self_w);
            return 1;
        }

        case FL_SHORTCUT:
            if (!(self->shortcut_ ? Fl_test_shortcut((Fl_Shortcut)self->shortcut_) : Fl_Widget_test_shortcut(self_w)))
                return 0;
            if (Fl_visible_focus() && Fl_Widget_handle(self_w, FL_FOCUS)) Fl_set_focus(self_w);
            goto triggered_by_keyboard;

        case FL_FOCUS:
        case FL_UNFOCUS:
            if (Fl_visible_focus()) {
                if (Fl_Widget_box(self_w) == FL_NO_BOX) {
                    Fl_Window *win = Fl_Widget_window(self_w);
                    int X = self_w->x > 0 ? self_w->x - 1 : 0;
                    int Y = self_w->y > 0 ? self_w->y - 1 : 0;
                    if (win) Fl_Widget_set_damage_area(FL_WIDGET(win), FL_DAMAGE_ALL, X, Y, self_w->w + 2, self_w->h + 2);
                } else {
                    Fl_Widget_redraw(self_w);
                }
                return 1;
            }
            return 0;

        case FL_KEYBOARD:
            if (Fl_focus() == self_w && Fl_event_key() == ' ' &&
                !Fl_event_state_of(FL_SHIFT | FL_CTRL | FL_ALT | FL_META)) {
                Fl_Widget_set_changed(self_w);
            triggered_by_keyboard: {
                Fl_Widget_Tracker wp;
                Fl_Widget_Tracker_watch(&wp, self_w);
                if (Fl_Widget_type(self_w) == FL_RADIO_BUTTON) {
                    if (!self->value_) {
                        Fl_Button_setonly(self);
                        if (Fl_Widget_when(self_w) & FL_WHEN_CHANGED) Fl_Widget_do_callback(self_w);
                    }
                } else if (Fl_Widget_type(self_w) == FL_TOGGLE_BUTTON) {
                    Fl_Button_set_value(self, !self->value_);
                    if (Fl_Widget_when(self_w) & FL_WHEN_CHANGED) Fl_Widget_do_callback(self_w);
                } else {
                    Fl_Button_simulate_key_action(self);
                }
                if (!Fl_Widget_Tracker_exists(&wp)) return 1;
                Fl_Widget_Tracker_release(&wp);
                if (Fl_Widget_when(self_w) & FL_WHEN_RELEASE) Fl_Widget_do_callback(self_w);
                return 1;
            }
            }
            return 0;

        default:
            return 0;
    }
}
