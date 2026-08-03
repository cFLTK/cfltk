/*
 * cfltk - fl_backend.h (internal)
 *
 * The seam between the platform-independent core (Fl.c, Fl_Window.c) and
 * a concrete platform backend (src/backend/x11). A backend must provide
 * every function declared here, and must call fl_backend_set_event_state()
 * + Fl_context_handle() (from cfltk/Fl.h) as it translates native events.
 *
 * This header is not installed; it is not part of cfltk's public API.
 * A NuttX/NX backend implements the exact same surface in
 * src/backend/nx/ without touching core/.
 */
#ifndef CFLTK_BACKEND_H
#define CFLTK_BACKEND_H

#include "cfltk/Fl_Window.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Lazily called the first time any window is shown. Idempotent. Opens
 * the display, installs the Fl_Graphics_Driver, initializes fonts. */
int fl_backend_init(void);
void fl_backend_shutdown(void);

void fl_backend_window_create(Fl_Window *win);
void fl_backend_window_show(Fl_Window *win);
void fl_backend_window_hide(Fl_Window *win);
void fl_backend_window_destroy(Fl_Window *win);
/* Re-syncs the native window's geometry after Fl_Widget's x/y/w/h have
 * already been updated. No-op if the window isn't shown yet. */
void fl_backend_window_reshape(Fl_Window *win);

/* Re-syncs the native window's title (WM_NAME) and icon/taskbar label
 * (WM_ICON_NAME) after Fl_Window's label_copy/icon_label_copy have
 * already been updated. No-op if the window isn't shown yet (the
 * initial values get applied once at window-creation time instead, see
 * fl_backend_window_create()). */
void fl_backend_window_relabel(Fl_Window *win);

/* Blits/exposes whatever cfltk drew into the window's damaged area. */
void fl_backend_window_flush(Fl_Window *win);

/* Processes pending native events, blocking up to timeout_secs if the
 * queue is empty (a huge value, e.g. 1e20, blocks "forever"). Returns
 * non-zero if at least one event was translated and dispatched. */
int fl_backend_wait(double timeout_secs);

/* Non-blocking: true if fl_backend_wait(0) would find something to do. */
int fl_backend_ready(void);

/* Redirects ALL subsequent pointer and keyboard events to `win`
 * regardless of which physical window is under the cursor (X11:
 * XGrabPointer/XGrabKeyboard with owner_events=False), until
 * fl_backend_ungrab(). Used only by the menu popup engine
 * (src/menu/fl_menu_popup.c) to implement click-outside-to-dismiss and
 * to keep tracking the pointer past a popup's edges without needing
 * cfltk's own Fl::grab()/modal() stack (not implemented -- see
 * docs/DESIGN.md). */
void fl_backend_grab(Fl_Window *win);
void fl_backend_ungrab(void);

/* Screen dimensions in pixels, for clamping popup menu geometry on
 * screen. */
void fl_backend_screen_size(int *w, int *h);

/* Live pointer position in root/screen coordinates (X11: XQueryPointer
 * on the root window), independent of the last dispatched event --
 * needed by Fl_Window_hotspot() to center a not-yet-shown dialog under
 * the mouse before any event targeting it has been received. */
void fl_backend_query_pointer(int *x_root, int *y_root);

/* System beep (X11: XBell). `type` is an Fl_Beep value (fl_ask.h),
 * passed through as a plain int since this header must stay
 * self-contained/backend-agnostic. */
void fl_backend_beep(int type);

/* Fills in the core's event-state snapshot; called by the backend right
 * before Fl_context_handle(). Declared in Fl.c, not here, because it
 * writes to the core's private Fl_Context -- this prototype is the
 * cross-file seam for that one function. */
void fl_backend_set_event_state(int x, int y, int x_root, int y_root,
                                 int dx, int dy, int button, int clicks,
                                 int is_click, int state, int key,
                                 const char *text, int length);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_BACKEND_H */
