/*
 * cfltk - Fl_Pack.c
 * See include/cfltk/Fl_Pack.h for the class-conversion notes.
 * Translated from src/Fl_Pack.cxx.
 */
#include <stdlib.h>

#include "cfltk/Fl_Pack.h"
#include "cfltk/fl_draw.h"

static void update_child(Fl_Widget *w) {
    if (Fl_Widget_damage(w)) { Fl_Widget_draw(w); Fl_Widget_clear_damage(w, 0); }
}

static void pack_draw(Fl_Widget *self_w) {
    Fl_Pack *self = (Fl_Pack *)self_w;
    int tx = self_w->x + fl_box_dx(self_w->box);
    int ty = self_w->y + fl_box_dy(self_w->box);
    int tw = self_w->w - fl_box_dw(self_w->box);
    int th = self_w->h - fl_box_dh(self_w->box);
    int rw, rh;
    int horiz = Fl_Pack_horizontal(self);
    int current_position = horiz ? tx : ty;
    int maximum_position = current_position;
    uchar d = Fl_Widget_damage(self_w);
    int i;

    if (horiz) {
        rw = -self->spacing_;
        rh = th;
        for (i = Fl_Group_children(&self->group); i--;) {
            Fl_Widget *c = Fl_Group_child(&self->group, i);
            if (Fl_Widget_visible(c)) {
                if (c != Fl_Group_resizable(&self->group)) rw += c->w;
                rw += self->spacing_;
            }
        }
    } else {
        rw = tw;
        rh = -self->spacing_;
        for (i = Fl_Group_children(&self->group); i--;) {
            Fl_Widget *c = Fl_Group_child(&self->group, i);
            if (Fl_Widget_visible(c)) {
                if (c != Fl_Group_resizable(&self->group)) rh += c->h;
                rh += self->spacing_;
            }
        }
    }

    for (i = 0; i < Fl_Group_children(&self->group); i++) {
        Fl_Widget *o = Fl_Group_child(&self->group, i);
        if (Fl_Widget_visible(o)) {
            int X, Y, W, H;
            int last = (i == Fl_Group_children(&self->group) - 1);
            if (horiz) { X = current_position; W = o->w; Y = ty; H = th; }
            else { X = tx; W = tw; Y = current_position; H = o->h; }

            if (last && o == Fl_Group_resizable(&self->group)) {
                if (horiz) W = tw - rw;
                else H = th - rh;
            }

            if (self->spacing_ && current_position > maximum_position && self_w->box &&
                (X != o->x || Y != o->y || (d & FL_DAMAGE_ALL))) {
                fl_color(self_w->color);
                if (horiz) fl_rectf(maximum_position, ty, self->spacing_, th);
                else fl_rectf(tx, maximum_position, tw, self->spacing_);
            }

            if (X != o->x || Y != o->y || W != o->w || H != o->h) {
                Fl_Widget_resize(o, X, Y, W, H);
                Fl_Widget_clear_damage(o, FL_DAMAGE_ALL);
            }
            if (d & FL_DAMAGE_ALL) {
                Fl_Group_draw_child(&self->group, o);
                Fl_Group_draw_outside_label(o);
            } else {
                update_child(o);
            }

            current_position += horiz ? o->w : o->h;
            if (current_position > maximum_position) maximum_position = current_position;
            current_position += self->spacing_;
        }
    }

    if (horiz) {
        if (maximum_position < tx + tw && self_w->box) {
            fl_color(self_w->color);
            fl_rectf(maximum_position, ty, tx + tw - maximum_position, th);
        }
        tw = maximum_position - tx;
    } else {
        if (maximum_position < ty + th && self_w->box) {
            fl_color(self_w->color);
            fl_rectf(tx, maximum_position, tw, ty + th - maximum_position);
        }
        th = maximum_position - ty;
    }

    tw += fl_box_dw(self_w->box); if (tw <= 0) tw = 1;
    th += fl_box_dh(self_w->box); if (th <= 0) th = 1;
    if (tw != self_w->w || th != self_w->h) {
        Fl_Widget_resize(self_w, self_w->x, self_w->y, tw, th);
        if (Fl_Widget_parent(self_w)) Fl_Group_init_sizes(Fl_Widget_parent(self_w));
        d = FL_DAMAGE_ALL;
    }
    if (d & FL_DAMAGE_ALL) {
        fl_draw_box(self_w->box, self_w->x, self_w->y, self_w->w, self_w->h, self_w->color);
        Fl_Widget_draw_label(self_w);
    }
}

const Fl_WidgetOps fl_pack_ops = {
    pack_draw,
    Fl_Group_handle,
    Fl_Group_resize,
    NULL, NULL,
    Fl_Group_destroy,
    Fl_Group_as_group,
    NULL
};

void Fl_Pack_init(Fl_Pack *self, int x, int y, int w, int h, const char *label) {
    Fl_Group_init(&self->group, x, y, w, h, label);
    self->group.widget.ops = &fl_pack_ops;
    self->group.resizable_widget = NULL;
    self->spacing_ = 0;
}

Fl_Pack *Fl_Pack_new(int x, int y, int w, int h, const char *label) {
    Fl_Pack *self = (Fl_Pack *)malloc(sizeof(Fl_Pack));
    Fl_Pack_init(self, x, y, w, h, label);
    return self;
}
