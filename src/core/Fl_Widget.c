/*
 * cfltk - Fl_Widget.c
 * See include/cfltk/Fl_Widget.h for the class-conversion notes.
 */
#include <stdlib.h>
#include <string.h>

#include "cfltk/Fl_Widget.h"
#include "cfltk/Fl_Group.h"
#include "cfltk/Fl_Window.h"
#include "cfltk/Fl.h"
#include "cfltk/fl_draw.h"
#include "cfltk/Fl_Image.h"

void Fl_Widget_init(Fl_Widget *self, const Fl_WidgetOps *ops,
                     int x, int y, int w, int h, const char *label) {
    self->ops = ops;
    self->parent = NULL;
    self->callback = Fl_Widget_default_callback;
    self->user_data = NULL;

    self->x = x;
    self->y = y;
    self->w = w;
    self->h = h;

    self->label.value = label;
    self->label.image = NULL;
    self->label.deimage = NULL;
    self->label.font = FL_HELVETICA;
    self->label.size = 14;
    self->label.color = FL_FOREGROUND_COLOR;
    self->label.align = FL_ALIGN_CENTER;
    self->label.type = FL_NORMAL_LABEL;

    self->flags = FL_WIDGET_VISIBLE_FOCUS;
    self->color = FL_BACKGROUND_COLOR;
    self->color2 = FL_SELECTION_COLOR;
    self->type = 0;
    self->damage = 0;
    self->box = FL_NO_BOX;
    self->when = FL_WHEN_RELEASE;

    self->tooltip = NULL;

    /* Matches upstream: the Fl_Widget constructor itself does
     * `if (Fl_Group::current()) Fl_Group::current()->add(this);` so that
     * building a widget tree can rely purely on begin()/end() bracketing
     * (or the implicit begin() every Fl_Group/Fl_Window constructor
     * performs) instead of every call site adding children by hand. */
    {
        Fl_Group *cur = Fl_Group_current();
        if (cur) Fl_Group_add(cur, self);
    }
}

void Fl_Widget_base_destroy(Fl_Widget *self) {
    if (!self) return;

    if (self->parent) {
        Fl_Group_remove(self->parent, self);
    }

    if (self->flags & FL_WIDGET_COPIED_LABEL) {
        free((void *)self->label.value);
        self->label.value = NULL;
    }
    if (self->flags & FL_WIDGET_COPIED_TOOLTIP) {
        free((void *)self->tooltip);
        self->tooltip = NULL;
    }

    Fl_context_widget_deleted(self);
}

void Fl_Widget_destroy(Fl_Widget *self) {
    if (self && self->ops && self->ops->destroy) {
        self->ops->destroy(self);
    }
}

void Fl_Widget_delete(Fl_Widget *self) {
    if (!self) return;
    Fl_Widget_destroy(self);
    free(self);
}

void Fl_Widget_draw(Fl_Widget *self) {
    if (self->ops && self->ops->draw) self->ops->draw(self);
}

int Fl_Widget_handle(Fl_Widget *self, int event) {
    if (self->ops && self->ops->handle) return self->ops->handle(self, event);
    return Fl_Widget_default_handle(self, event);
}

void Fl_Widget_resize(Fl_Widget *self, int x, int y, int w, int h) {
    if (self->ops && self->ops->resize) self->ops->resize(self, x, y, w, h);
    else Fl_Widget_default_resize(self, x, y, w, h);
}

int Fl_Widget_damage_resize(Fl_Widget *self, int x, int y, int w, int h) {
    if (self->x == x && self->y == y && self->w == w && self->h == h) return 0;
    Fl_Widget_resize(self, x, y, w, h);
    Fl_Widget_redraw(self);
    return 1;
}

void Fl_Widget_show(Fl_Widget *self) {
    if (self->ops && self->ops->show) self->ops->show(self);
    else Fl_Widget_default_show(self);
}

void Fl_Widget_hide(Fl_Widget *self) {
    if (self->ops && self->ops->hide) self->ops->hide(self);
    else Fl_Widget_default_hide(self);
}

int Fl_Widget_default_handle(Fl_Widget *self, int event) {
    switch (event) {
        case FL_FOCUS:
        case FL_UNFOCUS:
            return (int)Fl_Widget_visible_focus(self);
        default:
            return 0;
    }
}

void Fl_Widget_default_resize(Fl_Widget *self, int x, int y, int w, int h) {
    self->x = x;
    self->y = y;
    self->w = w;
    self->h = h;
}

void Fl_Widget_default_show(Fl_Widget *self) {
    if (!Fl_Widget_visible(self)) {
        Fl_Widget_set_visible(self);
        if (self->parent) Fl_Widget_redraw(self);
        Fl_Widget_handle(self, FL_SHOW);
    }
}

void Fl_Widget_default_hide(Fl_Widget *self) {
    Fl_Window *parent_window;

    if (Fl_Widget_visible(self)) {
        Fl_Widget_clear_visible(self);
        if (self->parent) {
            self->parent->widget.damage |= FL_DAMAGE_CHILD;
            Fl_Widget_redraw(self->parent ? &self->parent->widget : NULL);
        }
        Fl_Widget_handle(self, FL_HIDE);
    }

    parent_window = Fl_Widget_window(self);
    if (parent_window) {
        Fl_context_widget_hidden(self, parent_window);
    }
}

void Fl_Widget_set_label(Fl_Widget *self, const char *text) {
    if (self->flags & FL_WIDGET_COPIED_LABEL) {
        free((void *)self->label.value);
        self->flags &= ~(unsigned)FL_WIDGET_COPIED_LABEL;
    }
    self->label.value = text;
}

void Fl_Widget_copy_label(Fl_Widget *self, const char *new_label) {
    if (self->flags & FL_WIDGET_COPIED_LABEL) {
        free((void *)self->label.value);
    }
    if (new_label) {
        char *copy = (char *)malloc(strlen(new_label) + 1);
        memcpy(copy, new_label, strlen(new_label) + 1);
        self->label.value = copy;
        self->flags |= FL_WIDGET_COPIED_LABEL;
    } else {
        self->label.value = NULL;
        self->flags &= ~(unsigned)FL_WIDGET_COPIED_LABEL;
    }
}

void Fl_Widget_set_tooltip(Fl_Widget *self, const char *text) {
    if (self->flags & FL_WIDGET_COPIED_TOOLTIP) {
        free((void *)self->tooltip);
        self->flags &= ~(unsigned)FL_WIDGET_COPIED_TOOLTIP;
    }
    self->tooltip = text;
}

void Fl_Widget_copy_tooltip(Fl_Widget *self, const char *text) {
    if (self->flags & FL_WIDGET_COPIED_TOOLTIP) free((void *)self->tooltip);
    if (text) {
        size_t n = strlen(text) + 1;
        char *copy = (char *)malloc(n);
        memcpy(copy, text, n);
        self->tooltip = copy;
        self->flags |= FL_WIDGET_COPIED_TOOLTIP;
    } else {
        self->tooltip = NULL;
        self->flags &= ~(unsigned)FL_WIDGET_COPIED_TOOLTIP;
    }
}

void Fl_Widget_do_callback(Fl_Widget *self) {
    Fl_Widget_do_callback_for(self, self, self->user_data);
}

void Fl_Widget_do_callback_for(Fl_Widget *self, Fl_Widget *target, void *arg) {
    Fl_Widget_Tracker wp;
    if (!self->callback) return;
    Fl_Widget_Tracker_watch(&wp, self);
    self->callback(target, arg);
    if (!Fl_Widget_Tracker_exists(&wp)) { Fl_Widget_Tracker_release(&wp); return; }
    Fl_Widget_Tracker_release(&wp);
    if (self->callback != Fl_Widget_default_callback) Fl_Widget_clear_changed(self);
}

void Fl_Widget_default_callback(Fl_Widget *widget, void *data) {
    (void)data;
    Fl_context_push_readqueue(widget);
}

int Fl_Widget_visible_r(const Fl_Widget *self) {
    const Fl_Widget *w = self;
    while (w) {
        if (!Fl_Widget_visible(w)) return 0;
        if (!w->parent) break;
        w = &w->parent->widget;
    }
    return 1;
}

int Fl_Widget_active_r(const Fl_Widget *self) {
    const Fl_Widget *w = self;
    while (w) {
        if (!Fl_Widget_active(w)) return 0;
        if (!w->parent) break;
        w = &w->parent->widget;
    }
    return 1;
}

void Fl_Widget_activate(Fl_Widget *self) {
    if (!Fl_Widget_active(self)) {
        Fl_Widget_set_active(self);
        if (Fl_Widget_active_r(self)) {
            Fl_Widget_redraw(self);
            Fl_Widget_handle(self, FL_ACTIVATE);
            if (Fl_Widget_as_group(self)) Fl_Group_activate_children(FL_GROUP(self));
        }
    }
}

void Fl_Widget_deactivate(Fl_Widget *self) {
    int was_active = Fl_Widget_active_r(self);
    self->flags |= FL_WIDGET_INACTIVE;
    if (was_active) {
        Fl_Widget_redraw(self);
        Fl_Widget_handle(self, FL_DEACTIVATE);
        if (Fl_Widget_as_group(self)) Fl_Group_deactivate_children(FL_GROUP(self));
    }
}

int Fl_Widget_take_focus(Fl_Widget *self) {
    if (!Fl_Widget_takesevents(self)) return 0;
    if (!Fl_Widget_visible_focus(self)) return 0;
    if (Fl_focus() == self) return 1;
    if (!Fl_Widget_handle(self, FL_FOCUS)) return 0;
    Fl_set_focus(self);
    return 1;
}

void Fl_Widget_set_damage(Fl_Widget *self, uchar c) {
    self->damage |= c;
    if (self->damage) Fl_context_request_redraw(self);
}

void Fl_Widget_set_damage_area(Fl_Widget *self, uchar c, int x, int y, int w, int h) {
    (void)x; (void)y; (void)w; (void)h; /* sub-rectangle damage tracking is a
        future optimization; for now any damage triggers a full-widget redraw,
        matching a valid (if less efficient) subset of upstream behavior. */
    Fl_Widget_set_damage(self, c);
}

void Fl_Widget_redraw(Fl_Widget *self) {
    Fl_Widget_set_damage(self, FL_DAMAGE_ALL);
}

void Fl_Widget_redraw_label(Fl_Widget *self) {
    if (self->parent) Fl_Widget_redraw(&self->parent->widget);
    else Fl_Widget_redraw(self);
}

int Fl_Widget_contains(const Fl_Widget *self, const Fl_Widget *w) {
    while (w) {
        if (w == self) return 1;
        w = w->parent ? &w->parent->widget : NULL;
    }
    return 0;
}

Fl_Window *Fl_Widget_window(const Fl_Widget *self) {
    const Fl_Widget *w = self;
    while (w) {
        Fl_Window *as_win = Fl_Widget_as_window((Fl_Widget *)w);
        if (as_win) return as_win;
        w = w->parent ? &w->parent->widget : NULL;
    }
    return NULL;
}

unsigned int Fl_Widget_label_shortcut(const char *t) {
    if (!t) return 0;
    for (;;) {
        if (*t == 0) return 0;
        if (*t == '&') {
            unsigned int s = (unsigned char)t[1];
            if (s == 0) return 0;
            if (s == '&') { t++; }
            else return s;
        }
        t++;
    }
}

int Fl_Widget_test_shortcut_str(const char *t, int require_alt) {
    unsigned int c, ls;
    const char *text;
    if (!t) return 0;
    if (require_alt && !Fl_event_state_of(FL_ALT)) return 0;
    text = Fl_event_text();
    c = text[0] ? (unsigned char)text[0] : 0;
    if (!c) return 0;
    ls = Fl_Widget_label_shortcut(t);
    return c == ls;
}

int Fl_Widget_test_shortcut(const Fl_Widget *self) {
    if (!(self->flags & FL_WIDGET_SHORTCUT_LABEL)) return 0;
    return Fl_Widget_test_shortcut_str(Fl_Widget_label(self), 0);
}

void Fl_Widget_draw_label_at(const Fl_Widget *self, int x, int y, int w, int h, Fl_Align align) {
    Fl_Label l1 = self->label;
    if (!Fl_Widget_active_r(self)) {
        l1.color = fl_inactive(l1.color);
        if (l1.deimage) l1.image = l1.deimage;
    }
    if (self->flags & FL_WIDGET_SHORTCUT_LABEL) fl_draw_shortcut = 1;
    fl_label_draw(&l1, x, y, w, h, align);
    fl_draw_shortcut = 0;
}

void Fl_Widget_draw_label_in(const Fl_Widget *self, int x, int y, int w, int h) {
    Fl_Align align = Fl_Widget_align(self);
    if ((align & FL_ALIGN_POSITION_MASK) && !(align & FL_ALIGN_INSIDE)) return;
    Fl_Widget_draw_label_at(self, x, y, w, h, align);
}

void Fl_Widget_draw_label(const Fl_Widget *self) {
    Fl_Align align = Fl_Widget_align(self);
    int X = self->x + fl_box_dx(self->box);
    int W = self->w - fl_box_dw(self->box);
    int Y = self->y + fl_box_dy(self->box);
    int H = self->h - fl_box_dh(self->box);
    if (W > 11 && (align & (FL_ALIGN_LEFT | FL_ALIGN_RIGHT))) { X += 3; W -= 6; }
    Fl_Widget_draw_label_in(self, X, Y, W, H);
}

void Fl_Widget_draw_focus(const Fl_Widget *self, uchar boxtype, int x, int y, int w, int h) {
    if (!Fl_visible_focus()) return;
    switch (boxtype) {
        case FL_DOWN_BOX:
        case FL_DOWN_FRAME:
        case FL_THIN_DOWN_BOX:
        case FL_THIN_DOWN_FRAME:
            x++; y++;
            break;
        default:
            break;
    }
    fl_color(fl_contrast(FL_BLACK, self->color));
    fl_line_style(FL_DOT, 0, NULL);
    fl_rect(x + fl_box_dx(boxtype), y + fl_box_dy(boxtype),
            w - fl_box_dw(boxtype) - 1, h - fl_box_dh(boxtype) - 1);
    fl_line_style(FL_SOLID, 0, NULL);
}

void Fl_Widget_draw_backdrop(const Fl_Widget *self) {
    Fl_Image *img;
    if (!(Fl_Widget_align(self) & FL_ALIGN_IMAGE_BACKDROP)) return;
    img = Fl_Widget_image(self);
    if (img && Fl_Widget_deimage(self) && !Fl_Widget_active_r(self)) img = Fl_Widget_deimage(self);
    if (img) Fl_Image_draw_at(img, self->x + (self->w - Fl_Image_w(img)) / 2, self->y + (self->h - Fl_Image_h(img)) / 2);
}

Fl_Window *Fl_Widget_top_window(const Fl_Widget *self) {
    Fl_Window *win = Fl_Widget_window(self);
    Fl_Window *last = win;
    while (win) {
        Fl_Widget *win_widget = FL_WIDGET(win);
        last = win;
        win = win_widget->parent ? Fl_Widget_window(&win_widget->parent->widget) : NULL;
    }
    return last;
}
