/*
 * cfltk - fl_x11_event.c
 *
 * Translates native X11 events into Fl_* events and feeds them to
 * Fl_context_handle(). Translated in spirit from src/Fl_x.cxx
 * (fl_xevent handling inside Fl::wait()).
 */
#include <string.h>
#include <sys/select.h>

#include "fl_x11_internal.h"
#include "../fl_backend.h"
#include "cfltk/Fl.h"
#include "cfltk/Fl_Group.h"

static Fl_Window *find_window(Window xid) {
    Fl_Window *w;
    for (w = Fl_first_window(); w; w = Fl_next_window(w)) {
        Fl_X11_Window *xw = fl_x11_window_data(w);
        if (xw && xw->real_xid == xid) return w;
    }
    return NULL;
}

/* Click-multiplicity (double/triple-click) tracking, ported in spirit
 * from src/Fl_x.cxx's checkdouble()/set_event_xy(): a second ButtonPress
 * of the same button within 1000ms and 3px of the previous one bumps
 * the click count (Fl_event_clicks(): 0 for a plain click, 1 for a
 * double-click, 2 for a triple-click, ...); anything else resets it to
 * 0. Found missing (every click hardcoded clicks=1) while building and
 * interactively testing Fl_File_Browser's double-click-to-navigate
 * example -- nothing in the X11 backend had ever exercised
 * Fl_event_clicks() from a real double-click before. */
static int g_last_click_button = 0;
static int g_last_click_x = 0, g_last_click_y = 0;
static Time g_last_click_time = 0;
static int g_click_count = 0;

static int translate_button_state(unsigned int xstate) {
    int s = 0;
    if (xstate & ShiftMask) s |= FL_SHIFT;
    if (xstate & LockMask) s |= FL_CAPS_LOCK;
    if (xstate & ControlMask) s |= FL_CTRL;
    if (xstate & Mod1Mask) s |= FL_ALT;
    if (xstate & Button1Mask) s |= FL_BUTTON1;
    if (xstate & Button2Mask) s |= FL_BUTTON2;
    if (xstate & Button3Mask) s |= FL_BUTTON3;
    return s;
}

static int dispatch_one(XEvent *ev) {
    Fl_Window *win;

    switch (ev->type) {
        case Expose: {
            win = find_window(ev->xexpose.window);
            if (!win) return 0;
            Fl_Widget_redraw(FL_WIDGET(win));
            return 1;
        }

        case ConfigureNotify: {
            Fl_Widget *w;
            win = find_window(ev->xconfigure.window);
            if (!win) return 0;
            w = FL_WIDGET(win);
            if (w->x != ev->xconfigure.x || w->y != ev->xconfigure.y ||
                w->w != ev->xconfigure.width || w->h != ev->xconfigure.height) {
                Fl_Group_resize(w, ev->xconfigure.x, ev->xconfigure.y,
                                 ev->xconfigure.width, ev->xconfigure.height);
                Fl_Widget_redraw(w);
            }
            return 1;
        }

        case ButtonPress: {
            int dx, dy;
            win = find_window(ev->xbutton.window);
            if (!win) return 0;

            dx = ev->xbutton.x_root - g_last_click_x;
            dy = ev->xbutton.y_root - g_last_click_y;
            if (dx < 0) dx = -dx;
            if (dy < 0) dy = -dy;

            if ((int)ev->xbutton.button == g_last_click_button && dx + dy <= 3 &&
                (Time)(ev->xbutton.time - g_last_click_time) < 1000)
                g_click_count++;
            else
                g_click_count = 0;

            g_last_click_button = (int)ev->xbutton.button;
            g_last_click_x = ev->xbutton.x_root;
            g_last_click_y = ev->xbutton.y_root;
            g_last_click_time = ev->xbutton.time;

            fl_backend_set_event_state(ev->xbutton.x, ev->xbutton.y, ev->xbutton.x_root, ev->xbutton.y_root,
                                        0, 0, (int)ev->xbutton.button, g_click_count, 1,
                                        translate_button_state(ev->xbutton.state), 0, NULL, 0);
            Fl_context_handle(FL_PUSH, win);
            return 1;
        }

        case ButtonRelease: {
            win = find_window(ev->xbutton.window);
            if (!win) return 0;
            fl_backend_set_event_state(ev->xbutton.x, ev->xbutton.y, ev->xbutton.x_root, ev->xbutton.y_root,
                                        0, 0, (int)ev->xbutton.button, g_click_count, 1,
                                        translate_button_state(ev->xbutton.state), 0, NULL, 0);
            Fl_context_handle(FL_RELEASE, win);
            return 1;
        }

        case MotionNotify: {
            win = find_window(ev->xmotion.window);
            if (!win) return 0;
            fl_backend_set_event_state(ev->xmotion.x, ev->xmotion.y, ev->xmotion.x_root, ev->xmotion.y_root,
                                        0, 0, 0, 0, 0,
                                        translate_button_state(ev->xmotion.state), 0, NULL, 0);
            Fl_context_handle(FL_MOVE, win);
            return 1;
        }

        case EnterNotify:
        case LeaveNotify: {
            win = find_window(ev->xcrossing.window);
            if (!win) return 0;
            fl_backend_set_event_state(ev->xcrossing.x, ev->xcrossing.y, ev->xcrossing.x_root, ev->xcrossing.y_root,
                                        0, 0, 0, 0, 0,
                                        translate_button_state(ev->xcrossing.state), 0, NULL, 0);
            Fl_context_handle(ev->type == EnterNotify ? FL_ENTER : FL_LEAVE, win);
            return 1;
        }

        case KeyPress:
        case KeyRelease: {
            char text[32];
            KeySym keysym = 0;
            int len;
            win = find_window(ev->xkey.window);
            if (!win) return 0;
            len = XLookupString(&ev->xkey, text, (int)sizeof(text) - 1, &keysym, NULL);
            text[len < 0 ? 0 : len] = '\0';
            fl_backend_set_event_state(ev->xkey.x, ev->xkey.y, ev->xkey.x_root, ev->xkey.y_root,
                                        0, 0, 0, 0, 0,
                                        translate_button_state(ev->xkey.state), (int)keysym, text, len);
            Fl_context_handle(ev->type == KeyPress ? FL_KEYDOWN : FL_KEYUP, win);
            return 1;
        }

        case FocusIn:
        case FocusOut: {
            win = find_window(ev->xfocus.window);
            if (!win) return 0;
            Fl_context_handle(ev->type == FocusIn ? FL_FOCUS : FL_UNFOCUS, win);
            return 1;
        }

        case ClientMessage: {
            win = find_window(ev->xclient.window);
            if (!win) return 0;
            if ((Atom)ev->xclient.data.l[0] == fl_x11_wm_delete_window) {
                Fl_context_handle(FL_CLOSE, win);
            }
            return 1;
        }

        default:
            return 0;
    }
}

int fl_backend_ready(void) {
    return fl_x11_display && XPending(fl_x11_display) > 0;
}

/* -------------------------------------------------------------------
 * fd watching (Fl_add_fd()/Fl_remove_fd(), see Fl.h)
 *
 * Upstream FLTK's own Unix/X11 backend does exactly this: select()
 * over the X connection fd *and* every registered fd in the same
 * call, so a ready socket wakes the event loop the same way an X
 * event does - no separate polling loop needed. Previously cfltk had
 * no such mechanism at all (its own Fl.h banner: "Timers, add_fd(),
 * clipboard, ... intentionally minimal or absent in this phase"), so
 * a downstream embedder (a browser, entirely non-blocking-socket-
 * driven) had to build a complete, self-contained replacement outside
 * cfltk (its own select()-based registry, manually interleaved with
 * Fl_wait_for()). Ported that same design back in here, including a
 * real bug it found and fixed the hard way: dispatching must snapshot
 * every ready {fd, callback, data} triple *before* invoking any
 * callback, since a callback commonly calls Fl_remove_fd() on itself
 * or another fd (e.g. tearing down a window closes every socket it
 * had open) - iterating the live registry while a callback shrinks it
 * out from under the loop is a real, previously-hit SIGSEGV (stale
 * index / wrong entry / freed memory), not a hypothetical concern. */
#define FL_FD_MAX 64
typedef struct { int fd; int when; Fl_FD_Handler *cb; void *data; int active; } Fl_FD_Slot;
static Fl_FD_Slot g_fds[FL_FD_MAX];
static int g_fd_count = 0; /* number of active slots */

void Fl_add_fd(int fd, int when, Fl_FD_Handler *cb, void *data) {
    int i;
    if (fd < 0) return;
    for (i = 0; i < FL_FD_MAX; i++) {
        if (!g_fds[i].active) {
            g_fds[i].active = 1;
            g_fds[i].fd = fd;
            g_fds[i].when = when;
            g_fds[i].cb = cb;
            g_fds[i].data = data;
            g_fd_count++;
            return;
        }
    }
    /* Pool exhausted: silently dropped, same policy as Fl_add_timeout(). */
}

void Fl_remove_fd(int fd, int when) {
    int i;
    if (fd < 0) return;
    for (i = 0; i < FL_FD_MAX; i++) {
        if (g_fds[i].active && g_fds[i].fd == fd) {
            g_fds[i].when &= ~when;
            if (g_fds[i].when == 0) {
                g_fds[i].active = 0;
                g_fd_count--;
            }
        }
    }
}

int fl_backend_wait(double timeout_secs) {
    int dispatched = 0;

    if (!fl_x11_display) return 0;

    while (XPending(fl_x11_display) > 0) {
        XEvent ev;
        XNextEvent(fl_x11_display, &ev);
        if (dispatch_one(&ev)) dispatched = 1;
    }
    if (dispatched) return 1;

    {
        int xfd = ConnectionNumber(fl_x11_display);
        fd_set rfds, wfds, efds;
        struct timeval tv, *tvp = NULL;
        int maxfd = xfd, i, n;

        FD_ZERO(&rfds);
        FD_ZERO(&wfds);
        FD_ZERO(&efds);
        FD_SET(xfd, &rfds);

        for (i = 0; i < FL_FD_MAX; i++) {
            if (!g_fds[i].active) continue;
            if (g_fds[i].when & FL_READ) FD_SET(g_fds[i].fd, &rfds);
            if (g_fds[i].when & FL_WRITE) FD_SET(g_fds[i].fd, &wfds);
            if (g_fds[i].when & FL_EXCEPT) FD_SET(g_fds[i].fd, &efds);
            if (g_fds[i].fd > maxfd) maxfd = g_fds[i].fd;
        }

        if (timeout_secs < 1e10) {
            if (timeout_secs < 0) timeout_secs = 0;
            tv.tv_sec = (long)timeout_secs;
            tv.tv_usec = (long)((timeout_secs - (double)tv.tv_sec) * 1e6);
            tvp = &tv;
        }

        n = select(maxfd + 1, &rfds, &wfds, &efds, tvp);
        if (n <= 0) return 0;

        if (g_fd_count > 0) {
            /* Snapshot before dispatch - see this section's own banner. */
            struct { int fd; Fl_FD_Handler *cb; void *data; } ready[FL_FD_MAX];
            int nready = 0;
            for (i = 0; i < FL_FD_MAX; i++) {
                int when;
                if (!g_fds[i].active) continue;
                when = 0;
                if (FD_ISSET(g_fds[i].fd, &rfds)) when |= FL_READ;
                if (FD_ISSET(g_fds[i].fd, &wfds)) when |= FL_WRITE;
                if (FD_ISSET(g_fds[i].fd, &efds)) when |= FL_EXCEPT;
                if (when != 0 && (g_fds[i].when & when)) {
                    ready[nready].fd = g_fds[i].fd;
                    ready[nready].cb = g_fds[i].cb;
                    ready[nready].data = g_fds[i].data;
                    nready++;
                }
            }
            for (i = 0; i < nready; i++) {
                dispatched = 1;
                ready[i].cb(ready[i].fd, ready[i].data);
            }
        }
    }

    while (XPending(fl_x11_display) > 0) {
        XEvent ev;
        XNextEvent(fl_x11_display, &ev);
        if (dispatch_one(&ev)) dispatched = 1;
    }
    return dispatched;
}
