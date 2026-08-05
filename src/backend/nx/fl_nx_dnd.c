/*
 * cfltk - fl_nx_dnd.c
 *
 * Drag-and-drop, backing fl_backend_dnd_start() (../fl_backend.h) and
 * the FL_DND_ENTER/FL_DND_DRAG/FL_DND_LEAVE/FL_DND_RELEASE events
 * dispatched on drop.
 *
 * Unlike fl_x11_dnd.c's Xdnd (a real cross-process protocol between
 * whatever X clients happen to be on the desktop), mwin owns the
 * entire screen as a single process -- there is no window manager and
 * no other application to negotiate a drop with. So this is
 * necessarily intra-application only: dragging between two of this
 * same cfltk app's own top-level windows (or within one window, same
 * as any other widget-to-widget drop). That is still real,
 * functional drag-and-drop for a single-process embedded target, just
 * not an inter-app one -- there is nothing on this platform for an
 * inter-app protocol to talk to.
 *
 * fl_backend_dnd_start() runs its own short modal loop (mouse already
 * grabbed via SetCapture(), matching fl_backend_grab()'s pattern):
 * polls the cursor position every pass with GetCursorPos() +
 * WindowFromPoint() (a top-level-window hit test, exactly what's
 * needed here) to find which of this app's own Fl_Window's the
 * pointer is currently over, dispatching FL_DND_ENTER/FL_DND_DRAG/
 * FL_DND_LEAVE as that target changes -- and watches for WM_LBUTTONUP
 * as the drop signal, at which point the target receives
 * FL_DND_RELEASE and the payload is delivered through the exact same
 * Fl_copy()/Fl_paste() path an ordinary clipboard paste already uses.
 * Every other message type arriving during the drag is still
 * dispatched normally (so background repaints/timers/fd-watches keep
 * working); WM_MOUSEMOVE is swallowed by this loop instead, since
 * GetCursorPos() polling already covers tracking and dispatching it
 * normally would just deliver stale under-the-grab coordinates to
 * whichever window happens to hold capture.
 *
 * Tried and reverted: making the drag source widget itself follow the
 * cursor in real time by moving Fl_pushed() every iteration. Fl_pushed()
 * is not reliably the specific widget clicked at the point this
 * function runs -- Fl_Group_handle()'s FL_PUSH case only narrows it
 * from the enclosing window down to the actual child *after* that
 * child's own handle() (this function's caller) already returns, so
 * reading it here moved/resized the top-level window itself instead of
 * the button, reentering mwin's own window-move/resize path from
 * inside this already-nested loop and hanging the whole UI. Real live
 * "ghost" feedback needs the caller to hand this function a specific
 * widget explicitly (or a position-callback), not a generic global
 * lookup -- worth a real follow-up, not a guess made under retest
 * pressure.
 */
#ifndef _POSIX_C_SOURCE
/* clock_gettime()/CLOCK_MONOTONIC under strict -std=c99 -- same
 * rationale as Fl.c's identical guard. */
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _DEFAULT_SOURCE
/* usleep() -- POSIX.1-2008 dropped it from the set _POSIX_C_SOURCE
 * alone exposes; glibc still provides it under _DEFAULT_SOURCE. */
#define _DEFAULT_SOURCE
#endif

#include <nuttx/config.h> /* must be first -- see fl_nx_window.c's comment */
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "fl_nx_internal.h"
#include "../fl_backend.h"
#include "cfltk/Fl.h"

static char *g_dnd_payload = NULL;
static int g_dnd_payload_len = 0;

/* Guards against re-entering the modal loop below. Real, not
 * hypothetical: this loop dispatches every non-mouse-move message it
 * sees (so background repaints/timers/fd-watches keep working during
 * a drag), and that includes a *second* WM_LBUTTONDOWN if the pointer
 * lands back on a drag-source widget while a previous drag is still
 * winding down (its own WM_LBUTTONUP not drained yet) -- which calls
 * straight back into fl_backend_dnd_start() from inside this very
 * loop. Each nested call would SetCapture() over the last, and none
 * would ever see the WM_LBUTTONUP meant for a different nesting level
 * as anything but "not mine yet" -- observed directly as the whole UI
 * going unresponsive for minutes after a few quick clicks (each
 * nested loop only gives up after its own 2-minute deadline). */
static int g_dnd_in_progress = 0;

int fl_backend_dnd_start(const char *text, int len) {
    Fl_Window *src_win;
    Fl_NX_Window *nw;
    Fl_Window *current_target = NULL;
    int dropped = 0;
    struct timespec deadline, now;

    if (!text || len <= 0) return 0;
    if (g_dnd_in_progress) return 0;
    src_win = Fl_first_window();
    if (!src_win) return 0;
    nw = fl_nx_window_data(src_win);
    if (!nw) return 0;

    g_dnd_in_progress = 1;

    free(g_dnd_payload);
    g_dnd_payload = (char *)malloc((size_t)len);
    if (!g_dnd_payload) return 0;
    memcpy(g_dnd_payload, text, (size_t)len);
    g_dnd_payload_len = len;

    SetCapture(nw->hwnd);

    /* Bounded overall drag duration (2 minutes), same as the X11
     * backend's own guard: a lost/never-arriving button-release can
     * never freeze this modal loop forever. */
    clock_gettime(CLOCK_MONOTONIC, &deadline);
    deadline.tv_sec += 120;

    for (;;) {
        MSG msg;
        POINT pt;

        clock_gettime(CLOCK_MONOTONIC, &now);
        if (now.tv_sec > deadline.tv_sec ||
            (now.tv_sec == deadline.tv_sec && now.tv_nsec > deadline.tv_nsec))
            break;

        if (!PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            usleep(2000);
        } else if (msg.message == WM_LBUTTONUP) {
            if (current_target) {
                Fl_Widget *dest = Fl_belowmouse() ? Fl_belowmouse() : FL_WIDGET(current_target);
                fl_backend_set_event_state(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, 0);
                Fl_context_handle(FL_DND_RELEASE, current_target);
                Fl_copy(g_dnd_payload, g_dnd_payload_len, 0);
                Fl_paste(dest, 0);
                dropped = 1;
            }
            break;
        } else if (msg.message != WM_MOUSEMOVE) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (GetCursorPos(&pt)) {
            HWND under = WindowFromPoint(pt);
            Fl_Window *target = under ? fl_nx_find_window(under) : NULL;

            if (target != current_target) {
                if (current_target) Fl_context_handle(FL_DND_LEAVE, current_target);
                current_target = target;
                if (current_target) {
                    POINT local = pt;
                    ScreenToClient(under, &local);
                    fl_backend_set_event_state(local.x, local.y, pt.x, pt.y,
                                                0, 0, 0, 0, 0, 0, 0, NULL, 0);
                    Fl_context_handle(FL_DND_ENTER, current_target);
                }
            }
            if (current_target) {
                POINT local = pt;
                ScreenToClient(under, &local);
                fl_backend_set_event_state(local.x, local.y, pt.x, pt.y,
                                            0, 0, 0, 0, 0, 0, 0, NULL, 0);
                Fl_context_handle(FL_DND_DRAG, current_target);
            }
        }
    }

    ReleaseCapture();
    g_dnd_in_progress = 0;
    return dropped;
}
