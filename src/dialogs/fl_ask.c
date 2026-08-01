/*
 * cfltk - fl_ask.c
 * See include/cfltk/fl_ask.h for the class-conversion notes.
 * Translated from src/fl_ask.cxx.
 */
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cfltk/fl_ask.h"
#include "cfltk/Fl.h"
#include "cfltk/Fl_Window.h"
#include "cfltk/Fl_Box.h"
#include "cfltk/Fl_Button.h"
#include "cfltk/Fl_Return_Button.h"
#include "cfltk/Fl_Input.h"
#include "cfltk/fl_draw.h"
#include "../backend/fl_backend.h"

static Fl_Window *message_form = NULL;
static Fl_Box *message = NULL;
static Fl_Box *icon = NULL;
static Fl_Button *dlg_button[3];
static Fl_Input *dlg_input = NULL;
static int ret_val = 0;     /* button return value: 0, 1, 2 */
static int win_closed = 0;  /* -1 = Escape, -2 = close button */
static const char *iconlabel = "?";
static char *message_title_default_ = NULL;
Fl_Font fl_message_font_ = FL_HELVETICA;
Fl_Fontsize fl_message_size_ = -1;
static int enable_hotspot = 1;
static char avoid_recursion = 0;

const char *fl_no = "No";
const char *fl_yes = "Yes";
const char *fl_ok = "OK";
const char *fl_cancel = "Cancel";
const char *fl_close = "Close";

/* -------------------------------------------------------------------
 * The dialog window
 * ---------------------------------------------------------------- */

static void close_dialog(int closed_flag) {
    ret_val = 0;
    win_closed = closed_flag;
    Fl_Widget_hide(FL_WIDGET(message_form));
}

static void button_cb(Fl_Widget *w, void *data) {
    (void)w;
    ret_val = (int)(intptr_t)data;
    win_closed = 0;
    Fl_Widget_hide(FL_WIDGET(message_form));
}

static void window_cb(Fl_Widget *w, void *data) {
    (void)w; (void)data;
    close_dialog(-2); /* WM close button, via the FL_CLOSE -> do_callback() path */
}

/* Escape isn't any dialog button's own shortcut, so trying the normal
 * handle() first and only falling back to this is equivalent to
 * upstream's real ordering (its top-level Fl::handle_() only reaches
 * its own "Escape closes window" fallback after every widget already
 * had a chance to claim FL_SHORTCUT) -- just scoped to this window
 * instead of a global Fl::modal() fallback cfltk doesn't have. */
static int ask_win_handle(Fl_Widget *self_w, int event) {
    if (Fl_Group_handle(self_w, event)) return 1;
    if (event == FL_SHORTCUT && Fl_event_key() == FL_Escape) {
        close_dialog(-1);
        return 1;
    }
    return 0;
}

static const Fl_WidgetOps ask_win_ops = {
    Fl_Group_draw,
    ask_win_handle,
    Fl_Window_resize,
    Fl_Window_show,
    Fl_Window_hide,
    Fl_Window_destroy,
    Fl_Window_as_group,
    Fl_Window_as_window
};

static Fl_Window *makeform(void) {
    Fl_Group *prev_group;
    int b, x;

    if (message_form) return message_form;

    prev_group = Fl_Group_current();
    Fl_Group_set_current(NULL);

    /* Fl_Window_new() begin()s itself (Fl_Group_init() -> Fl_Group_begin()),
     * so every widget constructed below up to Fl_Group_end() auto-adds. */
    message_form = Fl_Window_new(0, 0, 410, 103, NULL);
    Fl_Widget_set_callback(FL_WIDGET(message_form), window_cb, NULL);
    FL_WIDGET(message_form)->ops = &ask_win_ops;

    message = Fl_Box_new(60, 25, 340, 20, NULL);
    Fl_Widget_set_align(&message->widget, FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);

    dlg_input = Fl_Input_new(60, 37, 340, 23, NULL);
    Fl_Widget_hide(&dlg_input->widget);

    icon = Fl_Box_new(10, 10, 50, 50, NULL);
    Fl_Widget_set_box(&icon->widget, FL_THIN_UP_BOX);
    Fl_Widget_set_labelfont(&icon->widget, FL_TIMES_BOLD);
    Fl_Widget_set_labelsize(&icon->widget, 34);
    Fl_Widget_set_color(&icon->widget, FL_WHITE);
    Fl_Widget_set_labelcolor(&icon->widget, FL_BLUE);

    Fl_Group_end(&message_form->group); /* don't add the buttons automatically */

    for (b = 0, x = 310; b < 3; b++, x -= 100) {
        if (b == 1) dlg_button[b] = Fl_Return_Button_new(x, 70, 90, 23, NULL);
        else dlg_button[b] = Fl_Button_new(x, 70, 90, 23, NULL);
        Fl_Widget_set_align(&dlg_button[b]->widget, FL_ALIGN_INSIDE | FL_ALIGN_WRAP);
        Fl_Widget_set_callback(&dlg_button[b]->widget, button_cb, (void *)(intptr_t)b);
    }
    /* add left to right so tab order matches visual order */
    for (b = 2; b >= 0; b--) Fl_Group_add(&message_form->group, &dlg_button[b]->widget);

    Fl_Group_begin(&message_form->group);
    {
        Fl_Box *resizable_box = Fl_Box_new(60, 10, 110 - 60, 27, NULL);
        Fl_Group_set_resizable(&message_form->group, &resizable_box->widget);
    }
    Fl_Group_end(&message_form->group);

    Fl_Group_set_current(prev_group);
    return message_form;
}

/* -------------------------------------------------------------------
 * Layout
 * ---------------------------------------------------------------- */

static void resizeform(void) {
    int i;
    int message_w, message_h;
    int text_height;
    int button_w[3], button_h[3];
    int x, w, h, max_w, max_h;
    const int icon_size = 50;

    Fl_Widget_size(FL_WIDGET(message_form), 410, 103);

    fl_font(Fl_Widget_labelfont(&message->widget), Fl_Widget_labelsize(&message->widget));
    message_w = message_h = 0;
    fl_measure(Fl_Widget_label(&message->widget), &message_w, &message_h, 1);

    message_w += 10;
    message_h += 10;
    if (message_w < 340) message_w = 340;
    if (message_h < 30) message_h = 30;

    fl_font(Fl_Widget_labelfont(&dlg_button[0]->widget), Fl_Widget_labelsize(&dlg_button[0]->widget));

    memset(button_w, 0, sizeof(button_w));
    memset(button_h, 0, sizeof(button_h));

    max_h = 25;
    for (i = 0; i < 3; i++) {
        if (Fl_Widget_visible(&dlg_button[i]->widget)) {
            fl_measure(Fl_Widget_label(&dlg_button[i]->widget), &button_w[i], &button_h[i], 1);
            if (i == 1) button_w[1] += 20;
            button_w[i] += 30;
            button_h[i] += 10;
            if (button_h[i] > max_h) max_h = button_h[i];
        }
    }

    if (Fl_Widget_visible(&dlg_input->widget)) text_height = message_h + 25;
    else text_height = message_h;

    max_w = message_w + 10 + icon_size;
    w = button_w[0] + button_w[1] + button_w[2] - 10;
    if (w > max_w) max_w = w;

    message_w = max_w - 10 - icon_size;

    w = max_w + 20;
    h = max_h + 30 + text_height;

    Fl_Widget_size(FL_WIDGET(message_form), w, h);

    Fl_Widget_resize(&message->widget, 20 + icon_size, 10, message_w, message_h);
    Fl_Widget_resize(&icon->widget, 10, 10, icon_size, icon_size);
    Fl_Widget_set_labelsize(&icon->widget, icon_size - 10);
    Fl_Widget_resize(&dlg_input->widget, 20 + icon_size, 10 + message_h, message_w, 25);

    for (x = w, i = 0; i < 3; i++) {
        if (button_w[i]) {
            x -= button_w[i];
            Fl_Widget_resize(&dlg_button[i]->widget, x, h - 10 - max_h, button_w[i] - 10, max_h);
        }
    }
}

/* -------------------------------------------------------------------
 * innards()
 * ---------------------------------------------------------------- */

static int innards(const char *fmt, va_list ap, const char *b0, const char *b1, const char *b2) {
    char buffer[1024];
    const char *prev_icon_label;
    Fl_Group *cur_group;

    Fl_set_pushed(NULL); /* stop dragging (STR #2159) */
    avoid_recursion = 1;

    makeform();
    Fl_Widget_size(FL_WIDGET(message_form), 410, 103);

    if (!strcmp(fmt, "%s")) {
        Fl_Widget_set_label(&message->widget, va_arg(ap, const char *));
    } else {
        vsnprintf(buffer, sizeof(buffer), fmt, ap);
        Fl_Widget_set_label(&message->widget, buffer);
    }

    Fl_Widget_set_labelfont(&message->widget, fl_message_font_);
    Fl_Widget_set_labelsize(&message->widget, fl_message_size_ == -1 ? FL_NORMAL_SIZE : fl_message_size_);

    if (b0) { Fl_Widget_show(&dlg_button[0]->widget); Fl_Widget_set_label(&dlg_button[0]->widget, b0); Fl_Widget_position(&dlg_button[1]->widget, 210, 70); }
    else { Fl_Widget_hide(&dlg_button[0]->widget); Fl_Widget_position(&dlg_button[1]->widget, 310, 70); }
    if (b1) { Fl_Widget_show(&dlg_button[1]->widget); Fl_Widget_set_label(&dlg_button[1]->widget, b1); }
    else Fl_Widget_hide(&dlg_button[1]->widget);
    if (b2) { Fl_Widget_show(&dlg_button[2]->widget); Fl_Widget_set_label(&dlg_button[2]->widget, b2); }
    else Fl_Widget_hide(&dlg_button[2]->widget);

    prev_icon_label = Fl_Widget_label(&icon->widget);
    if (!prev_icon_label) Fl_Widget_set_label(&icon->widget, iconlabel);

    resizeform();

    if (Fl_Widget_visible(&dlg_button[1]->widget) && !Fl_Widget_visible(&dlg_input->widget))
        Fl_Widget_take_focus(&dlg_button[1]->widget);
    if (enable_hotspot)
        Fl_Window_hotspot_widget(message_form, &dlg_button[0]->widget, 0);
    if (b0 && Fl_Widget_label_shortcut(b0))
        Fl_Button_set_shortcut(dlg_button[0], 0);

    if (!Fl_Window_label(message_form) && message_title_default_)
        Fl_Window_set_label(message_form, message_title_default_);

    cur_group = Fl_Group_current();
    Fl_Widget_show(FL_WIDGET(message_form));
    Fl_Group_set_current(cur_group);
    while (Fl_Window_shown(message_form)) Fl_wait();

    Fl_Widget_set_label(&icon->widget, prev_icon_label);
    Fl_Window_set_label(message_form, NULL); /* reset window title */

    avoid_recursion = 0;
    return ret_val;
}

/* -------------------------------------------------------------------
 * Public entry points
 * ---------------------------------------------------------------- */

void fl_beep(int type) { fl_backend_beep(type); }

void fl_message(const char *fmt, ...) {
    va_list ap;
    if (avoid_recursion) return;
    va_start(ap, fmt);
    iconlabel = "i";
    innards(fmt, ap, NULL, fl_close, NULL);
    va_end(ap);
    iconlabel = "?";
}

void fl_alert(const char *fmt, ...) {
    va_list ap;
    if (avoid_recursion) return;
    va_start(ap, fmt);
    iconlabel = "!";
    innards(fmt, ap, NULL, fl_close, NULL);
    va_end(ap);
    iconlabel = "?";
}

int fl_ask(const char *fmt, ...) {
    va_list ap;
    int r;
    if (avoid_recursion) return 0;
    va_start(ap, fmt);
    r = innards(fmt, ap, fl_no, fl_yes, NULL);
    va_end(ap);
    return r;
}

int fl_choice(const char *fmt, const char *b0, const char *b1, const char *b2, ...) {
    va_list ap;
    int r;
    if (avoid_recursion) return 0;
    va_start(ap, b2);
    r = innards(fmt, ap, b0, b1, b2);
    va_end(ap);
    return r;
}

int fl_choice_n(const char *fmt, const char *b0, const char *b1, const char *b2, ...) {
    va_list ap;
    int r;
    if (avoid_recursion) return -3;
    va_start(ap, b2);
    r = innards(fmt, ap, b0, b1, b2);
    va_end(ap);
    if (win_closed != 0 && r == 0) return win_closed;
    return r;
}

static const char *input_innards(const char *fmt, va_list ap, const char *defstr, int type) {
    const char *r;

    makeform();
    Fl_Widget_size(FL_WIDGET(message_form), 410, 103);
    Fl_Widget_position(&message->widget, 60, 10);
    Fl_Input_set_input_type(dlg_input, type);
    Fl_Widget_show(&dlg_input->widget);
    Fl_Input_set_value_str(dlg_input, defstr);
    Fl_Widget_take_focus(&dlg_input->widget);

    r = innards(fmt, ap, fl_cancel, fl_ok, NULL) ? Fl_Input_value(dlg_input) : NULL;

    Fl_Widget_hide(&dlg_input->widget);
    Fl_Widget_position(&message->widget, 60, 25);
    return r;
}

const char *fl_input(const char *fmt, const char *defstr, ...) {
    va_list ap;
    const char *r;
    if (avoid_recursion) return NULL;
    va_start(ap, defstr);
    r = input_innards(fmt, ap, defstr, FL_NORMAL_INPUT);
    va_end(ap);
    return r;
}

const char *fl_password(const char *fmt, const char *defstr, ...) {
    va_list ap;
    const char *r;
    if (avoid_recursion) return NULL;
    va_start(ap, defstr);
    r = input_innards(fmt, ap, defstr, FL_SECRET_INPUT);
    va_end(ap);
    return r;
}

Fl_Widget *fl_message_icon(void) {
    makeform();
    return &icon->widget;
}

void fl_message_font(Fl_Font f, Fl_Fontsize s) {
    fl_message_font_ = f;
    fl_message_size_ = s;
}

void fl_message_hotspot(int enable) { enable_hotspot = enable ? 1 : 0; }
int fl_message_hotspot_get(void) { return enable_hotspot; }

void fl_message_title(const char *title) {
    makeform();
    Fl_Window_set_label(message_form, title);
}

void fl_message_title_default(const char *title) {
    if (message_title_default_) { free(message_title_default_); message_title_default_ = NULL; }
    if (title) {
        size_t n = strlen(title) + 1;
        message_title_default_ = (char *)malloc(n);
        memcpy(message_title_default_, title, n);
    }
}
