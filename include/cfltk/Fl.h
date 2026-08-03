/*
 * cfltk - Fl.h
 *
 * C translation of FLTK 1.3 FL/Fl.H.
 *
 * Original class : Fl (a class with only static members -- never
 *                   instantiated).
 * New C structure : a private singleton `struct Fl_Context` defined in
 *                    Fl.c, reached only through the Fl_* free functions
 *                    declared here (event state, focus/pushed/belowmouse
 *                    widgets, the list of shown windows, the read queue).
 *                    Nothing about Fl_Context is exposed in this header;
 *                    callers only ever see Fl_event_x() etc., exactly as
 *                    callers of the C++ class only ever saw Fl::event_x().
 * Ownership       : the context owns nothing widgets don't already own;
 *                    it holds *observing* pointers (focus_, pushed_,
 *                    belowmouse_) that are cleared automatically when the
 *                    widget they point to is destroyed (see
 *                    Fl_context_widget_deleted, called from
 *                    Fl_Widget_base_destroy).
 * Known differences:
 *   - Timers, add_fd(), clipboard, and the multi-platform backend
 *     abstraction (Fl_X native handle table) are intentionally minimal or
 *     absent in this phase; see docs/DESIGN.md for the current status and
 *     the compile-time switches that will gate them.
 *   - grab()/modal() are not implemented yet: popups/dialogs are future
 *     work, tracked in docs/DESIGN.md.
 */
#ifndef CFLTK_FL_H
#define CFLTK_FL_H

#include "cfltk/Fl_Widget.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------
 * Event state -- mirrors Fl::event_x() and friends.
 * ---------------------------------------------------------------- */

int Fl_event(void);
int Fl_event_x(void);
int Fl_event_y(void);
int Fl_event_x_root(void);
int Fl_event_y_root(void);
int Fl_event_dx(void);
int Fl_event_dy(void);
int Fl_event_button(void);
int Fl_event_clicks(void);
int Fl_event_is_click(void);
int Fl_event_state(void);
int Fl_event_state_of(int mask);
int Fl_event_key(void);
const char *Fl_event_text(void);
int Fl_event_length(void);
int Fl_event_inside(const Fl_Widget *w);
int Fl_event_inside_rect(int x, int y, int w, int h);

/* Tests the current event (must be FL_KEYBOARD or FL_SHORTCUT) against a
 * Fl_Button::shortcut()-style value. Not to be confused with
 * Fl_Widget_test_shortcut(), which tests a widget's '&x' label shortcut. */
int Fl_test_shortcut(Fl_Shortcut shortcut);

/* -------------------------------------------------------------------
 * Global options -- mirrors Fl::visible_focus()/Fl::option(). Only the
 * one option cfltk currently needs is implemented; the rest of
 * Fl::Fl_Option is future work.
 * ---------------------------------------------------------------- */

int Fl_visible_focus(void);
void Fl_set_visible_focus(int v);

/* Mirrors Fl::scrollbar_size()/Fl::scrollbar_size(int): the default
 * trough thickness (px) for scrollbars that don't set their own via
 * Fl_Scrollbar_size(), e.g. Fl_Scroll's. Upstream defaults to 16. */
int Fl_scrollbar_size(void);
void Fl_set_scrollbar_size(int w);

/* Mirrors Fl::get_mouse(int&,int&): the live pointer position in root
 * (screen) coordinates, queried directly rather than read from the
 * last dispatched event -- see fl_backend_query_pointer(). */
void Fl_get_mouse(int *x_root, int *y_root);

/* Mirrors Fl::screen_xywh()/Fl::screen_work_area() for the single-
 * monitor case: cfltk assumes one screen at origin (0,0), matching the
 * existing convention in fl_menu_popup.c's clamp_to_screen(). */
void Fl_screen_xywh(int *x, int *y, int *w, int *h);
static inline void Fl_screen_work_area(int *x, int *y, int *w, int *h) { Fl_screen_xywh(x, y, w, h); }

/* Real physical screen DPI (X11: computed from DisplayWidthMM/HeightMM),
 * matching upstream's Fl::screen_dpi(h, v). Falls back to 96.0 if the
 * display reports a zero physical size. */
void Fl_screen_dpi(float *dpi_x, float *dpi_y);

/* -------------------------------------------------------------------
 * Clipboard -- mirrors Fl::copy()/Fl::paste(), restricted to an
 * in-process buffer (clipboard 0 = mouse/PRIMARY-style selection,
 * clipboard 1 = Ctrl-C/Ctrl-V-style clipboard). Known difference: does
 * not claim X11 selection ownership or answer other applications'
 * paste requests -- cut/copy/paste works within a cfltk process (e.g.
 * between two Fl_Input widgets) but not with the desktop clipboard.
 * See docs/DESIGN.md.
 * ---------------------------------------------------------------- */

void Fl_copy(const char *text, int len, int clipboard);
/* Synchronously delivers the stored clipboard text to `receiver` as an
 * FL_PASTE event (Fl_event_text()/Fl_event_length() are valid for the
 * duration of that call only). No-op if the clipboard is empty. */
void Fl_paste(Fl_Widget *receiver, int clipboard);

/* -------------------------------------------------------------------
 * Timers -- mirrors Fl::add_timeout()/repeat_timeout()/remove_timeout().
 *
 * Backed by a fixed-size slot pool (CFLTK_MAX_TIMEOUTS in Fl.c), not
 * per-call heap allocation: the embedded-systems requirement to avoid
 * heap fragmentation applies just as much to a repeat button held down
 * for a while as to anything else. Fl_wait_for() clamps its backend
 * wait to the earliest pending deadline and fires expired callbacks
 * before flushing damage, exactly like upstream's Fl::wait().
 * ---------------------------------------------------------------- */

typedef void (Fl_Timeout_Handler)(void *data);

void Fl_add_timeout(double seconds, Fl_Timeout_Handler *cb, void *data);
/* Schedules `seconds` after the expiration of the timeout currently
 * being handled, for more accurate periodic repeats than add_timeout()
 * called from inside its own callback; falls back to add_timeout()'s
 * behavior if called outside a firing callback. */
void Fl_repeat_timeout(double seconds, Fl_Timeout_Handler *cb, void *data);
void Fl_remove_timeout(Fl_Timeout_Handler *cb, void *data);
int Fl_has_timeout(Fl_Timeout_Handler *cb, void *data);

/* True idle callbacks, matching upstream's Fl::add_idle()/remove_idle():
 * `cb` fires once per Fl_wait_for() iteration for as long as it stays
 * registered, and - unlike a timeout - while any idle callback is
 * registered, Fl_wait_for() never blocks waiting for the next event
 * (clamped to a 0-second wait), so the event loop spins continuously
 * calling idle callbacks whenever nothing else is pending. Adding the
 * same (cb,data) pair twice is a no-op, matching upstream. Reuses
 * Fl_Timeout_Handler's void(*)(void*) shape (upstream's own
 * Fl_Idle_Handler has the same signature). */
void Fl_add_idle(Fl_Timeout_Handler *cb, void *data);
void Fl_remove_idle(Fl_Timeout_Handler *cb, void *data);
int Fl_has_idle(Fl_Timeout_Handler *cb, void *data);

/* -------------------------------------------------------------------
 * fd watching, matching upstream's Fl::add_fd()/remove_fd(): hooks a
 * file descriptor's readiness directly into the event loop's own
 * select() call (X11 backend), so a ready socket wakes Fl_wait_for()
 * the same way an X event does - no separate polling loop needed.
 * `when` is a bitmask of FL_READ/FL_WRITE/FL_EXCEPT. Removing only
 * some bits (not all the ones the fd was added with) leaves it
 * registered for the rest, matching upstream and Fl_remove_timeout()'s
 * own precedent.
 * ---------------------------------------------------------------- */

#define FL_READ   1
#define FL_WRITE  4
#define FL_EXCEPT 8

typedef void (Fl_FD_Handler)(int fd, void *data);

void Fl_add_fd(int fd, int when, Fl_FD_Handler *cb, void *data);
void Fl_remove_fd(int fd, int when);

/* -------------------------------------------------------------------
 * Threading -- matches upstream's Fl::lock()/unlock()/awake()/
 * thread_message(), split into Fl_awake() (plain-message form) and
 * Fl_awake_cb() (the Fl_Awake_Handler-callback overload) since C has
 * no overloading. Safe to call from any thread; a worker thread should
 * still call Fl_lock()/Fl_unlock() around any direct cfltk widget
 * access (both Fl_awake() forms are already thread-safe on their own
 * and don't need the lock just to queue a message/callback).
 * ---------------------------------------------------------------- */

/* Locks/unlocks a process-wide recursive mutex protecting cfltk's
 * internal state. Lazily initialized on first use, so single-threaded
 * programs (the common case - e.g. dillo, whose worker threads talk to
 * the main thread over a plain watched fd and never call into cfltk
 * directly) never pay for a mutex they don't use. */
void Fl_lock(void);
void Fl_unlock(void);

/* Queues `msg` and wakes a blocked Fl_wait_for() on the main thread;
 * the app retrieves queued messages itself via Fl_thread_message()
 * (e.g. from an Fl_add_idle() callback) - matches upstream's plain
 * Fl::awake(void*): the message is NOT auto-dispatched. Silently
 * dropped if the queue is full (matches Fl_add_timeout()'s existing
 * drop-on-exhaustion policy elsewhere in this header). */
void Fl_awake(void *msg);
/* Returns the next queued message (FIFO order), or NULL if none. */
void *Fl_thread_message(void);

/* Queues `cb` to run on the main thread (with `data`) and wakes a
 * blocked Fl_wait_for() - matches upstream's Fl::awake(Fl_Awake_Handler,
 * void*) overload: unlike Fl_awake() above, this IS auto-dispatched,
 * once per Fl_wait_for() pass, before delete-queue draining. Returns 0
 * on success, -1 if the callback queue is full (matches upstream's own
 * return convention). */
int Fl_awake_cb(void (*cb)(void *data), void *data);

/* -------------------------------------------------------------------
 * Focus / mouse-tracking widgets -- mirrors Fl::focus()/pushed()/
 * belowmouse(), split into getter/setter pairs since C has no overloading.
 * ---------------------------------------------------------------- */

Fl_Widget *Fl_focus(void);
void Fl_set_focus(Fl_Widget *w);

Fl_Widget *Fl_pushed(void);
void Fl_set_pushed(Fl_Widget *w);

Fl_Widget *Fl_belowmouse(void);
void Fl_set_belowmouse(Fl_Widget *w);

/* -------------------------------------------------------------------
 * Main loop.
 * ---------------------------------------------------------------- */

int Fl_run(void);
int Fl_wait(void);
double Fl_wait_for(double seconds);
int Fl_check(void);
int Fl_ready(void);
void Fl_flush(void);
void Fl_redraw(void);

/* Queues `w` for deletion once the event currently being processed has
 * finished (drained at the top of every Fl_wait_for() iteration, before
 * the next event is dispatched and before Fl_flush() redraws anything
 * else) - matches upstream's Fl::delete_widget(), safe to call from
 * inside a widget's own callback to remove/replace itself, unlike
 * Fl_Widget_delete() (immediate - freeing a widget while its own
 * callback is still executing is a use-after-free). Idempotent (queuing
 * the same widget twice is a no-op) and safe to call with multiple
 * widgets in an ancestor/descendant relationship - only the outermost
 * ancestor actually gets deleted, since deleting it already destroys
 * its children (see Fl_Widget_delete()'s own contract). */
void Fl_delete_widget(Fl_Widget *w);

Fl_Window *Fl_first_window(void);
Fl_Window *Fl_next_window(const Fl_Window *window);

/* Returns the currently-shown modal window, or NULL if none, matching
 * upstream's Fl::modal(). See Fl_Window_set_modal() (Fl_Window.h) for
 * how a window becomes modal. */
Fl_Window *Fl_modal(void);

/* Returns cfltk's own version string (e.g. "0.1.0"), matching upstream
 * FLTK's Fl::api_version() in spirit (a runtime-queryable version) -
 * previously the only place cfltk's version was recorded anywhere was
 * its pkg-config .pc file, unreachable at runtime. */
const char *Fl_api_version(void);

/* -------------------------------------------------------------------
 * Internal, cross-file API.
 *
 * C has no `friend`; these functions are the seams Fl_Widget.c,
 * Fl_Group.c, Fl_Window.c and the backend need to reach into the
 * context. They are not meant to be called from application code, but
 * unlike a real "private" interface there is no way to enforce that in
 * C beyond this comment.
 * ---------------------------------------------------------------- */

/* Called once from Fl_Widget_base_destroy(): clears focus_/pushed_/
 * belowmouse_ and any live Fl_Widget_Tracker if they reference `self`. */
void Fl_context_widget_deleted(Fl_Widget *self);

/* Simple FIFO used by the default widget callback (Fl::readqueue()). */
void Fl_context_push_readqueue(Fl_Widget *w);
Fl_Widget *Fl_readqueue(void);

/* Marks the widget (and the window it belongs to) as needing a
 * flush on the next Fl_flush()/Fl_wait() cycle. */
void Fl_context_request_redraw(Fl_Widget *w);

/* Called from Fl_Widget_default_hide(): lets the context drop
 * references to a widget that just left the visible tree. */
void Fl_context_widget_hidden(Fl_Widget *w, Fl_Window *win);

/* Window-list management, called only by Fl_Window.c / the backend. */
void Fl_context_register_window(Fl_Window *win);
void Fl_context_unregister_window(Fl_Window *win);

/* Sets/clears the currently-modal window, called only by
 * Fl_Window.c's show()/hide(). Use Fl_modal() (public, below) to read
 * it. */
void Fl_context_set_modal_window(Fl_Window *win);

/* Temporarily offsets event_x()/event_y() while dispatching into a child
 * window's widget tree (upstream's Fl_Group.cxx `send()` helper does the
 * same save/subtract/restore around child->handle() for FL_WINDOW
 * children). Not reentrant-safe beyond the save/restore discipline the
 * caller must follow (subtract, dispatch, add back the same amount). */
void Fl_context_set_event_xy(int x, int y);

/* Central event entry point: the backend translates a native event into
 * an Fl_Event code + updated event state, then calls this to run FLTK's
 * dispatch (focus/pushed/belowmouse bookkeeping + Fl_Widget_handle).
 * Returns what the target widget's handle() returned. */
int Fl_context_handle(int event, Fl_Window *window);

/* -------------------------------------------------------------------
 * Fl_Widget_Tracker: a weak reference used to notice when a callback
 * deleted a widget out from under an in-progress event dispatch (e.g.
 * Fl_Group's FL_PUSH loop checking whether the child it just sent
 * FL_PUSH to is still alive before reading further fields from it).
 * ---------------------------------------------------------------- */

typedef struct Fl_Widget_Tracker {
    Fl_Widget *widget;
    struct Fl_Widget_Tracker *next;
} Fl_Widget_Tracker;

void Fl_Widget_Tracker_watch(Fl_Widget_Tracker *t, Fl_Widget *w);
void Fl_Widget_Tracker_release(Fl_Widget_Tracker *t);
static inline int Fl_Widget_Tracker_exists(const Fl_Widget_Tracker *t) { return t->widget != NULL; }

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_H */
