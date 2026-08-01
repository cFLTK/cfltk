/*
 * cfltk - Fl_Menu_.c
 * See include/cfltk/Fl_Menu_.h for the class-conversion notes.
 * Translated from src/Fl_Menu_.cxx (array management + picked()/
 * setonly()); add()/remove() are cfltk's own simplified flat-array
 * versions, not translations of the path-parsing src/Fl_Menu_add.cxx.
 */
#include <stdlib.h>
#include <string.h>

#include "cfltk/Fl_Menu_.h"
#include "cfltk/Fl.h"

void Fl_Menu__init(Fl_Menu_ *self, const Fl_WidgetOps *ops, int x, int y, int w, int h, const char *label) {
    Fl_Widget_init(&self->widget, ops, x, y, w, h, label);
    self->widget.flags |= FL_WIDGET_SHORTCUT_LABEL;
    self->widget.box = FL_UP_BOX;
    Fl_Widget_set_when(&self->widget, FL_WHEN_RELEASE_ALWAYS);
    Fl_Widget_set_selection_color(&self->widget, FL_SELECTION_COLOR);

    self->menu_ = NULL;
    self->value_ = NULL;
    self->alloc = 0;
    self->textfont_ = FL_HELVETICA;
    self->textsize_ = FL_NORMAL_SIZE;
    self->textcolor_ = FL_FOREGROUND_COLOR;
    self->down_box_ = FL_NO_BOX;
}

void Fl_Menu_clear(Fl_Menu_ *self) {
    if (self->alloc) {
        free(self->menu_);
        self->alloc = 0;
    }
    self->menu_ = NULL;
    self->value_ = NULL;
}

void Fl_Menu__destroy(Fl_Widget *self_w) {
    Fl_Menu_clear((Fl_Menu_ *)self_w);
    Fl_Widget_base_destroy(self_w);
}

void Fl_Menu_set_menu(Fl_Menu_ *self, const Fl_Menu_Item *m) {
    Fl_Menu_clear(self);
    self->value_ = self->menu_ = (Fl_Menu_Item *)m;
}

void Fl_Menu_copy(Fl_Menu_ *self, const Fl_Menu_Item *m, void *user_data) {
    int i, n = Fl_Menu_Item_size(m);
    Fl_Menu_Item *arr = (Fl_Menu_Item *)malloc((size_t)n * sizeof(Fl_Menu_Item));
    memcpy(arr, m, (size_t)n * sizeof(Fl_Menu_Item));
    Fl_Menu_set_menu(self, arr);
    self->alloc = 1;
    if (user_data) {
        for (i = 0; i < n; i++)
            if (arr[i].callback_) arr[i].user_data_ = user_data;
    }
}

int Fl_Menu_add(Fl_Menu_ *self, const char *label, Fl_Shortcut shortcut, Fl_Callback *cb, void *user_data, int flags) {
    int old_count = self->menu_ ? Fl_Menu_Item_size(self->menu_) : 0;
    int keep = old_count > 0 ? old_count - 1 : 0;
    Fl_Menu_Item *arr = (Fl_Menu_Item *)malloc((size_t)(keep + 2) * sizeof(Fl_Menu_Item));

    if (keep) memcpy(arr, self->menu_, (size_t)keep * sizeof(Fl_Menu_Item));

    memset(&arr[keep], 0, sizeof(Fl_Menu_Item));
    arr[keep].text = label;
    arr[keep].shortcut_ = (int)shortcut;
    arr[keep].callback_ = cb;
    arr[keep].user_data_ = user_data;
    arr[keep].flags = flags;

    memset(&arr[keep + 1], 0, sizeof(Fl_Menu_Item));

    if (self->alloc) free(self->menu_);
    self->menu_ = arr;
    self->alloc = 1;

    return keep;
}

void Fl_Menu_remove(Fl_Menu_ *self, int index) {
    int n = Fl_Menu_size(self);
    if (!self->menu_ || index < 0 || index >= n - 1) return;

    if (!self->alloc) {
        Fl_Menu_Item *copy = (Fl_Menu_Item *)malloc((size_t)n * sizeof(Fl_Menu_Item));
        memcpy(copy, self->menu_, (size_t)n * sizeof(Fl_Menu_Item));
        self->menu_ = copy;
        self->alloc = 1;
    }
    if (self->value_ == &self->menu_[index]) self->value_ = NULL;
    memmove(&self->menu_[index], &self->menu_[index + 1], (size_t)(n - index - 1) * sizeof(Fl_Menu_Item));
}

int Fl_Menu_find_index_item(const Fl_Menu_ *self, const Fl_Menu_Item *item) {
    int n = Fl_Menu_size(self);
    if (!self->menu_ || item < self->menu_ || item >= self->menu_ + n) return -1;
    return (int)(item - self->menu_);
}

int Fl_Menu_find_index_cb(const Fl_Menu_ *self, Fl_Callback *cb) {
    int i, n = Fl_Menu_size(self);
    for (i = 0; i < n; i++)
        if (self->menu_[i].callback_ == cb) return i;
    return -1;
}

int Fl_Menu_set_value(Fl_Menu_ *self, const Fl_Menu_Item *m) {
    Fl_Widget_clear_changed(&self->widget);
    if (self->value_ != m) {
        self->value_ = m;
        return 1;
    }
    return 0;
}

void Fl_Menu_setonly(Fl_Menu_ *self, Fl_Menu_Item *item) {
    if (Fl_Menu_find_index_item(self, item) < 0) return; /* not part of our menu */
    Fl_Menu_Item_setonly(item);
}

const Fl_Menu_Item *Fl_Menu_picked(Fl_Menu_ *self, const Fl_Menu_Item *v) {
    Fl_Widget *self_w = &self->widget;
    if (!v) return v;

    if (Fl_Menu_Item_radio(v)) {
        if (!Fl_Menu_Item_value(v)) {
            Fl_Widget_set_changed(self_w);
            Fl_Menu_setonly(self, (Fl_Menu_Item *)v);
        }
        Fl_Widget_redraw(self_w);
    } else if (v->flags & FL_MENU_TOGGLE) {
        Fl_Widget_set_changed(self_w);
        ((Fl_Menu_Item *)v)->flags ^= FL_MENU_VALUE;
        Fl_Widget_redraw(self_w);
    } else if (v != self->value_) {
        Fl_Widget_set_changed(self_w);
    }

    self->value_ = v;
    if (Fl_Widget_when(self_w) & (FL_WHEN_CHANGED | FL_WHEN_RELEASE)) {
        if (Fl_Widget_changed(self_w) || (Fl_Widget_when(self_w) & FL_WHEN_NOT_CHANGED)) {
            if (self->value_->callback_) Fl_Menu_Item_do_callback(self->value_, self_w);
            else Fl_Widget_do_callback(self_w);
        }
    }
    return v;
}
