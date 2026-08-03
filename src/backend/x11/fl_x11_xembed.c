/*
 * cfltk - fl_x11_xembed.c
 *
 * The embedded-client half of the XEmbed protocol
 * (https://specifications.freedesktop.org/xembed-spec/xembed-spec-latest.html):
 * lets an Fl_Window reparent itself into a window belonging to another,
 * potentially unrelated X client (the "embedder") instead of becoming
 * an ordinary WM-managed top-level, and translates the embedder's
 * `_XEMBED` ClientMessages into the same FL_FOCUS/FL_UNFOCUS events any
 * other focus-change source already produces. Backs
 * Fl_Window_set_embed_xid() (see include/cfltk/Fl_Window.h).
 */
#include <string.h>

#include "fl_x11_internal.h"
#include "../fl_backend.h"
#include "cfltk/Fl.h"

/* XEmbed opcodes (data.l[1] of an `_XEMBED` ClientMessage) this file
 * cares about - the spec defines more (WINDOW_ACTIVATE/DEACTIVATE,
 * REQUEST_FOCUS, FOCUS_NEXT/PREV, MODALITY_ON/OFF, ...), but upstream
 * FLTK's own Xembed handling is likewise focused on the two that
 * actually matter for a keyboard-interactive embedded widget: whether
 * it currently has keyboard focus at all. */
#define XEMBED_FOCUS_IN  4
#define XEMBED_FOCUS_OUT 5

/* _XEMBED_INFO flags (freedesktop spec). */
#define XEMBED_MAPPED (1 << 0)

static Atom XA_XEMBED, XA_XEMBED_INFO;
static int g_atoms_ready = 0;

static void ensure_atoms(void) {
    if (g_atoms_ready) return;
    XA_XEMBED      = XInternAtom(fl_x11_display, "_XEMBED", False);
    XA_XEMBED_INFO = XInternAtom(fl_x11_display, "_XEMBED_INFO", False);
    g_atoms_ready = 1;
}

/* Swallows BadWindow/BadMatch from the embedder's window vanishing
 * mid-reparent (same "catch and continue rather than let the default
 * handler exit() the process" precedent as fl_x11_driver.c's
 * d_read_image() and fl_x11_dnd.c's own error handler). */
static int xembed_err_handler(Display *d, XErrorEvent *e) { (void)d; (void)e; return 0; }

void fl_x11_xembed_window_created(Fl_Window *win, Window xid) {
    long info[2];
    XErrorHandler old;

    if (!win->embed_xid) return;
    ensure_atoms();

    /* Advertise _XEMBED_INFO (version 0, XEMBED_MAPPED set) before the
     * window is ever mapped, per spec - the embedder is expected to
     * read this once it sees the reparent to decide whether to make
     * the embedded window visible; setting XEMBED_MAPPED here matches
     * this port's own behavior of always mapping right after show()
     * regardless (fl_backend_window_show()'s unconditional
     * XMapRaised()), so there's no separate "start unmapped, wait to
     * be told to map" state to track. */
    info[0] = 0;
    info[1] = XEMBED_MAPPED;
    XChangeProperty(fl_x11_display, xid, XA_XEMBED_INFO, XA_XEMBED_INFO, 32,
                     PropModeReplace, (unsigned char *)info, 2);

    /* No XAddToSaveSet() here: that call only works on a window
     * created by *another* client (BadMatch otherwise - confirmed by
     * trying it: the X server rejects adding our own, self-created
     * window to our own save set). Protecting against the embedder
     * disappearing unexpectedly (crash, exiting without reparenting
     * children back out first) is the *embedder's* responsibility per
     * the XEmbed spec - it is the one reparenting a foreign window
     * into its own hierarchy, so it is the one positioned to save-set
     * it. A well-behaved embedder does this; ours can't do it on the
     * embedder's behalf. What we *can* and do control is not crashing
     * if it doesn't: see fl_backend_window_flush()'s temporary error
     * handler (fl_x11_window.c) for the other half of this - confirmed
     * by reproducing exactly this with a test "embedder" that
     * reparents a cfltk window in and then exits without unmapping/
     * reparenting it back out, which used to take the whole embedded
     * process down with it via an unguarded XCopyArea BadDrawable. */
    old = XSetErrorHandler(xembed_err_handler);
    XReparentWindow(fl_x11_display, xid, (Window)win->embed_xid, 0, 0);
    XSetErrorHandler(old);
}

int fl_x11_xembed_handle_client_message(XEvent *ev) {
    Fl_Window *win;
    long opcode;

    ensure_atoms();
    if (ev->xclient.message_type != XA_XEMBED) return 0;

    win = find_window(ev->xclient.window);
    if (!win || !win->embed_xid) return 1; /* not one of our embedded windows; consumed anyway (it IS an _XEMBED message) */

    opcode = ev->xclient.data.l[1];
    if (opcode == XEMBED_FOCUS_IN) {
        fl_backend_set_event_state(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, 0);
        Fl_context_handle(FL_FOCUS, win);
    } else if (opcode == XEMBED_FOCUS_OUT) {
        fl_backend_set_event_state(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, 0);
        Fl_context_handle(FL_UNFOCUS, win);
    }
    /* Other opcodes (WINDOW_ACTIVATE/DEACTIVATE, REQUEST_FOCUS,
     * FOCUS_NEXT/PREV, MODALITY_ON/OFF) are accepted (the message is
     * still consumed - the embedder gets no error/is not left waiting
     * on anything) but not acted on, matching the scope note above. */
    return 1;
}
