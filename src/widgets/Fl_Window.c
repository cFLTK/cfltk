/*
 * cfltk - Fl_Window.c
 * See include/cfltk/Fl_Window.h for the class-conversion notes.
 * Translated from src/Fl_Window.cxx / src/Fl_X.cxx (platform-independent
 * parts only; the native window itself lives in the backend).
 */
#include <stdlib.h>
#include <string.h>

#include "cfltk/Fl_Window.h"
#include "cfltk/Fl.h"
#include "cfltk/fl_draw.h"
#include "../backend/fl_backend.h"

Fl_Group *Fl_Window_as_group(Fl_Widget *self) { return (Fl_Group *)self; }
Fl_Window *Fl_Window_as_window(Fl_Widget *self) { return (Fl_Window *)self; }

/* Upstream: "Fl_Window has a default callback that calls Fl_Window::hide()." */
static void Fl_Window_default_callback(Fl_Widget *w, void *data) {
    (void)data;
    Fl_Widget_hide(w);
}

const Fl_WidgetOps fl_window_ops = {
    Fl_Group_draw,
    Fl_Group_handle,
    Fl_Window_resize,
    Fl_Window_show,
    Fl_Window_hide,
    Fl_Window_destroy,
    Fl_Window_as_group,
    Fl_Window_as_window
};

void Fl_Window_init(Fl_Window *self, int x, int y, int w, int h, const char *label) {
    /* Fl_Group_init() would auto-add to the currently-open group, which
     * is correct for child windows but must not happen before we've
     * finished setting up window-specific fields; upstream has the same
     * ordering constraint (Fl_Widget base ctor runs before Fl_Window's
     * own body). */
    Fl_Group_init(&self->group, x, y, w, h, label);
    self->group.widget.ops = &fl_window_ops;
    self->group.widget.box = FL_FLAT_BOX;
    self->group.widget.color = FL_GRAY;
    self->group.widget.label.align = FL_ALIGN_CENTER;
    Fl_Widget_set_callback(&self->group.widget, Fl_Window_default_callback, NULL);
    self->group.resizable_widget = NULL; /* upstream: NULL by default for windows, unlike Fl_Group */

    self->label_copy = NULL;
    self->backend_data = NULL;
    self->shown = 0;
    self->next_shown = NULL;
    self->double_buffered = 0;

    /* Fl_Group_init() already called begin() on this window, matching
     * upstream: the constructor opens the group for adding children, but
     * does NOT close it. Callers must call Fl_Group_end(&win->group) (or
     * add widgets to a different group first) once they're done adding
     * children -- exactly like `Fl_Window w(...); ...; w.end();` upstream. */
}

Fl_Window *Fl_Window_new(int x, int y, int w, int h, const char *label) {
    Fl_Window *self = (Fl_Window *)malloc(sizeof(Fl_Window));
    Fl_Window_init(self, x, y, w, h, label);
    return self;
}

void Fl_Window_destroy(Fl_Widget *self_w) {
    Fl_Window *self = (Fl_Window *)self_w;
    if (self->shown) Fl_Window_hide(self_w);
    free(self->label_copy);
    self->label_copy = NULL;
    Fl_Group_destroy(self_w);
}

void Fl_Window_show(Fl_Widget *self_w) {
    Fl_Window *self = (Fl_Window *)self_w;

    if (!self->shown) {
        if (!fl_backend_init()) return;
        fl_backend_window_create(self);
        self->shown = 1;
        Fl_context_register_window(self);
    }
    fl_backend_window_show(self);
    Fl_Widget_default_show(self_w);
    Fl_Widget_redraw(self_w);
}

void Fl_Window_hide(Fl_Widget *self_w) {
    Fl_Window *self = (Fl_Window *)self_w;
    if (!self->shown) return;
    fl_backend_window_hide(self);
    fl_backend_window_destroy(self);
    self->shown = 0;
    Fl_context_unregister_window(self);
    Fl_Widget_default_hide(self_w);
}

void Fl_Window_resize(Fl_Widget *self_w, int x, int y, int w, int h) {
    Fl_Window *self = (Fl_Window *)self_w;
    Fl_Group_resize(self_w, x, y, w, h);
    if (self->shown) fl_backend_window_reshape(self);
}

void Fl_Window_set_label(Fl_Window *self, const char *text) {
    free(self->label_copy);
    self->label_copy = NULL;
    if (text) {
        self->label_copy = (char *)malloc(strlen(text) + 1);
        memcpy(self->label_copy, text, strlen(text) + 1);
    }
    Fl_Widget_set_label(&self->group.widget, self->label_copy);
}

void Fl_Window_set_border(Fl_Window *self, int b) {
    if (b) self->group.widget.flags &= ~(unsigned)FL_WIDGET_NOBORDER;
    else self->group.widget.flags |= FL_WIDGET_NOBORDER;
}

void Fl_Window_flush(Fl_Window *self) {
    Fl_Widget_redraw(FL_WIDGET(self));
}

void Fl_Window_hotspot(Fl_Window *self, int X, int Y, int offscreen) {
    Fl_Widget *self_w = FL_WIDGET(self);
    int mx, my;

    Fl_get_mouse(&mx, &my);
    X = mx - X;
    Y = my - Y;

    if (!offscreen) {
        int scr_x, scr_y, scr_w, scr_h;
        /* Generic on-screen-border allowance for a decorated window;
         * same fixed KDE-ish defaults upstream's non-WIN32/non-APPLE
         * branch uses, since cfltk has no way to query the actual
         * window manager's frame size. */
        int top = 0, left = 0, right = 0, bottom = 0;

        Fl_screen_work_area(&scr_x, &scr_y, &scr_w, &scr_h);
        if (Fl_Window_border(self)) { top = 20; left = 4; right = 4; bottom = 8; }

        if (X + self_w->w + right > scr_w + scr_x) X = scr_w + scr_x - right - self_w->w;
        if (X - left < scr_x) X = left + scr_x;
        if (Y + self_w->h + bottom > scr_h + scr_y) Y = scr_h + scr_y - bottom - self_w->h;
        if (Y - top < scr_y) Y = top + scr_y;
    }

    Fl_Widget_position(self_w, X, Y);
}

void Fl_Window_hotspot_widget(Fl_Window *self, const Fl_Widget *w, int offscreen) {
    /* w's x()/y() are already relative to its enclosing top-level
     * window (cfltk keeps every widget's coordinates in that single
     * flat space, cascading updates through Fl_Group_resize() rather
     * than upstream's per-level relative offsets -- see
     * Fl_Group_draw_children()'s "window vs. non-window" split in
     * Fl_Group.c). Skips upstream's o->window() accumulation loop,
     * which exists to walk across *nested* window boundaries -- not a
     * supported/tested configuration here. */
    Fl_Window_hotspot(self, w->x + w->w / 2, w->y + w->h / 2, offscreen);
}
