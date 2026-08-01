/*
 * cfltk - Fl_Tooltip.c
 * See include/cfltk/Fl_Tooltip.h for the class-conversion notes.
 * Translated from src/Fl_Tooltip.cxx.
 */
#include <stdlib.h>
#include <string.h>

#include "cfltk/Fl_Tooltip.h"
#include "cfltk/Fl_Window.h"
#include "cfltk/Fl.h"
#include "cfltk/fl_draw.h"
#include "../backend/fl_backend.h"

/* -------------------------------------------------------------------
 * Style/config state (Fl_Tooltip's static members)
 * ---------------------------------------------------------------- */

static float s_delay = 1.0f;
static float s_hoverdelay = 0.2f;
/* fl_color_cube(FL_NUM_RED-1, FL_NUM_GREEN-1, FL_NUM_BLUE-2): the same
 * pale yellow upstream defaults to. */
static Fl_Color s_color;
static int s_color_init = 0;
static Fl_Color s_textcolor = FL_BLACK;
static Fl_Font s_font = FL_HELVETICA;
static Fl_Fontsize s_size = -1; /* -1 = use FL_NORMAL_SIZE, see size() */
static int s_margin_width = 3;
static int s_margin_height = 3;
static int s_wrap_width = 400;
static int s_enabled = 1;

static Fl_Widget *s_widget = NULL; /* current target widget */
static const char *s_tip = NULL;
static int s_anchor_y = 0, s_anchor_h = 0; /* Y,H from enter_area() */
static char s_recent_tooltip = 0;
static char s_recursion = 0;

float Fl_Tooltip_delay(void) { return s_delay; }
void Fl_Tooltip_set_delay(float f) { s_delay = f; }
float Fl_Tooltip_hoverdelay(void) { return s_hoverdelay; }
void Fl_Tooltip_set_hoverdelay(float f) { s_hoverdelay = f; }

int Fl_Tooltip_enabled(void) { return s_enabled; }
void Fl_Tooltip_enable(int b) { s_enabled = b ? 1 : 0; }
void Fl_Tooltip_disable(void) { Fl_Tooltip_enable(0); }

Fl_Font Fl_Tooltip_font(void) { return s_font; }
void Fl_Tooltip_set_font(Fl_Font f) { s_font = f; }
Fl_Fontsize Fl_Tooltip_size(void) { return s_size < 0 ? FL_NORMAL_SIZE : s_size; }
void Fl_Tooltip_set_size(Fl_Fontsize s) { s_size = s; }
Fl_Color Fl_Tooltip_color(void) {
    if (!s_color_init) { s_color = fl_color_cube(FL_NUM_RED - 1, FL_NUM_GREEN - 1, FL_NUM_BLUE - 2); s_color_init = 1; }
    return s_color;
}
void Fl_Tooltip_set_color(Fl_Color c) { s_color = c; s_color_init = 1; }
Fl_Color Fl_Tooltip_textcolor(void) { return s_textcolor; }
void Fl_Tooltip_set_textcolor(Fl_Color c) { s_textcolor = c; }
int Fl_Tooltip_margin_width(void) { return s_margin_width; }
void Fl_Tooltip_set_margin_width(int v) { s_margin_width = v; }
int Fl_Tooltip_margin_height(void) { return s_margin_height; }
void Fl_Tooltip_set_margin_height(int v) { s_margin_height = v; }
int Fl_Tooltip_wrap_width(void) { return s_wrap_width; }
void Fl_Tooltip_set_wrap_width(int v) { s_wrap_width = v; }

/* -------------------------------------------------------------------
 * The popup window (a plain border-0 Fl_Window with custom draw/handle,
 * same override-redirect trick as fl_menu_popup.c's g_menu.win).
 * ---------------------------------------------------------------- */

static Fl_Window *s_window = NULL;

/* Splits `s` on '\n' only -- no automatic word-wrap, see
 * Fl_Tooltip.h's "Known differences". Calls cb(line, len, userdata)
 * for each line. */
static void for_each_line(const char *s, void (*cb)(const char *, int, void *), void *ud) {
    const char *p = s;
    while (p) {
        const char *nl = strchr(p, '\n');
        int len = nl ? (int)(nl - p) : (int)strlen(p);
        cb(p, len, ud);
        p = nl ? nl + 1 : NULL;
    }
}

static void measure_line(const char *line, int len, void *ud) {
    int *acc = (int *)ud; /* acc[0] = max width, acc[1] = line count */
    int lw = (int)fl_width(line, len);
    if (lw > acc[0]) acc[0] = lw;
    acc[1]++;
}

static void draw_line(const char *line, int len, void *ud) {
    int *y = (int *)ud;
    int baseline = *y + fl_height() - fl_descent();
    fl_draw_text(line, len, Fl_Tooltip_margin_width(), baseline);
    *y += fl_height();
}

static void tooltip_draw(Fl_Widget *self_w) {
    int y = Fl_Tooltip_margin_height();
    fl_draw_box(FL_BORDER_BOX, 0, 0, self_w->w, self_w->h, Fl_Tooltip_color());
    fl_color(Fl_Tooltip_textcolor());
    fl_font(Fl_Tooltip_font(), Fl_Tooltip_size());
    if (s_tip) for_each_line(s_tip, draw_line, &y);
}

static int tooltip_handle(Fl_Widget *self_w, int event) {
    if (event == FL_PUSH || event == FL_KEYDOWN) {
        Fl_Widget_hide(self_w);
        return 1;
    }
    return Fl_Group_handle(self_w, event);
}

static const Fl_WidgetOps tooltip_ops = {
    tooltip_draw,
    tooltip_handle,
    Fl_Window_resize,
    Fl_Window_show,
    Fl_Window_hide,
    Fl_Window_destroy,
    NULL,
    NULL
};

static void ensure_window(void) {
    if (s_window) return;
    s_window = Fl_Window_new(0, 0, 10, 10, NULL);
    Fl_Group_end(&s_window->group);
    s_window->group.widget.ops = &tooltip_ops;
    s_window->group.widget.flags |= FL_WIDGET_TOOLTIP_WINDOW;
    Fl_Window_set_border(s_window, 0);
}

static void tooltip_layout(void) {
    int acc[2] = { 0, 0 };
    int ww, hh, ox, oy, sw, sh;
    Fl_Window *widget_win;

    fl_font(Fl_Tooltip_font(), Fl_Tooltip_size());
    if (s_tip) for_each_line(s_tip, measure_line, acc);
    ww = acc[0];
    hh = acc[1] * fl_height();
    if (ww > s_wrap_width) ww = s_wrap_width; /* capped, not wrapped -- see header */
    ww += Fl_Tooltip_margin_width() * 2;
    hh += Fl_Tooltip_margin_height() * 2;

    /* Absolute screen position: s_widget's own x()/y() are relative to
     * its enclosing window's drawable (see Fl_Group_draw_children()'s
     * "window vs. non-window" distinction in Fl_Group.c), and that
     * window's own x()/y() are its screen position -- summing the two
     * gives the widget's screen position, matching upstream's
     * Fl_TooltipBox::layout() loop over p->window() chains. */
    ox = Fl_event_x_root();
    oy = s_anchor_y + s_anchor_h + 2;
    if (s_widget) {
        oy += s_widget->y;
        widget_win = Fl_Widget_window(s_widget);
        if (widget_win) oy += FL_WIDGET(widget_win)->y;
    }

    fl_backend_screen_size(&sw, &sh);
    if (ox + ww > sw) ox = sw - ww;
    if (ox < 0) ox = 0;
    if (s_anchor_h > 30) {
        oy = Fl_event_y_root() + 13;
        if (oy + hh > sh) oy -= 23 + hh;
    } else {
        if (oy + hh > sh) oy -= (4 + hh + s_anchor_h);
    }
    if (oy < 0) oy = 0;

    Fl_Widget_resize(FL_WIDGET(s_window), ox, oy, ww, hh);
}

/* -------------------------------------------------------------------
 * current()
 * ---------------------------------------------------------------- */

Fl_Widget *Fl_Tooltip_current(void) { return s_widget; }

void Fl_Tooltip_set_current(Fl_Widget *w) {
    Fl_Widget *tw;
    Fl_Tooltip_exit(NULL);
    tw = w;
    for (;;) {
        if (!tw) return;
        if (Fl_Widget_tooltip(tw)) break;
        tw = FL_WIDGET(Fl_Widget_parent(tw));
    }
    s_widget = w;
}

/* -------------------------------------------------------------------
 * Timers
 * ---------------------------------------------------------------- */

static void recent_timeout(void *data) {
    (void)data;
    s_recent_tooltip = 0;
}

static int top_win_iconified(void) {
    Fl_Window *topwin;
    if (!s_widget) return 0;
    topwin = Fl_Widget_top_window(s_widget);
    if (!topwin) return 0;
    return !Fl_Widget_visible(FL_WIDGET(topwin));
}

static void tooltip_timeout(void *data) {
    (void)data;
    if (s_recursion) return;
    s_recursion = 1;
    if (!top_win_iconified()) {
        if (!s_tip || !*s_tip) {
            if (s_window) Fl_Widget_hide(FL_WIDGET(s_window));
        } else {
            ensure_window();
            tooltip_layout();
            Fl_Widget_redraw(FL_WIDGET(s_window));
            Fl_Widget_show(FL_WIDGET(s_window));
        }
    }
    Fl_remove_timeout(recent_timeout, NULL);
    s_recent_tooltip = 1;
    s_recursion = 0;
}

/* -------------------------------------------------------------------
 * enter_()/exit_()/enter_area()
 * ---------------------------------------------------------------- */

void Fl_Tooltip_enter(Fl_Widget *w) {
    Fl_Widget *tw;

    if (w && Fl_Widget_as_window(w) && (w->flags & FL_WIDGET_TOOLTIP_WINDOW)) {
        /* Mouse entered the floating tooltip bubble itself: relayout in
         * place, and if that didn't move it, leave the current tooltip
         * alone instead of resetting it (matches upstream's STR #2650
         * fix -- "if there's no better place for a tooltip window,
         * don't move it"). */
        int oldx = w->x, oldy = w->y;
        tooltip_layout();
        if (w->x == oldx && w->y == oldy) return;
    }

    tw = w;
    for (;;) {
        if (!tw) { Fl_Tooltip_exit(NULL); return; }
        if (tw == s_widget) return;
        if (Fl_Widget_tooltip(tw)) break;
        tw = FL_WIDGET(Fl_Widget_parent(tw));
    }
    Fl_Tooltip_enter_area(w, 0, 0, w->w, w->h, Fl_Widget_tooltip(tw));
}

void Fl_Tooltip_exit(Fl_Widget *w) {
    if (!s_widget || (w && FL_WIDGET(s_window) == w)) return;
    s_widget = NULL;
    Fl_remove_timeout(tooltip_timeout, NULL);
    Fl_remove_timeout(recent_timeout, NULL);
    if (s_window && Fl_Widget_visible(FL_WIDGET(s_window))) Fl_Widget_hide(FL_WIDGET(s_window));
    if (s_recent_tooltip) {
        if (Fl_event_state() & FL_BUTTONS) s_recent_tooltip = 0;
        else Fl_add_timeout(Fl_Tooltip_hoverdelay(), recent_timeout, NULL);
    }
}

void Fl_Tooltip_enter_area(Fl_Widget *wid, int x, int y, int w, int h, const char *tip) {
    (void)x; (void)w;

    if (s_recursion) return;
    if (!tip || !*tip || !Fl_Tooltip_enabled()) {
        Fl_Tooltip_exit(NULL);
        return;
    }
    if (wid == s_widget && tip == s_tip) return;
    Fl_remove_timeout(tooltip_timeout, NULL);
    Fl_remove_timeout(recent_timeout, NULL);
    s_widget = wid;
    s_anchor_y = y;
    s_anchor_h = h;
    s_tip = tip;
    if (s_recent_tooltip) {
        if (s_window) Fl_Widget_hide(FL_WIDGET(s_window));
        Fl_add_timeout(Fl_Tooltip_hoverdelay(), tooltip_timeout, NULL);
    } else if (Fl_Tooltip_delay() < .1f) {
        tooltip_timeout(NULL);
    } else {
        if (s_window && Fl_Widget_visible(FL_WIDGET(s_window))) Fl_Widget_hide(FL_WIDGET(s_window));
        Fl_add_timeout(Fl_Tooltip_delay(), tooltip_timeout, NULL);
    }
}

void Fl_Tooltip_widget_deleted(Fl_Widget *w) {
    if (w == s_widget) Fl_Tooltip_set_current(NULL);
    if (s_window && w == FL_WIDGET(s_window)) return; /* the popup itself is never a tooltip target */
    Fl_Tooltip_exit(w);
}
