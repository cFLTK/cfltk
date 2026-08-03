/*
 * cfltk - Fl.c
 * See include/cfltk/Fl.h for the class-conversion notes.
 * Translated from src/Fl.cxx (event state, focus/pushed/belowmouse
 * bookkeeping, the run/wait/flush loop, the shown-window list).
 */
#ifndef _POSIX_C_SOURCE
/* 200809L (POSIX.1-2008) rather than just 199309L: still covers
 * clock_gettime()/CLOCK_MONOTONIC, and additionally exposes
 * PTHREAD_MUTEX_RECURSIVE for Fl_lock()'s mutex under strict -std=c99. */
#define _POSIX_C_SOURCE 200809L
#endif

#include <ctype.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

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
 * Idle callbacks
 * ---------------------------------------------------------------- */

#define CFLTK_MAX_IDLE 16

typedef struct Fl_Idle_Slot {
    Fl_Timeout_Handler *cb; /* same shape as a timeout handler: void(*)(void*) */
    void *data;
    int active;
} Fl_Idle_Slot;

static Fl_Idle_Slot g_idle[CFLTK_MAX_IDLE];
static int g_idle_count = 0; /* number of active slots, kept in sync so Fl_wait_for()'s hot path is a single comparison */

void Fl_add_idle(Fl_Timeout_Handler *cb, void *data) {
    int i;
    if (Fl_has_idle(cb, data)) return; /* matches upstream: adding twice is a no-op */
    for (i = 0; i < CFLTK_MAX_IDLE; i++) {
        if (!g_idle[i].active) {
            g_idle[i].active = 1;
            g_idle[i].cb = cb;
            g_idle[i].data = data;
            g_idle_count++;
            return;
        }
    }
    /* Pool exhausted: silently dropped, same policy as Fl_add_timeout(). */
}

void Fl_remove_idle(Fl_Timeout_Handler *cb, void *data) {
    int i;
    for (i = 0; i < CFLTK_MAX_IDLE; i++) {
        if (g_idle[i].active && g_idle[i].cb == cb && g_idle[i].data == data) {
            g_idle[i].active = 0;
            g_idle_count--;
        }
    }
}

int Fl_has_idle(Fl_Timeout_Handler *cb, void *data) {
    int i;
    for (i = 0; i < CFLTK_MAX_IDLE; i++) {
        if (g_idle[i].active && g_idle[i].cb == cb && g_idle[i].data == data) return 1;
    }
    return 0;
}

/* Calls every registered idle callback once. A callback that calls
 * Fl_remove_idle() on itself (the common one-shot-idle pattern, e.g.
 * dillo's own deferred-layout use) is safe: slots are snapshotted by
 * index range before iterating, and removed slots are simply skipped,
 * matching process_timeouts()'s same "deactivate before running"
 * discipline against self-modification. */
static void run_idle_callbacks(void) {
    int i;
    for (i = 0; i < CFLTK_MAX_IDLE; i++) {
        if (g_idle[i].active) g_idle[i].cb(g_idle[i].data);
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

void Fl_get_mouse(int *x_root, int *y_root) { fl_backend_query_pointer(x_root, y_root); }

void Fl_screen_xywh(int *x, int *y, int *w, int *h) {
    *x = 0;
    *y = 0;
    fl_backend_screen_size(w, h);
}

void Fl_screen_dpi(float *dpi_x, float *dpi_y) {
    fl_backend_screen_dpi(dpi_x, dpi_y);
}

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
 * Modality (Fl::modal(), see Fl_Window_set_modal() in Fl_Window.h)
 * ---------------------------------------------------------------- */

static Fl_Window *g_modal_window = NULL;

void Fl_context_set_modal_window(Fl_Window *win) { g_modal_window = win; }
Fl_Window *Fl_modal(void) { return g_modal_window; }

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

    /* Modal input gating (Fl::modal(), see Fl_Window_set_modal()): a
     * shown modal window blocks input events from reaching any *other*
     * window, matching upstream's Fl::handle_() modal check. Window-
     * management events (SHOW/HIDE/CLOSE) still pass through so a
     * blocked-but-still-visible window keeps repainting/responding to
     * the WM correctly - only the interactive input events a real user
     * could use to work around the modal dialog are dropped. */
    if (g_modal_window && window != g_modal_window) {
        switch (event) {
            case FL_PUSH: case FL_RELEASE: case FL_DRAG: case FL_MOVE:
            case FL_KEYDOWN: case FL_KEYUP: case FL_MOUSEWHEEL:
                return 0;
            default:
                break;
        }
    }

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

/* -------------------------------------------------------------------
 * Deferred widget deletion (Fl_delete_widget(), see Fl.h)
 * ------------------------------------------------------------------ */

#define DELETE_QUEUE_MAX 64
static Fl_Widget *g_delete_queue[DELETE_QUEUE_MAX];
static int g_delete_queue_count = 0;

void Fl_delete_widget(Fl_Widget *w) {
    int i;
    if (!w) return;
    for (i = 0; i < g_delete_queue_count; i++)
        if (g_delete_queue[i] == w) return; /* already queued */
    if (g_delete_queue_count < DELETE_QUEUE_MAX)
        g_delete_queue[g_delete_queue_count++] = w;
    else
        Fl_Widget_delete(w); /* queue full (should never happen in practice): fall back to immediate */
}

/* Actually destroys every queued widget, safe to call once the event
 * that queued them has finished processing (never from inside a
 * widget's own callback, which is the entire point of deferring this
 * in the first place - see Fl_delete_widget()'s header comment). Skips
 * any queued widget that is a descendant of another widget also in the
 * queue: Fl_Widget_delete() on a group already recursively destroys
 * its children (see Fl_Widget_delete()'s own contract), so deleting
 * both would double-free the descendant. */
static void Fl_process_delete_queue(void) {
    int i, j;
    if (!g_delete_queue_count) return;
    for (i = 0; i < g_delete_queue_count; i++) {
        int is_descendant = 0;
        for (j = 0; j < g_delete_queue_count; j++) {
            if (i == j) continue;
            if (Fl_Widget_contains(g_delete_queue[j], g_delete_queue[i])) {
                is_descendant = 1;
                break;
            }
        }
        if (!is_descendant)
            Fl_Widget_delete(g_delete_queue[i]);
    }
    g_delete_queue_count = 0;
}

/* -------------------------------------------------------------------
 * Threading: Fl_lock()/Fl_unlock()/Fl_awake()/Fl_thread_message()
 *
 * Matches upstream's Fl::lock()/unlock()/awake()/thread_message() (and
 * the Fl_Awake_Handler overload of awake(), split into Fl_awake_cb()
 * here since C has no overloading). Reuses the fd-watch registry from
 * Fl_add_fd() (already integrated into fl_backend_wait()'s select()
 * call) for the wake side: a self-pipe whose read end is registered
 * once, so a worker thread's Fl_awake()/Fl_awake_cb() interrupts a
 * blocked Fl_wait_for() the same way any other watched fd would,
 * rather than needing a second, backend-specific wake mechanism.
 * ---------------------------------------------------------------- */

#define CFLTK_MAX_AWAKE_MSGS 256
#define CFLTK_MAX_AWAKE_CBS 64

static pthread_mutex_t g_fl_mutex;
static pthread_once_t g_fl_mutex_once = PTHREAD_ONCE_INIT;
static int g_awake_pipe[2] = { -1, -1 };

static void *g_awake_msgs[CFLTK_MAX_AWAKE_MSGS];
static int g_awake_msg_head = 0, g_awake_msg_count = 0;

typedef struct { void (*cb)(void *data); void *data; } Fl_Awake_Cb_Slot;
static Fl_Awake_Cb_Slot g_awake_cbs[CFLTK_MAX_AWAKE_CBS];
static int g_awake_cb_head = 0, g_awake_cb_count = 0;
/* Fast-path hint checked every Fl_wait_for() call, so programs that
 * never touch threading (dillo included - its own worker threads talk
 * to the main thread over a plain fd, never call into cfltk directly)
 * don't pay a mutex lock/unlock on every pass through the event loop.
 * Plain `volatile` is not enough here (it blocks compiler reordering
 * but is not a synchronization primitive - a worker thread's unlocked
 * write and the main thread's unlocked read are still a data race
 * under the C memory model, confirmed by ThreadSanitizer even though
 * it's harmless in practice on x86); __atomic_* builtins (available
 * under -std=c99 as compiler intrinsics, not libc) give a real,
 * portable lock-free flag instead. Correctness never depends on this
 * hint alone - every place that acts on it still takes g_fl_mutex
 * before touching the actual queue, this only decides whether to
 * bother locking at all. */
static int g_awake_cb_pending = 0;

static void init_fl_mutex(void) {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&g_fl_mutex, &attr);
    pthread_mutexattr_destroy(&attr);
}

static void ensure_awake_pipe(void);

void Fl_lock(void) {
    pthread_once(&g_fl_mutex_once, init_fl_mutex);
    pthread_mutex_lock(&g_fl_mutex);
    /* Eagerly create (though not yet fd-watch-register, still
     * main-thread-only-safe, see register_awake_pipe_if_needed()) the
     * awake pipe here too: if the caller follows upstream's documented
     * "call Fl::lock() once from the main thread before starting any
     * other threads" pattern, this makes even the very first
     * Fl_awake()/Fl_awake_cb() call from a worker thread wake an
     * already-blocked Fl_wait_for() immediately, with no first-call
     * latency window. */
    ensure_awake_pipe();
}

void Fl_unlock(void) {
    pthread_once(&g_fl_mutex_once, init_fl_mutex);
    pthread_mutex_unlock(&g_fl_mutex);
}

/* Drains whatever bytes woke the pipe; the actual queued messages/
 * callbacks are handled separately (thread_message() polled by the
 * app, awake callbacks run from Fl_wait_for() below) - this callback's
 * only job is to make fl_backend_wait()'s select() return. */
static void awake_pipe_cb(int fd, void *data) {
    char buf[64];
    (void)data;
    while (read(fd, buf, sizeof(buf)) > 0) { }
}

/* Set once the pipe fds exist but before Fl_add_fd() has registered
 * the read end - checked/cleared from Fl_wait_for() (main thread
 * only, see below) so the actual Fl_add_fd() call, which mutates
 * fl_x11_event.c's shared g_fds[] array with no locking of its own,
 * never races against fl_backend_wait() concurrently reading that same
 * array. Creating the pipe itself (plain syscalls, no shared cfltk
 * state) is safe from any thread under the mutex; only the fd-watch
 * registration needs to be pushed onto the main thread. __atomic_*
 * builtins for the same reason as g_awake_cb_pending above - a plain
 * unlocked int read/write here is a real data race under the C memory
 * model (confirmed by ThreadSanitizer), even though correctness never
 * depends on this flag alone (every reader still takes g_fl_mutex
 * before acting on it being set). */
static int g_awake_pipe_needs_registration = 0;

static void ensure_awake_pipe(void) {
    if (g_awake_pipe[0] != -1) return;
    pthread_once(&g_fl_mutex_once, init_fl_mutex);
    pthread_mutex_lock(&g_fl_mutex);
    if (g_awake_pipe[0] == -1 && pipe(g_awake_pipe) == 0) {
        fcntl(g_awake_pipe[0], F_SETFL, fcntl(g_awake_pipe[0], F_GETFL) | O_NONBLOCK);
        fcntl(g_awake_pipe[1], F_SETFL, fcntl(g_awake_pipe[1], F_GETFL) | O_NONBLOCK);
        __atomic_store_n(&g_awake_pipe_needs_registration, 1, __ATOMIC_RELEASE);
    }
    pthread_mutex_unlock(&g_fl_mutex);
}

/* Called from any thread (including the main one); wakes a blocked
 * Fl_wait_for() so it notices the queued message/callback promptly
 * instead of waiting out its full timeout. */
static void wake_main_thread(void) {
    ensure_awake_pipe();
    if (g_awake_pipe[1] != -1) {
        char c = 0;
        ssize_t n = write(g_awake_pipe[1], &c, 1);
        (void)n; /* a full pipe (already-pending wake) is not an error */
    }
}

/* Main-thread-only: finishes registering the awake pipe with the
 * fd-watch registry if a (possibly worker-thread) call to
 * Fl_awake()/Fl_awake_cb() created it since the last pass. Must run
 * before fl_backend_wait() so the very wait call that would otherwise
 * block misses the wake is the one watching the pipe. */
static void register_awake_pipe_if_needed(void) {
    if (!__atomic_load_n(&g_awake_pipe_needs_registration, __ATOMIC_ACQUIRE)) return;
    pthread_mutex_lock(&g_fl_mutex);
    if (g_awake_pipe_needs_registration) {
        Fl_add_fd(g_awake_pipe[0], FL_READ, awake_pipe_cb, NULL);
        __atomic_store_n(&g_awake_pipe_needs_registration, 0, __ATOMIC_RELEASE);
    }
    pthread_mutex_unlock(&g_fl_mutex);
}

void Fl_awake(void *msg) {
    pthread_once(&g_fl_mutex_once, init_fl_mutex);
    pthread_mutex_lock(&g_fl_mutex);
    if (g_awake_msg_count < CFLTK_MAX_AWAKE_MSGS) {
        int tail = (g_awake_msg_head + g_awake_msg_count) % CFLTK_MAX_AWAKE_MSGS;
        g_awake_msgs[tail] = msg;
        g_awake_msg_count++;
    }
    pthread_mutex_unlock(&g_fl_mutex);
    wake_main_thread();
}

void *Fl_thread_message(void) {
    void *msg = NULL;
    pthread_once(&g_fl_mutex_once, init_fl_mutex);
    pthread_mutex_lock(&g_fl_mutex);
    if (g_awake_msg_count > 0) {
        msg = g_awake_msgs[g_awake_msg_head];
        g_awake_msg_head = (g_awake_msg_head + 1) % CFLTK_MAX_AWAKE_MSGS;
        g_awake_msg_count--;
    }
    pthread_mutex_unlock(&g_fl_mutex);
    return msg;
}

int Fl_awake_cb(void (*cb)(void *data), void *data) {
    int ok;
    pthread_once(&g_fl_mutex_once, init_fl_mutex);
    pthread_mutex_lock(&g_fl_mutex);
    ok = (g_awake_cb_count < CFLTK_MAX_AWAKE_CBS);
    if (ok) {
        int tail = (g_awake_cb_head + g_awake_cb_count) % CFLTK_MAX_AWAKE_CBS;
        g_awake_cbs[tail].cb = cb;
        g_awake_cbs[tail].data = data;
        g_awake_cb_count++;
        __atomic_store_n(&g_awake_cb_pending, 1, __ATOMIC_RELEASE);
    }
    pthread_mutex_unlock(&g_fl_mutex);
    wake_main_thread();
    return ok ? 0 : -1; /* matches upstream: 0 on success, -1 if the queue is full */
}

/* Runs every queued Fl_awake_cb() callback on the main thread. Matches
 * upstream: the callback-form of awake() is auto-dispatched by the
 * event loop, unlike the plain-message form (Fl_thread_message()),
 * which the app must poll for itself. */
static void run_awake_callbacks(void) {
    for (;;) {
        void (*cb)(void *data);
        void *data;
        pthread_mutex_lock(&g_fl_mutex);
        if (g_awake_cb_count == 0) {
            __atomic_store_n(&g_awake_cb_pending, 0, __ATOMIC_RELEASE);
            pthread_mutex_unlock(&g_fl_mutex);
            return;
        }
        cb = g_awake_cbs[g_awake_cb_head].cb;
        data = g_awake_cbs[g_awake_cb_head].data;
        g_awake_cb_head = (g_awake_cb_head + 1) % CFLTK_MAX_AWAKE_CBS;
        g_awake_cb_count--;
        pthread_mutex_unlock(&g_fl_mutex);
        cb(data);
    }
}

double Fl_wait_for(double seconds) {
    double clamped;
    int got_event;
    if (g_idle_count > 0) seconds = 0.0; /* never block while idle work is pending */
    register_awake_pipe_if_needed();
    clamped = process_timeouts(seconds);
    got_event = fl_backend_wait(clamped);
    process_timeouts(0.0); /* fire anything whose deadline landed during the wait */
    if (g_idle_count > 0) run_idle_callbacks();
    if (__atomic_load_n(&g_awake_cb_pending, __ATOMIC_ACQUIRE)) run_awake_callbacks();
    Fl_process_delete_queue();
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

#ifndef CFLTK_VERSION_STR
#define CFLTK_VERSION_STR "0.0.0" /* fallback if built outside the Makefile's -D */
#endif

const char *Fl_api_version(void) { return CFLTK_VERSION_STR; }
