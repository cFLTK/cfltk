/*
 * cfltk - Fl_Wizard.c
 * See include/cfltk/Fl_Wizard.h for the class-conversion notes.
 * Translated from src/Fl_Wizard.cxx.
 */
#include <stdlib.h>

#include "cfltk/Fl_Wizard.h"
#include "cfltk/fl_draw.h"

static void update_child(Fl_Widget *w) {
    if (Fl_Widget_damage(w)) { Fl_Widget_draw(w); Fl_Widget_clear_damage(w, 0); }
}

static void wizard_draw(Fl_Widget *self_w) {
    Fl_Wizard *self = (Fl_Wizard *)self_w;
    Fl_Widget *kid = Fl_Wizard_value(self);

    if (Fl_Widget_damage(self_w) & FL_DAMAGE_ALL) {
        if (kid) {
            fl_draw_box(self_w->box, self_w->x, self_w->y, self_w->w, self_w->h, kid->color);
            Fl_Group_draw_child(&self->group, kid);
        } else {
            fl_draw_box(self_w->box, self_w->x, self_w->y, self_w->w, self_w->h, self_w->color);
        }
    } else if (kid) {
        update_child(kid);
    }
}

const Fl_WidgetOps fl_wizard_ops = {
    wizard_draw,
    Fl_Group_handle,
    Fl_Group_resize,
    NULL, NULL,
    Fl_Group_destroy,
    Fl_Group_as_group,
    NULL
};

void Fl_Wizard_init(Fl_Wizard *self, int x, int y, int w, int h, const char *label) {
    Fl_Group_init(&self->group, x, y, w, h, label);
    self->group.widget.ops = &fl_wizard_ops;
    self->group.widget.box = FL_THIN_UP_BOX;
    self->value_ = NULL;
}

Fl_Wizard *Fl_Wizard_new(int x, int y, int w, int h, const char *label) {
    Fl_Wizard *self = (Fl_Wizard *)malloc(sizeof(Fl_Wizard));
    Fl_Wizard_init(self, x, y, w, h, label);
    return self;
}

void Fl_Wizard_next(Fl_Wizard *self) {
    Fl_Group *g = &self->group;
    int i, n = Fl_Group_children(g);

    for (i = 0; i < n; i++) if (Fl_Widget_visible(Fl_Group_child(g, i))) break;

    if (i < n - 1) Fl_Wizard_set_value(self, Fl_Group_child(g, i + 1));
}

void Fl_Wizard_prev(Fl_Wizard *self) {
    Fl_Group *g = &self->group;
    int i, n = Fl_Group_children(g);

    for (i = 0; i < n; i++) if (Fl_Widget_visible(Fl_Group_child(g, i))) break;

    if (i > 0 && i < n) Fl_Wizard_set_value(self, Fl_Group_child(g, i - 1));
}

Fl_Widget *Fl_Wizard_value(Fl_Wizard *self) {
    Fl_Group *g = &self->group;
    int i, n = Fl_Group_children(g);
    Fl_Widget *kid = NULL;

    if (n == 0) return NULL;

    for (i = 0; i < n; i++) {
        Fl_Widget *w = Fl_Group_child(g, i);
        if (Fl_Widget_visible(w)) {
            if (kid) Fl_Widget_hide(w);
            else kid = w;
        }
    }

    if (!kid) {
        kid = Fl_Group_child(g, n - 1);
        Fl_Widget_show(kid);
    }

    return kid;
}

void Fl_Wizard_set_value(Fl_Wizard *self, Fl_Widget *kid) {
    Fl_Group *g = &self->group;
    int i, n = Fl_Group_children(g);

    if (n == 0) return;

    for (i = 0; i < n; i++) {
        Fl_Widget *w = Fl_Group_child(g, i);
        if (w == kid) {
            if (!Fl_Widget_visible(w)) Fl_Widget_show(w);
        } else {
            Fl_Widget_hide(w);
        }
    }
}
