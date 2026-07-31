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
        if (xw && xw->xid == xid) return w;
    }
    return NULL;
}

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

        case ButtonPress:
        case ButtonRelease: {
            win = find_window(ev->xbutton.window);
            if (!win) return 0;
            fl_backend_set_event_state(ev->xbutton.x, ev->xbutton.y, ev->xbutton.x_root, ev->xbutton.y_root,
                                        0, 0, (int)ev->xbutton.button, 1, 1,
                                        translate_button_state(ev->xbutton.state), 0, NULL, 0);
            Fl_context_handle(ev->type == ButtonPress ? FL_PUSH : FL_RELEASE, win);
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
        int fd = ConnectionNumber(fl_x11_display);
        fd_set set;
        struct timeval tv, *tvp = NULL;

        FD_ZERO(&set);
        FD_SET(fd, &set);
        if (timeout_secs < 1e10) {
            if (timeout_secs < 0) timeout_secs = 0;
            tv.tv_sec = (long)timeout_secs;
            tv.tv_usec = (long)((timeout_secs - (double)tv.tv_sec) * 1e6);
            tvp = &tv;
        }
        if (select(fd + 1, &set, NULL, NULL, tvp) <= 0) return 0;
    }

    while (XPending(fl_x11_display) > 0) {
        XEvent ev;
        XNextEvent(fl_x11_display, &ev);
        if (dispatch_one(&ev)) dispatched = 1;
    }
    return dispatched;
}
