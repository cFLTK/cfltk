/*
 * cfltk - Fl_File_Input.c
 * See include/cfltk/Fl_File_Input.h for the class-conversion notes.
 * Translated from src/Fl_File_Input.cxx.
 */
#include <stdlib.h>
#include <string.h>

#include "cfltk/Fl_File_Input.h"
#include "cfltk/Fl.h"
#include "cfltk/fl_draw.h"
#include "cfltk/fl_filename.h"

#define DIR_HEIGHT 10
#define FL_DAMAGE_BAR 0x10

static void update_buttons(Fl_File_Input *self) {
    int i;
    const char *start, *end;

    fl_font(Fl_Input_textfont(&self->input), Fl_Input_textsize(&self->input));

    for (i = 0, start = Fl_File_Input_value(self);
         start && i < (int)(sizeof(self->buttons_) / sizeof(self->buttons_[0]) - 1);
         start = end, i++) {
        end = strchr(start, '/');
        if (!end) break;
        end++;

        self->buttons_[i] = (short)fl_width(start, (int)(end - start));
        if (!i) self->buttons_[i] = (short)(self->buttons_[i] + fl_box_dx(self->input.widget.box) + 6);
    }
    self->buttons_[i] = 0;
}

static void draw_buttons(Fl_File_Input *self) {
    Fl_Widget *self_w = &self->input.widget;
    int i, X;

    if (Fl_Widget_damage(self_w) & (FL_DAMAGE_BAR | FL_DAMAGE_ALL)) update_buttons(self);

    for (X = 0, i = 0; self->buttons_[i]; i++) {
        if ((X + self->buttons_[i]) > self->input.xscroll_) {
            if (X < self->input.xscroll_) {
                fl_draw_box(self->pressed_ == i ? fl_down(self->down_box_) : self->down_box_,
                            self_w->x, self_w->y, X + self->buttons_[i] - self->input.xscroll_, DIR_HEIGHT, FL_GRAY);
            } else if ((X + self->buttons_[i] - self->input.xscroll_) > self_w->w) {
                fl_draw_box(self->pressed_ == i ? fl_down(self->down_box_) : self->down_box_,
                            self_w->x + X - self->input.xscroll_, self_w->y,
                            self_w->w - X + self->input.xscroll_, DIR_HEIGHT, FL_GRAY);
            } else {
                fl_draw_box(self->pressed_ == i ? fl_down(self->down_box_) : self->down_box_,
                            self_w->x + X - self->input.xscroll_, self_w->y, self->buttons_[i], DIR_HEIGHT, FL_GRAY);
            }
        }
        X += self->buttons_[i];
    }

    if (X < self_w->w) {
        fl_draw_box(self->pressed_ == i ? fl_down(self->down_box_) : self->down_box_,
                    self_w->x + X - self->input.xscroll_, self_w->y, self_w->w - X + self->input.xscroll_, DIR_HEIGHT, FL_GRAY);
    }
}

int Fl_File_Input_set_value(Fl_File_Input *self, const char *str, int len) {
    Fl_Widget_set_damage(&self->input.widget, FL_DAMAGE_BAR);
    return Fl_Input_set_value(&self->input, str, len);
}

void Fl_File_Input_draw(Fl_Widget *self_w) {
    Fl_File_Input *self = (Fl_File_Input *)self_w;
    uchar b = self_w->box;
    /* Keeps Fl_Input_draw_text_region from drawing a bogus box when
     * there's nothing to redraw and we're not focused -- matches
     * upstream's "must_trick_fl_input_" guard exactly. */
    int must_trick = Fl_focus() != self_w && !self->input.size_ && !(Fl_Widget_damage(self_w) & FL_DAMAGE_ALL);

    if (Fl_Widget_damage(self_w) & (FL_DAMAGE_BAR | FL_DAMAGE_ALL)) draw_buttons(self);

    if (must_trick) return;

    Fl_Input_draw_text_region(&self->input,
        self_w->x, self_w->y + DIR_HEIGHT, self_w->w, self_w->h - DIR_HEIGHT,
        self_w->x + fl_box_dx(b) + 3, self_w->y + fl_box_dy(b) + DIR_HEIGHT,
        self_w->w - fl_box_dw(b) - 6, self_w->h - fl_box_dh(b) - DIR_HEIGHT);
}

static int handle_button(Fl_File_Input *self, int event) {
    Fl_Widget *self_w = &self->input.widget;
    int i, X;
    char *start, *end;
    char newvalue[FL_PATH_MAX];

    for (X = 0, i = 0; self->buttons_[i]; i++) {
        X += self->buttons_[i];
        if (X > self->input.xscroll_ && Fl_event_x() < (self_w->x + X - self->input.xscroll_)) break;
    }

    self->pressed_ = (event == FL_RELEASE) ? -1 : (short)i;

    Fl_Widget_redraw(self_w); /* see "Known differences": deferred instead of upstream's synchronous mid-handler draw */

    if (!self->buttons_[i] || event != FL_RELEASE) return 1;

    strncpy(newvalue, Fl_File_Input_value(self), sizeof(newvalue) - 1);
    newvalue[sizeof(newvalue) - 1] = '\0';

    for (start = newvalue, end = start; start && i >= 0; start = end, i--) {
        end = strchr(start, '/');
        if (!end) break;
        end++;
    }

    if (i < 0) {
        *start = '\0';
        Fl_File_Input_set_value(self, newvalue, (int)(start - newvalue));

        Fl_Widget_set_changed(self_w);
        if (Fl_Widget_when(self_w) & (FL_WHEN_CHANGED | FL_WHEN_RELEASE)) Fl_Widget_do_callback(self_w);
    }

    return 1;
}

int Fl_File_Input_handle(Fl_Widget *self_w, int event) {
    Fl_File_Input *self = (Fl_File_Input *)self_w;
    static char in_button_bar = 0;

    switch (event) {
        case FL_MOVE:
        case FL_ENTER:
            /* Upstream also hints the mouse cursor here (window()->cursor());
             * cfltk has no cursor-shape API yet, see Known differences. */
            return 1;

        case FL_PUSH:
            in_button_bar = Fl_event_y() < (self_w->y + DIR_HEIGHT);
            /* fall through */
        case FL_RELEASE:
        case FL_DRAG:
            if (in_button_bar) return handle_button(self, event);
            return Fl_Input_handle(self_w, event);

        default: {
            if (Fl_Input_handle(self_w, event)) {
                Fl_Widget_set_damage(self_w, FL_DAMAGE_BAR);
                return 1;
            }
            return 0;
        }
    }
}

const Fl_WidgetOps fl_file_input_ops = {
    Fl_File_Input_draw,
    Fl_File_Input_handle,
    NULL, /* resize: Fl_Widget's default */
    NULL, /* show: Fl_Widget's default */
    NULL, /* hide: Fl_Widget's default */
    Fl_Input_destroy,
    NULL,
    NULL
};

void Fl_File_Input_init(Fl_File_Input *self, int x, int y, int w, int h, const char *label) {
    Fl_Input_init(&self->input, x, y, w, h, label);
    self->input.widget.ops = &fl_file_input_ops;
    self->buttons_[0] = 0;
    self->errorcolor_ = FL_RED;
    self->ok_entry_ = 1;
    self->pressed_ = -1;
    self->down_box_ = FL_UP_BOX;
}

Fl_File_Input *Fl_File_Input_new(int x, int y, int w, int h, const char *label) {
    Fl_File_Input *self = (Fl_File_Input *)malloc(sizeof(Fl_File_Input));
    Fl_File_Input_init(self, x, y, w, h, label);
    return self;
}
