/*
 * cfltk - Fl.c
 * See include/cfltk/Fl.h for the class-conversion notes.
 * Translated from src/Fl.cxx (event state, focus/pushed/belowmouse
 * bookkeeping, the run/wait/flush loop, the shown-window list).
 */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L /* clock_gettime()/CLOCK_MONOTONIC under strict -std=c99 */
#endif

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "cfltk/Fl.h"
#include "cfltk/Fl_Window.h"
#include "cfltk/Fl_Tooltip.h"
#include "cfltk/fl_draw.h"
#include "../backend/fl_backend.h"

#define READQUEUE_CAPACITY 64
#define CFLTK_MAX_TIMEOUTS 32

typedef struct Fl_Context {
    int event;
    int e_x, e_y, e_x_root, e_y_root, e_dx, e_dy;
    int e_button, e_clicks, e_is_click;
    int e_state;
    int e_key;
    char e_text[32];
    int e_length;
    /* NULL except while dispatching a synthesized FL_PASTE: points at a
     * clipboard buffer instead of e_text, so pasted text isn't limited
     * to e_text's small fixed size the way a single keystroke is. */
    const char *e_text_override;

    Fl_Widget *focus;
    Fl_Widget *pushed;
    Fl_Widget *belowmouse;

    Fl_Window *first_shown_window;

    Fl_Widget *readqueue[READQUEUE_CAPACITY];
    int readqueue_head;
    int readqueue_count;

    Fl_Widget_Tracker *trackers;

    int damage;
} Fl_Context;

static Fl_Context g_ctx;

/* -------------------------------------------------------------------
 * Timers
 * ---------------------------------------------------------------- */

typedef struct Fl_Timeout_Slot {
    double deadline;
    Fl_Timeout_Handler *cb;
    void *data;
    int active;
} Fl_Timeout_Slot;

static Fl_Timeout_Slot g_timeouts[CFLTK_MAX_TIMEOUTS];
/* Set only while a timeout callback is running, to the deadline that
 * just fired; Fl_repeat_timeout() schedules relative to this instead of
 * "now" so periodic repeats (e.g. Fl_Repeat_Button) don't drift by the
 * dispatch overhead of each cycle. */
static double g_firing_deadline = -1.0;

static double monotonic_now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

void Fl_add_timeout(double seconds, Fl_Timeout_Handler *cb, void *data) {
    int i;
    for (i = 0; i < CFLTK_MAX_TIMEOUTS; i++) {
        if (!g_timeouts[i].active) {
            g_timeouts[i].active = 1;
            g_timeouts[i].cb = cb;
            g_timeouts[i].data = data;
            g_timeouts[i].deadline = monotonic_now() + seconds;
            return;
        }
    }
    /* Pool exhausted: silently dropped. CFLTK_MAX_TIMEOUTS is sized well
     * above what any single widget tree needs concurrently; hitting this
     * indicates a leak (missing remove_timeout), not a legitimate need
     * for more slots. */
}

void Fl_repeat_timeout(double seconds, Fl_Timeout_Handler *cb, void *data) {
    double base = g_firing_deadline >= 0.0 ? g_firing_deadline : monotonic_now();
    int i;
    for (i = 0; i < CFLTK_MAX_TIMEOUTS; i++) {
        if (!g_timeouts[i].active) {
            g_timeouts[i].active = 1;
            g_timeouts[i].cb = cb;
            g_timeouts[i].data = data;
            g_timeouts[i].deadline = base + seconds;
            return;
        }
    }
}

void Fl_remove_timeout(Fl_Timeout_Handler *cb, void *data) {
    int i;
    for (i = 0; i < CFLTK_MAX_TIMEOUTS; i++) {
        if (g_timeouts[i].active && g_timeouts[i].cb == cb && g_timeouts[i].data == data) {
            g_timeouts[i].active = 0;
        }
    }
}

int Fl_has_timeout(Fl_Timeout_Handler *cb, void *data) {
    int i;
    for (i = 0; i < CFLTK_MAX_TIMEOUTS; i++) {
        if (g_timeouts[i].active && g_timeouts[i].cb == cb && g_timeouts[i].data == data) return 1;
    }
    return 0;
}

/* Returns seconds until the next deadline, clamped to
 * [0, requested_seconds]; fires every expired timeout as a side effect.
 * A slot is deactivated before its callback runs so a callback that
 * calls add_timeout()/remove_timeout() (including re-arming itself,
 * the common repeat-button pattern) can't corrupt the slot it's
 * currently occupying. */
static double process_timeouts(double requested_seconds) {
    double now = monotonic_now();
    double earliest = -1.0;
    int i;

    for (i = 0; i < CFLTK_MAX_TIMEOUTS; i++) {
        if (g_timeouts[i].active && g_timeouts[i].deadline <= now) {
            Fl_Timeout_Handler *cb = g_timeouts[i].cb;
            void *data = g_timeouts[i].data;
            double deadline = g_timeouts[i].deadline;
            g_timeouts[i].active = 0;
            g_firing_deadline = deadline;
            cb(data);
            g_firing_deadline = -1.0;
        }
    }

    for (i = 0; i < CFLTK_MAX_TIMEOUTS; i++) {
        if (g_timeouts[i].active && (earliest < 0.0 || g_timeouts[i].deadline < earliest))
            earliest = g_timeouts[i].deadline;
    }

    if (earliest < 0.0) return requested_seconds;
    {
        double remaining = earliest - now;
        if (remaining < 0.0) remaining = 0.0;
        return remaining < requested_seconds ? remaining : requested_seconds;
    }
}

/* -------------------------------------------------------------------
 * Event state
 * ---------------------------------------------------------------- */

int Fl_event(void) { return g_ctx.event; }
int Fl_event_x(void) { return g_ctx.e_x; }
int Fl_event_y(void) { return g_ctx.e_y; }
int Fl_event_x_root(void) { return g_ctx.e_x_root; }
int Fl_event_y_root(void) { return g_ctx.e_y_root; }
int Fl_event_dx(void) { return g_ctx.e_dx; }
int Fl_event_dy(void) { return g_ctx.e_dy; }
int Fl_event_button(void) { return g_ctx.e_button; }
int Fl_event_clicks(void) { return g_ctx.e_clicks; }
int Fl_event_is_click(void) { return g_ctx.e_is_click; }
int Fl_event_state(void) { return g_ctx.e_state; }
int Fl_event_state_of(int mask) { return g_ctx.e_state & mask; }
int Fl_event_key(void) { return g_ctx.e_key; }
const char *Fl_event_text(void) { return g_ctx.e_text_override ? g_ctx.e_text_override : g_ctx.e_text; }
int Fl_event_length(void) { return g_ctx.e_length; }

int Fl_event_inside_rect(int x, int y, int w, int h) {
    int xx = g_ctx.e_x - x, yy = g_ctx.e_y - y;
    return (xx >= 0 && xx < w && yy >= 0 && yy < h);
}
int Fl_event_inside(const Fl_Widget *wg) {
    return Fl_event_inside_rect(wg->x, wg->y, wg->w, wg->h);
}

void Fl_context_set_event_xy(int x, int y) { g_ctx.e_x = x; g_ctx.e_y = y; }

/* Translated from Fl::test_shortcut() in src/fl_shortcut.cxx, minus the
 * UTF-8 decode of event_text() (ASCII-only until fl_utf8decode() is
 * ported -- see docs/DESIGN.md). */
int Fl_test_shortcut(Fl_Shortcut shortcut) {
    unsigned int v, key, shift_state, mismatch, first_char;

    if (!shortcut) return 0;

    v = shortcut & FL_KEY_MASK;
    if ((unsigned)tolower((int)v) != v) shortcut |= FL_SHIFT;

    shift_state = (unsigned)g_ctx.e_state;
    if ((shortcut & shift_state) != (shortcut & 0x7fff0000u)) return 0;
    mismatch = (shortcut ^ shift_state) & 0x7fff0000u;
    if (mismatch & (FL_META | FL_ALT | FL_CTRL)) return 0;

    key = shortcut & FL_KEY_MASK;
    if (!(mismatch & FL_SHIFT) && key == (unsigned)g_ctx.e_key) return 1;

    first_char = g_ctx.e_text[0] ? (unsigned char)g_ctx.e_text[0] : 0;
    if (!(shift_state & FL_CAPS_LOCK) && key == first_char) return 1;

    if ((shift_state & FL_CTRL) && key >= 0x3f && key <= 0x5f && first_char == (key ^ 0x40)) return 1;
    return 0;
}

static int g_visible_focus = 1;
int Fl_visible_focus(void) { return g_visible_focus; }
void Fl_set_visible_focus(int v) { g_visible_focus = v; }

static int g_scrollbar_size = 16;
int Fl_scrollbar_size(void) { return g_scrollbar_size; }
void Fl_set_scrollbar_size(int w) { g_scrollbar_size = w; }

/* -------------------------------------------------------------------
 * Clipboard (in-process only, see Fl.h)
 * ---------------------------------------------------------------- */

static char *g_clipboard[2];
static int g_clipboard_len[2];

void Fl_copy(const char *text, int len, int clipboard) {
    char **buf;
    int slot = clipboard ? 1 : 0;
    if (!text || len <= 0) return;
    buf = &g_clipboard[slot];
    free(*buf);
    *buf = (char *)malloc((size_t)len);
    memcpy(*buf, text, (size_t)len);
    g_clipboard_len[slot] = len;
}

void Fl_paste(Fl_Widget *receiver, int clipboard) {
    int slot = clipboard ? 1 : 0;
    if (!g_clipboard[slot] || !g_clipboard_len[slot]) return;
    g_ctx.e_text_override = g_clipboard[slot];
    g_ctx.e_length = g_clipboard_len[slot];
    Fl_Widget_handle(receiver, FL_PASTE);
    g_ctx.e_text_override = NULL;
}

/* Called by the backend as it translates a native event; kept internal
 * (declared in fl_backend.h) since applications never construct events
 * by hand. */
void fl_backend_set_event_state(int x, int y, int x_root, int y_root,
                                 int dx, int dy, int button, int clicks,
                                 int is_click, int state, int key,
                                 const char *text, int length) {
    g_ctx.e_x = x; g_ctx.e_y = y;
    g_ctx.e_x_root = x_root; g_ctx.e_y_root = y_root;
    g_ctx.e_dx = dx; g_ctx.e_dy = dy;
    g_ctx.e_button = button;
    g_ctx.e_clicks = clicks;
    g_ctx.e_is_click = is_click;
    g_ctx.e_state = state;
    g_ctx.e_key = key;
    g_ctx.e_length = length < (int)sizeof(g_ctx.e_text) - 1 ? length : (int)sizeof(g_ctx.e_text) - 1;
    if (text && g_ctx.e_length > 0) memcpy(g_ctx.e_text, text, (size_t)g_ctx.e_length);
    g_ctx.e_text[g_ctx.e_length] = '\0';
    g_ctx.e_text_override = NULL;
}

/* -------------------------------------------------------------------
 * Focus / pushed / belowmouse
 * ---------------------------------------------------------------- */

Fl_Widget *Fl_focus(void) { return g_ctx.focus; }

void Fl_set_focus(Fl_Widget *o) {
    Fl_Widget *p = g_ctx.focus;
    if (o == p) return;
    g_ctx.focus = o;
    for (; p; p = Fl_Widget_parent(p) ? &Fl_Widget_parent(p)->widget : NULL) {
        Fl_Widget_handle(p, FL_UNFOCUS);
    }
}

Fl_Widget *Fl_pushed(void) { return g_ctx.pushed; }
void Fl_set_pushed(Fl_Widget *o) { g_ctx.pushed = o; }

Fl_Widget *Fl_belowmouse(void) { return g_ctx.belowmouse; }
void Fl_set_belowmouse(Fl_Widget *o) {
    Fl_Widget *p = g_ctx.belowmouse;
    if (o == p) return;
    g_ctx.belowmouse = o;
    for (; p && !Fl_Widget_contains(p, o); p = Fl_Widget_parent(p) ? &Fl_Widget_parent(p)->widget : NULL) {
        Fl_Widget_handle(p, FL_LEAVE);
    }
    /* Upstream calls Fl_Tooltip::enter(belowmouse()) at every one of its
     * own FL_MOVE/FL_DRAG/FL_ENTER call sites, each guarded by "did
     * belowmouse() actually change" -- since this setter already only
     * runs its body when it did (the early return above), hooking here
     * once covers all of those call sites centrally. */
    Fl_Tooltip_enter(o);
}

/* -------------------------------------------------------------------
 * Widget lifetime bookkeeping
 * ---------------------------------------------------------------- */

void Fl_context_widget_deleted(Fl_Widget *self) {
    Fl_Widget_Tracker *t;

    if (g_ctx.focus == self) g_ctx.focus = NULL;
    if (g_ctx.pushed == self) g_ctx.pushed = NULL;
    if (g_ctx.belowmouse == self) g_ctx.belowmouse = NULL;
    Fl_Tooltip_widget_deleted(self);

    for (t = g_ctx.trackers; t; t = t->next) {
        if (t->widget == self) t->widget = NULL;
    }
}

void Fl_Widget_Tracker_watch(Fl_Widget_Tracker *t, Fl_Widget *w) {
    t->widget = w;
    t->next = g_ctx.trackers;
    g_ctx.trackers = t;
}

void Fl_Widget_Tracker_release(Fl_Widget_Tracker *t) {
    Fl_Widget_Tracker **link = &g_ctx.trackers;
    while (*link) {
        if (*link == t) { *link = t->next; return; }
        link = &(*link)->next;
    }
}

/* -------------------------------------------------------------------
 * Read queue (Fl::readqueue(), backing Fl_Widget_default_callback)
 * ---------------------------------------------------------------- */

void Fl_context_push_readqueue(Fl_Widget *w) {
    if (g_ctx.readqueue_count >= READQUEUE_CAPACITY) return; /* drop, queue full */
    g_ctx.readqueue[(g_ctx.readqueue_head + g_ctx.readqueue_count) % READQUEUE_CAPACITY] = w;
    g_ctx.readqueue_count++;
}

Fl_Widget *Fl_readqueue(void) {
    Fl_Widget *w;
    if (g_ctx.readqueue_count == 0) return NULL;
    w = g_ctx.readqueue[g_ctx.readqueue_head];
    g_ctx.readqueue_head = (g_ctx.readqueue_head + 1) % READQUEUE_CAPACITY;
    g_ctx.readqueue_count--;
    return w;
}

/* -------------------------------------------------------------------
 * Redraw / damage
 * ---------------------------------------------------------------- */

void Fl_context_request_redraw(Fl_Widget *w) {
    Fl_Window *win = Fl_Widget_window(w);
    g_ctx.damage = 1;
    if (win && win != (Fl_Window *)w) win->group.widget.damage |= FL_DAMAGE_CHILD;
}

void Fl_context_widget_hidden(Fl_Widget *w, Fl_Window *win) {
    (void)w; (void)win;
}

void Fl_redraw(void) {
    Fl_Window *w;
    for (w = g_ctx.first_shown_window; w; w = w->next_shown) Fl_Widget_redraw(FL_WIDGET(w));
}

/* -------------------------------------------------------------------
 * Shown-window list
 * ---------------------------------------------------------------- */

void Fl_context_register_window(Fl_Window *win) {
    win->next_shown = g_ctx.first_shown_window;
    g_ctx.first_shown_window = win;
}

void Fl_context_unregister_window(Fl_Window *win) {
    Fl_Window **link = &g_ctx.first_shown_window;
    while (*link) {
        if (*link == win) { *link = win->next_shown; return; }
        link = &(*link)->next_shown;
    }
}

Fl_Window *Fl_first_window(void) { return g_ctx.first_shown_window; }
Fl_Window *Fl_next_window(const Fl_Window *window) { return window->next_shown; }

/* -------------------------------------------------------------------
 * Central event dispatch (simplified translation of Fl::handle_()).
 *
 * Known differences from upstream: no grab()/modal() stack (popup/dialog
 * support is future work), no drag-and-drop, no tooltip scheduling. See
 * docs/DESIGN.md.
 * ---------------------------------------------------------------- */

int Fl_context_handle(int event, Fl_Window *window) {
    Fl_Widget *wi = FL_WIDGET(window);
    g_ctx.event = event;
    if (!window) return 0;

    switch (event) {
        case FL_CLOSE:
            Fl_Widget_do_callback(wi);
            return 1;

        case FL_SHOW:
            Fl_Widget_default_show(wi);
            return 1;

        case FL_HIDE:
            Fl_Widget_default_hide(wi);
            return 1;

        case FL_PUSH:
            Fl_set_pushed(wi);
            Fl_Tooltip_set_current(wi);
            return Fl_Widget_handle(wi, event);

        case FL_MOVE:
        case FL_DRAG:
            if (g_ctx.pushed) { wi = g_ctx.pushed; event = FL_DRAG; g_ctx.event = event; }
            return Fl_Widget_handle(wi, event);

        case FL_RELEASE: {
            int ret;
            if (g_ctx.pushed) wi = g_ctx.pushed;
            ret = Fl_Widget_handle(wi, event);
            Fl_set_pushed(NULL);
            return ret;
        }

        case FL_KEYDOWN:
        case FL_KEYUP: {
            Fl_Widget *focus_w = g_ctx.focus;
            if (event == FL_KEYDOWN) Fl_Tooltip_enter(NULL);
            if (focus_w && Fl_Widget_handle(focus_w, event)) return 1;
            if (event == FL_KEYDOWN) return Fl_Widget_handle(wi, FL_SHORTCUT);
            return 0;
        }

        case FL_FOCUS:
            if (!g_ctx.focus || !Fl_Widget_contains(wi, g_ctx.focus)) {
                if (!Fl_Widget_take_focus(wi)) Fl_set_focus(wi);
            }
            return 1;

        case FL_UNFOCUS:
            Fl_set_focus(NULL);
            return 1;

        case FL_ENTER:
            return Fl_Widget_handle(wi, FL_MOVE);

        case FL_LEAVE:
            Fl_set_belowmouse(NULL);
            return 1;

        default:
            return Fl_Widget_handle(wi, event);
    }
}

/* -------------------------------------------------------------------
 * Main loop
 * ---------------------------------------------------------------- */

void Fl_flush(void) {
    Fl_Window *w;
    if (!g_ctx.damage) return;
    g_ctx.damage = 0;
    for (w = g_ctx.first_shown_window; w; w = w->next_shown) {
        if (!Fl_Widget_visible_r(FL_WIDGET(w))) continue;
        if (Fl_Widget_damage(FL_WIDGET(w))) {
            fl_backend_window_flush(w);
            Fl_Widget_clear_damage(FL_WIDGET(w), 0);
        }
    }
}

double Fl_wait_for(double seconds) {
    double clamped = process_timeouts(seconds);
    int got_event = fl_backend_wait(clamped);
    process_timeouts(0.0); /* fire anything whose deadline landed during the wait */
    Fl_flush();
    return got_event ? 1.0 : 0.0;
}

int Fl_wait(void) {
    if (!g_ctx.first_shown_window) return 0;
    Fl_wait_for(1e20);
    return g_ctx.first_shown_window != NULL;
}

int Fl_check(void) {
    Fl_wait_for(0.0);
    return g_ctx.first_shown_window != NULL;
}

int Fl_ready(void) { return fl_backend_ready(); }

int Fl_run(void) {
    while (g_ctx.first_shown_window) Fl_wait_for(1e20);
    return 0;
}
