/*
 * cfltk - fl_x11_dnd.c
 *
 * Xdnd protocol (https://freedesktop.org/wiki/Specifications/XDND/),
 * version 5. Backs two directions:
 *
 *  - Target: any cfltk window can have something dropped onto it from
 *    another Xdnd-aware application. The dropped data always arrives
 *    as text (matches upstream FLTK's own scope - Xdnd itself is
 *    type-negotiated, but this only ever requests text/uri-list or a
 *    plain string type, never arbitrary binary payloads), delivered to
 *    whichever widget is under the drop point via Fl_paste() - the
 *    exact same delivery path Fl_paste()/clipboard paste already uses,
 *    reusing clipboard buffer 0 as the transfer buffer.
 *  - Source: Fl_dnd() drags whatever's currently in clipboard buffer 0
 *    (the same buffer Fl_copy(text,len,0) fills - matches upstream's
 *    Fl::dnd(), which has no separate "set drag payload" call either).
 *
 * Both directions center on the same three ideas: a handful of atoms
 * naming the protocol's ClientMessage types, the existing X SELECTION
 * mechanism (XSetSelectionOwner/XConvertSelection/SelectionRequest/
 * SelectionNotify) reused with the XdndSelection atom instead of
 * CLIPBOARD/PRIMARY, and ClientMessages carrying drag-position/status/
 * drop/finished notifications between source and target.
 */
#ifndef _POSIX_C_SOURCE
/* clock_gettime()/CLOCK_MONOTONIC under strict -std=c99 - same
 * rationale as Fl.c's identical guard. */
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _DEFAULT_SOURCE
/* usleep() - POSIX.1-2008 dropped it from the set _POSIX_C_SOURCE
 * alone exposes; glibc still provides it under _DEFAULT_SOURCE. */
#define _DEFAULT_SOURCE
#endif

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <X11/Xatom.h> /* XA_ATOM, XA_STRING */

#include "fl_x11_internal.h"
#include "../fl_backend.h"
#include "cfltk/Fl.h"

static Atom XA_XdndAware, XA_XdndEnter, XA_XdndPosition, XA_XdndStatus,
            XA_XdndLeave, XA_XdndDrop, XA_XdndFinished, XA_XdndSelection,
            XA_XdndActionCopy, XA_XdndTypeList,
            XA_text_uri_list, XA_UTF8_STRING;
static int g_atoms_ready = 0;

static void ensure_atoms(void) {
    if (g_atoms_ready) return;
    XA_XdndAware      = XInternAtom(fl_x11_display, "XdndAware", False);
    XA_XdndEnter      = XInternAtom(fl_x11_display, "XdndEnter", False);
    XA_XdndPosition   = XInternAtom(fl_x11_display, "XdndPosition", False);
    XA_XdndStatus     = XInternAtom(fl_x11_display, "XdndStatus", False);
    XA_XdndLeave      = XInternAtom(fl_x11_display, "XdndLeave", False);
    XA_XdndDrop       = XInternAtom(fl_x11_display, "XdndDrop", False);
    XA_XdndFinished   = XInternAtom(fl_x11_display, "XdndFinished", False);
    XA_XdndSelection  = XInternAtom(fl_x11_display, "XdndSelection", False);
    XA_XdndActionCopy = XInternAtom(fl_x11_display, "XdndActionCopy", False);
    XA_XdndTypeList   = XInternAtom(fl_x11_display, "XdndTypeList", False);
    XA_text_uri_list  = XInternAtom(fl_x11_display, "text/uri-list", False);
    XA_UTF8_STRING    = XInternAtom(fl_x11_display, "UTF8_STRING", False);
    g_atoms_ready = 1;
}

/* Swallows BadWindow/BadMatch/etc from any X call touching a window
 * that vanished mid-drag (the other application closed it, a WM
 * reparented it, ...) - same rationale/precedent as
 * fl_x11_driver.c's d_read_image() error handler: upstream's own
 * fl_read_image() wraps exactly this kind of racy cross-client X call
 * the same way, "make sure we catch the error and continue" rather
 * than letting the default handler exit() the whole process over a
 * drag-and-drop hiccup. */
static int dnd_err_handler(Display *d, XErrorEvent *e) { (void)d; (void)e; return 0; }

/* ------------------------------------------------------------------ */
/* Target side                                                         */
/* ------------------------------------------------------------------ */

void fl_x11_dnd_window_created(Window xid) {
    long version = 5;
    ensure_atoms();
    XChangeProperty(fl_x11_display, xid, XA_XdndAware, XA_ATOM, 32,
                     PropModeReplace, (unsigned char *)&version, 1);
}

/* Which window (if any) our own top-level is currently tracking as the
 * source of an incoming drag, and which of our windows the drag is
 * currently hovering over - both None/NULL when no drag is in
 * progress. Xdnd is inherently single-focus (one pointer, one active
 * drag at a time), so process-wide state is sufficient. */
static Window g_dnd_source = None;
static Fl_Window *g_dnd_target_win = NULL;
static Atom g_dnd_types[3];
static int g_dnd_ntypes = 0;

static int type_offered(Atom a) {
    int i;
    if (a == None) return 0;
    for (i = 0; i < g_dnd_ntypes; i++) if (g_dnd_types[i] == a) return 1;
    return 0;
}

static void send_client_message(Window dest, Atom type, Window data0, long d1, long d2, long d3, long d4) {
    XEvent xev;
    memset(&xev, 0, sizeof(xev));
    xev.xclient.type = ClientMessage;
    xev.xclient.window = dest;
    xev.xclient.message_type = type;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = (long)data0;
    xev.xclient.data.l[1] = d1;
    xev.xclient.data.l[2] = d2;
    xev.xclient.data.l[3] = d3;
    xev.xclient.data.l[4] = d4;
    XSendEvent(fl_x11_display, dest, False, NoEventMask, &xev);
}

int fl_x11_dnd_handle_client_message(XEvent *ev) {
    Atom type = ev->xclient.message_type;
    Fl_Window *win;
    XErrorHandler old;

    ensure_atoms();
    if (type != XA_XdndEnter && type != XA_XdndPosition &&
        type != XA_XdndLeave && type != XA_XdndDrop)
        return 0;

    old = XSetErrorHandler(dnd_err_handler);

    if (type == XA_XdndEnter) {
        g_dnd_source = (Window)ev->xclient.data.l[0];
        if (ev->xclient.data.l[1] & 1) {
            /* More than 3 types offered: the real list lives in the
             * source's own XdndTypeList property instead of this
             * message's data.l[2..4]. */
            Atom actual_type; int actual_format; unsigned long nitems, after;
            unsigned char *prop = NULL;
            g_dnd_ntypes = 0;
            if (XGetWindowProperty(fl_x11_display, g_dnd_source, XA_XdndTypeList, 0, 3, False,
                                    XA_ATOM, &actual_type, &actual_format, &nitems, &after, &prop) == Success
                && prop) {
                unsigned long i;
                Atom *atoms = (Atom *)prop;
                for (i = 0; i < nitems && i < 3; i++) g_dnd_types[g_dnd_ntypes++] = atoms[i];
                XFree(prop);
            }
        } else {
            g_dnd_ntypes = 0;
            if (ev->xclient.data.l[2]) g_dnd_types[g_dnd_ntypes++] = (Atom)ev->xclient.data.l[2];
            if (ev->xclient.data.l[3]) g_dnd_types[g_dnd_ntypes++] = (Atom)ev->xclient.data.l[3];
            if (ev->xclient.data.l[4]) g_dnd_types[g_dnd_ntypes++] = (Atom)ev->xclient.data.l[4];
        }
        XSetErrorHandler(old);
        return 1;
    }

    win = find_window(ev->xclient.window);
    if (!win) { XSetErrorHandler(old); return 1; }

    if (type == XA_XdndPosition) {
        Fl_X11_Window *xw = fl_x11_window_data(win);
        int root_x = (int)((unsigned long)ev->xclient.data.l[2] >> 16);
        int root_y = (int)((unsigned long)ev->xclient.data.l[2] & 0xffff);
        int local_x = 0, local_y = 0;
        Window child;
        int accept;

        if (xw) XTranslateCoordinates(fl_x11_display, fl_x11_root, xw->real_xid, root_x, root_y, &local_x, &local_y, &child);

        fl_backend_set_event_state(local_x, local_y, root_x, root_y, 0, 0, 0, 0, 0, 0, 0, NULL, 0);
        if (g_dnd_target_win != win) {
            if (g_dnd_target_win) Fl_context_handle(FL_DND_LEAVE, g_dnd_target_win);
            g_dnd_target_win = win;
            Fl_context_handle(FL_DND_ENTER, win);
        } else {
            Fl_context_handle(FL_DND_DRAG, win);
        }

        accept = type_offered(XA_text_uri_list) || type_offered(XA_UTF8_STRING) || type_offered(XA_STRING);
        send_client_message(g_dnd_source, XA_XdndStatus, ev->xclient.window,
                             accept ? 1 : 0, 0, 0, accept ? (long)XA_XdndActionCopy : (long)None);
        XFlush(fl_x11_display);
        XSetErrorHandler(old);
        return 1;
    }

    if (type == XA_XdndLeave) {
        if (g_dnd_target_win == win) {
            Fl_context_handle(FL_DND_LEAVE, win);
            g_dnd_target_win = NULL;
        }
        g_dnd_source = None;
        g_dnd_ntypes = 0;
        XSetErrorHandler(old);
        return 1;
    }

    if (type == XA_XdndDrop) {
        Fl_X11_Window *xw = fl_x11_window_data(win);
        Time time = (Time)ev->xclient.data.l[2];
        Atom chosen = None;

        if (type_offered(XA_text_uri_list)) chosen = XA_text_uri_list;
        else if (type_offered(XA_UTF8_STRING)) chosen = XA_UTF8_STRING;
        else if (type_offered(XA_STRING)) chosen = XA_STRING;

        if (!xw || chosen == None) {
            send_client_message(g_dnd_source, XA_XdndFinished, ev->xclient.window, 0, 0, 0, 0);
            XFlush(fl_x11_display);
            g_dnd_target_win = NULL;
            XSetErrorHandler(old);
            return 1;
        }

        XConvertSelection(fl_x11_display, XA_XdndSelection, chosen, XA_XdndSelection, xw->real_xid, time);
        XFlush(fl_x11_display);

        /* Bounded synchronous wait for the resulting SelectionNotify -
         * matches upstream FLTK's own target-side drop handling (a
         * local selection round-trip is expected to complete in well
         * under this window; a hung/malicious source just means the
         * drop silently does nothing after the timeout, not a stuck
         * event loop). */
        {
            XEvent sel_ev;
            int got = 0;
            int tries;
            for (tries = 0; tries < 1000 && !got; tries++) {
                if (XCheckTypedWindowEvent(fl_x11_display, xw->real_xid, SelectionNotify, &sel_ev)) { got = 1; break; }
                usleep(2000);
            }
            if (got && sel_ev.xselection.property != None) {
                Atom actual_type; int actual_format; unsigned long nitems, after;
                unsigned char *data = NULL;
                if (XGetWindowProperty(fl_x11_display, xw->real_xid, sel_ev.xselection.property, 0, 1L << 20, True,
                                        AnyPropertyType, &actual_type, &actual_format, &nitems, &after, &data) == Success
                    && data) {
                    Fl_Widget *dest = Fl_belowmouse() ? Fl_belowmouse() : FL_WIDGET(win);
                    fl_backend_set_event_state(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, 0);
                    Fl_context_handle(FL_DND_RELEASE, win);
                    Fl_copy((const char *)data, (int)nitems, 0);
                    Fl_paste(dest, 0);
                    XFree(data);
                }
            }
        }

        send_client_message(g_dnd_source, XA_XdndFinished, ev->xclient.window, 1, (long)XA_XdndActionCopy, 0, 0);
        XFlush(fl_x11_display);
        g_dnd_target_win = NULL;
        g_dnd_source = None;
        g_dnd_ntypes = 0;
        XSetErrorHandler(old);
        return 1;
    }

    XSetErrorHandler(old);
    return 1;
}

/* Only relevant while we're the target of an XConvertSelection reply
 * that fl_x11_dnd_handle_client_message()'s XdndDrop branch already
 * consumes synchronously via XCheckTypedWindowEvent() above - normal
 * dispatch_one() SelectionNotify handling never needs to do anything,
 * this exists purely so dispatch_one() has something to call without
 * special-casing "was this consumed already". */
int fl_x11_dnd_handle_selection_notify(XEvent *ev) {
    (void)ev;
    return 1;
}

/* ------------------------------------------------------------------ */
/* Source side (Fl_dnd())                                              */
/* ------------------------------------------------------------------ */

static char *g_drag_payload = NULL;
static int g_drag_payload_len = 0;

int fl_x11_dnd_handle_selection_request(XEvent *ev) {
    XSelectionRequestEvent *req = &ev->xselectionrequest;
    XEvent reply;
    Atom target = req->target;

    ensure_atoms();
    if (req->selection != XA_XdndSelection) return 0;

    memset(&reply, 0, sizeof(reply));
    reply.xselection.type = SelectionNotify;
    reply.xselection.requestor = req->requestor;
    reply.xselection.selection = req->selection;
    reply.xselection.target = req->target;
    reply.xselection.time = req->time;
    reply.xselection.property = None;

    if (g_drag_payload && (target == XA_UTF8_STRING || target == XA_STRING || target == XA_text_uri_list)) {
        XChangeProperty(fl_x11_display, req->requestor, req->property, target, 8, PropModeReplace,
                         (unsigned char *)g_drag_payload, g_drag_payload_len);
        reply.xselection.property = req->property;
    }

    XSendEvent(fl_x11_display, req->requestor, False, NoEventMask, &reply);
    XFlush(fl_x11_display);
    return 1;
}

/* Finds the Xdnd-aware window (if any) under screen point
 * (root_x,root_y) - descends up to a few levels from the root (covers
 * both a bare/WM-less window, a direct child of root, and one level of
 * window-manager reparenting into a frame window) checking XdndAware
 * at each level, matching how other toolkits' Xdnd source-side pointer
 * tracking finds the real target rather than a WM frame around it. */
static Window find_xdnd_target_at(int root_x, int root_y) {
    Window w = fl_x11_root;
    int depth;
    for (depth = 0; depth < 4; depth++) {
        Window child = None;
        int wx, wy;
        Atom actual_type; int actual_format; unsigned long nitems, after;
        unsigned char *data = NULL;

        if (!XTranslateCoordinates(fl_x11_display, fl_x11_root, w, root_x, root_y, &wx, &wy, &child) || child == None)
            return None;

        if (XGetWindowProperty(fl_x11_display, child, XA_XdndAware, 0, 1, False, AnyPropertyType,
                                &actual_type, &actual_format, &nitems, &after, &data) == Success
            && actual_type != None && nitems >= 1) {
            if (data) XFree(data);
            return child;
        }
        if (data) XFree(data);
        w = child;
    }
    return None;
}

int fl_backend_dnd_start(const char *text, int len) {
    Fl_Window *src_win;
    Fl_X11_Window *xw;
    Window drag_win;
    Window current_target = None;
    int dropped = 0;
    XErrorHandler old;
    struct timespec deadline;

    if (!text || len <= 0) return 0;
    src_win = Fl_first_window();
    if (!src_win) return 0;
    xw = fl_x11_window_data(src_win);
    if (!xw) return 0;
    drag_win = xw->real_xid;

    ensure_atoms();

    free(g_drag_payload);
    g_drag_payload = (char *)malloc((size_t)len);
    memcpy(g_drag_payload, text, (size_t)len);
    g_drag_payload_len = len;

    old = XSetErrorHandler(dnd_err_handler);

    XSetSelectionOwner(fl_x11_display, XA_XdndSelection, drag_win, CurrentTime);
    if (XGetSelectionOwner(fl_x11_display, XA_XdndSelection) != drag_win) {
        XSetErrorHandler(old);
        return 0;
    }

    if (XGrabPointer(fl_x11_display, drag_win, False, ButtonReleaseMask | PointerMotionMask,
                      GrabModeAsync, GrabModeAsync, None, None, CurrentTime) != GrabSuccess) {
        XSetErrorHandler(old);
        return 0;
    }

    /* Bounded overall drag duration (2 minutes) so a caller can never
     * hang forever with the pointer grabbed if something goes wrong
     * (e.g. the button-release event gets lost) - the pointer grab
     * would otherwise freeze input to the whole display. */
    clock_gettime(CLOCK_MONOTONIC, &deadline);
    deadline.tv_sec += 120;

    for (;;) {
        XEvent ev;
        struct timespec now;

        if (!XPending(fl_x11_display)) {
            clock_gettime(CLOCK_MONOTONIC, &now);
            if (now.tv_sec > deadline.tv_sec ||
                (now.tv_sec == deadline.tv_sec && now.tv_nsec > deadline.tv_nsec))
                break;
            usleep(2000);
            continue;
        }
        XNextEvent(fl_x11_display, &ev);

        if (ev.type == MotionNotify) {
            Window target = find_xdnd_target_at(ev.xmotion.x_root, ev.xmotion.y_root);
            if (target != current_target) {
                if (current_target != None)
                    send_client_message(current_target, XA_XdndLeave, drag_win, 0, 0, 0, 0);
                current_target = target;
                if (current_target != None)
                    send_client_message(current_target, XA_XdndEnter, drag_win,
                                         (5L << 24), (long)XA_UTF8_STRING, (long)XA_text_uri_list, 0);
            }
            if (current_target != None)
                send_client_message(current_target, XA_XdndPosition, drag_win, 0,
                                     ((long)ev.xmotion.x_root << 16) | (ev.xmotion.y_root & 0xffff),
                                     CurrentTime, (long)XA_XdndActionCopy);
            XFlush(fl_x11_display);
        } else if (ev.type == ButtonRelease) {
            if (current_target != None) {
                send_client_message(current_target, XA_XdndDrop, drag_win, 0, CurrentTime, 0, 0);
                XFlush(fl_x11_display);
                dropped = 1;
                /* Don't stop yet: the target won't request the actual
                 * payload (SelectionRequest, serviced below) until it
                 * has processed this XdndDrop, which happens
                 * asynchronously shortly after. Keep servicing events -
                 * SelectionRequest in particular - for a bounded extra
                 * window so the target can actually complete the
                 * transfer before we ungrab and return. */
                {
                    struct timespec drop_deadline;
                    clock_gettime(CLOCK_MONOTONIC, &drop_deadline);
                    drop_deadline.tv_sec += 2;
                    for (;;) {
                        XEvent ev2;
                        struct timespec now2;
                        if (!XPending(fl_x11_display)) {
                            clock_gettime(CLOCK_MONOTONIC, &now2);
                            if (now2.tv_sec > drop_deadline.tv_sec ||
                                (now2.tv_sec == drop_deadline.tv_sec && now2.tv_nsec > drop_deadline.tv_nsec))
                                break;
                            usleep(2000);
                            continue;
                        }
                        XNextEvent(fl_x11_display, &ev2);
                        if (ev2.type == SelectionRequest) {
                            fl_x11_dnd_handle_selection_request(&ev2);
                        } else if (ev2.type == ClientMessage && ev2.xclient.message_type == XA_XdndFinished) {
                            break;
                        }
                    }
                }
            }
            break;
        } else if (ev.type == SelectionRequest) {
            fl_x11_dnd_handle_selection_request(&ev);
        }
        /* XdndStatus replies from the target are received here too but
         * not required for correctness of this simplified source (no
         * drag-cursor visual feedback to update); ignored. */
    }

    XUngrabPointer(fl_x11_display, CurrentTime);
    XFlush(fl_x11_display);
    XSetErrorHandler(old);
    return dropped;
}
