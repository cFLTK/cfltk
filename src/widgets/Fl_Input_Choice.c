/*
 * cfltk - Fl_Input_Choice.c
 * See include/cfltk/Fl_Input_Choice.h for the class-conversion notes.
 * Translated from FL/Fl_Input_Choice.H + the out-of-line constructor in
 * src/Fl_Group.cxx.
 */
#include <stdlib.h>
#include <string.h>

#include "cfltk/Fl_Input_Choice.h"
#include "cfltk/Fl.h"
#include "cfltk/fl_draw.h"

/* -------------------------------------------------------------------
 * Private "InputMenuButton": an Fl_Menu_Button with a small triangle
 * instead of the usual down-arrow, matching upstream's private nested
 * class of the same name.
 * ---------------------------------------------------------------- */

static void input_menu_button_draw(Fl_Widget *self_w) {
    int xc, yc;
    fl_draw_box(FL_UP_BOX, self_w->x, self_w->y, self_w->w, self_w->h, Fl_Widget_color(self_w));
    fl_color(Fl_Widget_active_r(self_w) ? Fl_Widget_labelcolor(self_w) : fl_inactive(Fl_Widget_labelcolor(self_w)));
    xc = self_w->x + self_w->w / 2;
    yc = self_w->y + self_w->h / 2;
    fl_polygon3(xc - 5, yc - 3, xc + 5, yc - 3, xc, yc + 3);
    if (Fl_focus() == self_w) Fl_Widget_draw_focus(self_w, self_w->box, self_w->x, self_w->y, self_w->w, self_w->h);
}

static const Fl_WidgetOps input_menu_button_ops = {
    input_menu_button_draw,
    Fl_Menu_Button_handle,
    NULL, NULL, NULL,
    Fl_Menu__destroy,
    NULL, NULL
};

/* -------------------------------------------------------------------
 * Fl_Input_Choice
 * ---------------------------------------------------------------- */

static int inp_x(const Fl_Widget *self_w) { return self_w->x + fl_box_dx(self_w->box); }
static int inp_y(const Fl_Widget *self_w) { return self_w->y + fl_box_dy(self_w->box); }
static int inp_w(const Fl_Widget *self_w) { return self_w->w - fl_box_dw(self_w->box) - 20; }
static int inp_h(const Fl_Widget *self_w) { return self_w->h - fl_box_dh(self_w->box); }

static int menu_x(const Fl_Widget *self_w) { return self_w->x + self_w->w - 20 - fl_box_dx(self_w->box); }
static int menu_y(const Fl_Widget *self_w) { return self_w->y + fl_box_dy(self_w->box); }
static int menu_h(const Fl_Widget *self_w) { return self_w->h - fl_box_dh(self_w->box); }

static void menu_cb(Fl_Widget *w, void *data) {
    Fl_Input_Choice *o = (Fl_Input_Choice *)data;
    Fl_Widget *self_w = &o->group.widget;
    Fl_Widget_Tracker wp;
    const Fl_Menu_Item *item;
    (void)w;

    Fl_Widget_Tracker_watch(&wp, self_w);

    item = Fl_Menu_mvalue(o->menu_);
    if (item && Fl_Menu_Item_submenu(item)) { Fl_Widget_Tracker_release(&wp); return; } /* ignore submenus */

    {
        int idx = Fl_Menu_value(o->menu_);
        const char *text = (idx >= 0) ? Fl_Menu_Item_label(Fl_Menu_menu(o->menu_) + idx) : NULL;
        if (!text) text = "";

        if (!strcmp(Fl_Input_value(o->inp_), text)) {
            Fl_Widget_clear_changed(self_w);
            if (Fl_Widget_when(self_w) & FL_WHEN_NOT_CHANGED) Fl_Widget_do_callback(self_w);
        } else {
            Fl_Input_set_value_str(o->inp_, text);
            Fl_Widget_set_changed(&o->inp_->widget);
            Fl_Widget_set_changed(self_w);
            if (Fl_Widget_when(self_w) & (FL_WHEN_CHANGED | FL_WHEN_RELEASE)) Fl_Widget_do_callback(self_w);
        }
    }

    if (!Fl_Widget_Tracker_exists(&wp)) { Fl_Widget_Tracker_release(&wp); return; }

    if (Fl_Widget_callback(self_w) != Fl_Widget_default_callback) {
        Fl_Widget_clear_changed(self_w);
        Fl_Widget_clear_changed(&o->inp_->widget);
    }
    Fl_Widget_Tracker_release(&wp);
}

static void inp_cb(Fl_Widget *w, void *data) {
    Fl_Input_Choice *o = (Fl_Input_Choice *)data;
    Fl_Widget *self_w = &o->group.widget;
    Fl_Widget_Tracker wp;
    (void)w;

    Fl_Widget_Tracker_watch(&wp, self_w);

    if (Fl_Widget_changed(&o->inp_->widget)) {
        Fl_Widget_set_changed(self_w);
        if (Fl_Widget_when(self_w) & (FL_WHEN_CHANGED | FL_WHEN_RELEASE)) Fl_Widget_do_callback(self_w);
    } else {
        Fl_Widget_clear_changed(self_w);
        if (Fl_Widget_when(self_w) & FL_WHEN_NOT_CHANGED) Fl_Widget_do_callback(self_w);
    }

    if (!Fl_Widget_Tracker_exists(&wp)) { Fl_Widget_Tracker_release(&wp); return; }

    if (Fl_Widget_callback(self_w) != Fl_Widget_default_callback) Fl_Widget_clear_changed(self_w);
    Fl_Widget_Tracker_release(&wp);
}

static void Fl_Input_Choice_resize(Fl_Widget *self_w, int X, int Y, int W, int H) {
    Fl_Input_Choice *self = (Fl_Input_Choice *)self_w;
    Fl_Group_resize(self_w, X, Y, W, H);
    Fl_Widget_resize(&self->inp_->widget, inp_x(self_w), inp_y(self_w), inp_w(self_w), inp_h(self_w));
    Fl_Widget_resize(&self->menu_->widget, menu_x(self_w), menu_y(self_w), 20, menu_h(self_w));
}

const Fl_WidgetOps fl_input_choice_ops = {
    Fl_Group_draw,
    Fl_Group_handle,
    Fl_Input_Choice_resize,
    NULL, NULL,
    Fl_Group_destroy,
    Fl_Group_as_group,
    NULL
};

void Fl_Input_Choice_init(Fl_Input_Choice *self, int x, int y, int w, int h, const char *label) {
    Fl_Widget *self_w = &self->group.widget;

    Fl_Group_init(&self->group, x, y, w, h, label);
    self_w->ops = &fl_input_choice_ops;
    Fl_Widget_set_box(self_w, FL_DOWN_BOX);
    Fl_Widget_set_align(self_w, FL_ALIGN_LEFT);

    self->inp_ = Fl_Input_new(inp_x(self_w), inp_y(self_w), inp_w(self_w), inp_h(self_w), NULL);
    Fl_Widget_set_callback(&self->inp_->widget, inp_cb, self);
    Fl_Widget_set_box(&self->inp_->widget, FL_FLAT_BOX);
    Fl_Widget_set_when(&self->inp_->widget, FL_WHEN_CHANGED | FL_WHEN_NOT_CHANGED);

    self->menu_ = Fl_Menu_Button_new(menu_x(self_w), menu_y(self_w), 20, menu_h(self_w), NULL);
    self->menu_->widget.ops = &input_menu_button_ops;
    Fl_Widget_set_callback(&self->menu_->widget, menu_cb, self);
    /* box() itself is never consulted -- input_menu_button_draw() always
     * draws FL_UP_BOX regardless, matching upstream's InputMenuButton::
     * draw(), which does the same. Set to FL_FLAT_BOX only for fidelity
     * with upstream's own (equally vestigial) "cosmetic" box() call. */
    Fl_Widget_set_box(&self->menu_->widget, FL_FLAT_BOX);

    Fl_Group_end(&self->group);
}

Fl_Input_Choice *Fl_Input_Choice_new(int x, int y, int w, int h, const char *label) {
    Fl_Input_Choice *self = (Fl_Input_Choice *)malloc(sizeof(Fl_Input_Choice));
    Fl_Input_Choice_init(self, x, y, w, h, label);
    return self;
}

int Fl_Input_Choice_changed(const Fl_Input_Choice *self) {
    return Fl_Widget_changed(&self->inp_->widget) | Fl_Widget_changed(&self->group.widget);
}

void Fl_Input_Choice_clear_changed(Fl_Input_Choice *self) {
    Fl_Widget_clear_changed(&self->inp_->widget);
    Fl_Widget_clear_changed(&self->group.widget);
}

void Fl_Input_Choice_set_changed(Fl_Input_Choice *self) {
    Fl_Widget_set_changed(&self->inp_->widget);
}

void Fl_Input_Choice_set_value_index(Fl_Input_Choice *self, int val) {
    const Fl_Menu_Item *m = Fl_Menu_menu(self->menu_);
    Fl_Menu_set_value_index(self->menu_, val);
    Fl_Input_set_value_str(self->inp_, m ? Fl_Menu_Item_label(m + val) : "");
}
