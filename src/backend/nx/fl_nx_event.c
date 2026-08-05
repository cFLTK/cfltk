/*
 * cfltk - fl_nx_event.c
 *
 * Translates mwin WM_* messages into Fl_* events and feeds them to
 * Fl_context_handle(), plus the fl_backend_wait()/fl_backend_ready()
 * pump. Modeled in spirit on fl_x11_event.c's dispatch_one(), adapted
 * for mwin's callback-driven message loop: fl_nx_wndproc() is
 * registered once (fl_nx_window.c's fl_backend_init()) as every
 * cfltk window's WndProc, and does the same per-event translation
 * fl_x11_event.c's dispatch_one() does per-XEvent, just invoked by
 * DispatchMessage() instead of pulled out of an XNextEvent() loop.
 *
 * Mouse buttons/motion, window close/paint/resize, and keyboard input
 * are all translated. Keyboard: printable characters dispatch off
 * WM_CHAR (already shift/caps-translated by TranslateMessage(), so
 * its wParam is the actual typed character -- matches cfltk's FL_KEY
 * encoding directly, which is ASCII for the printable range, same as
 * upstream FLTK's own X11-keysym-derived scheme); navigation/control
 * keys that never produce a WM_CHAR (arrows, Home/End, Delete/Insert,
 * PageUp/PageDown, Backspace/Tab/Return/Escape) dispatch off
 * WM_KEYDOWN/WM_KEYUP through vk_to_fl_key()'s table -- winkbd.h does
 * have a full VK_* table (an earlier pass of this comment claimed
 * otherwise off an incomplete search; not true).
 *
 * Click-multiplicity (double/triple-click) tracking and drag-and-drop
 * (present in the X11 backend) are not yet ported. Each is a real,
 * separate follow-up, not silently dropped.
 *
 * Fl_add_fd()/timers (bottom of this file) are implemented against
 * mwin's own fd-watch primitives (MwRegisterFdInput()/Output()/
 * Except(), src/mwin/winmain.c's MwSelect()) and SetTimer()/WM_TIMER,
 * rather than reimplementing select() the way fl_x11_event.c does
 * around Xlib's raw connection fd: mwin's message pump already does
 * exactly that internally (GetMessage() -> MwSelect(TRUE), which
 * select()s across the mouse/keyboard fds *and* every
 * MwRegisterFd*()-registered fd in one call), so this backend rides
 * that existing mechanism instead of building a second one next to it.
 */
#include <nuttx/config.h> /* must be first -- see fl_nx_window.c's comment */
#include <string.h>

#include "fl_nx_internal.h"
#include "../fl_backend.h"
#include "cfltk/Fl.h"
#include "cfltk/Fl_Group.h"

/* VK_* -> FL_* for the keys that never produce a WM_CHAR. cfltk's
 * FL_KEY encoding is upstream FLTK's own -- raw X11 keysym values for
 * anything outside the printable-ASCII range -- so this is a real
 * translation table, not an identity pass-through: e.g. FL_Left is
 * 0xff51, not VK_LEFT's 0x25. Returns 0 (not a cfltk FL_KEY at all)
 * for anything not in this table -- the caller drops the event
 * rather than forwarding a raw VK_ code that would collide with an
 * unrelated ASCII character on cfltk's side. */
static int vk_to_fl_key(WPARAM vk) {
    switch (vk) {
        case VK_BACK: return FL_BackSpace;
        case VK_TAB: return FL_Tab;
        case VK_RETURN: return FL_Enter;
        case VK_ESCAPE: return FL_Escape;
        case VK_LEFT: return FL_Left;
        case VK_UP: return FL_Up;
        case VK_RIGHT: return FL_Right;
        case VK_DOWN: return FL_Down;
        case VK_HOME: return FL_Home;
        case VK_END: return FL_End;
        case VK_INSERT: return FL_Insert;
        case VK_DELETE: return FL_Delete;
        case VK_PRIOR: return FL_Page_Up;
        case VK_NEXT: return FL_Page_Down;
        default: return 0;
    }
}

static int translate_button_state(WPARAM wparam) {
    /* Unlike real Win32, this microwindows tree's mouse messages don't
     * carry MK_SHIFT/MK_CONTROL bits in wParam (only the MK_*BUTTON
     * ones) -- there is no such #define here at all. Modifier state is
     * queried separately via GetKeyState(). */
    int s = 0;
    if (GetKeyState(VK_SHIFT) & 0x8000) s |= FL_SHIFT;
    if (GetKeyState(VK_CONTROL) & 0x8000) s |= FL_CTRL;
    if (wparam & MK_LBUTTON) s |= FL_BUTTON1;
    if (wparam & MK_MBUTTON) s |= FL_BUTTON2;
    if (wparam & MK_RBUTTON) s |= FL_BUTTON3;
    return s;
}

LRESULT CALLBACK fl_nx_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    Fl_Window *win;
    int x, y;

    switch (msg) {
        case WM_PAINT:
            win = fl_nx_find_window(hwnd);
            if (win) Fl_Widget_redraw(FL_WIDGET(win));
            return 0;

        case WM_SIZE: {
            Fl_Widget *w;
            int width = (int)(short)LOWORD(lparam), height = (int)(short)HIWORD(lparam);
            win = fl_nx_find_window(hwnd);
            if (!win) break;
            w = FL_WIDGET(win);
            if (w->w != width || w->h != height) {
                Fl_Group_resize(w, w->x, w->y, width, height);
                Fl_Widget_redraw(w);
            }
            return 0;
        }

        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN: {
            int button = msg == WM_LBUTTONDOWN ? 1 : msg == WM_MBUTTONDOWN ? 2 : 3;
            win = fl_nx_find_window(hwnd);
            if (!win) break;
            x = (int)(short)LOWORD(lparam);
            y = (int)(short)HIWORD(lparam);
            /* Click-multiplicity (double/triple-click, Fl_event_clicks())
             * not tracked yet -- always reports a plain single click.
             * See fl_x11_event.c's own tracking for the model to port. */
            fl_backend_set_event_state(x, y, x, y, 0, 0, button, 0, 1,
                                        translate_button_state(wparam), 0, NULL, 0);
            Fl_context_handle(FL_PUSH, win);
            return 0;
        }

        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP: {
            int button = msg == WM_LBUTTONUP ? 1 : msg == WM_MBUTTONUP ? 2 : 3;
            win = fl_nx_find_window(hwnd);
            if (!win) break;
            x = (int)(short)LOWORD(lparam);
            y = (int)(short)HIWORD(lparam);
            fl_backend_set_event_state(x, y, x, y, 0, 0, button, 0, 1,
                                        translate_button_state(wparam), 0, NULL, 0);
            Fl_context_handle(FL_RELEASE, win);
            return 0;
        }

        case WM_MOUSEMOVE:
            win = fl_nx_find_window(hwnd);
            if (!win) break;
            x = (int)(short)LOWORD(lparam);
            y = (int)(short)HIWORD(lparam);
            fl_backend_set_event_state(x, y, x, y, 0, 0, 0, 0, 0,
                                        translate_button_state(wparam), 0, NULL, 0);
            Fl_context_handle(FL_MOVE, win);
            return 0;

        case WM_CHAR: {
            char ch = (char)wparam;
            win = fl_nx_find_window(hwnd);
            if (!win) break;
            fl_backend_set_event_state(0, 0, 0, 0, 0, 0, 0, 0, 0,
                                        translate_button_state(0), (int)wparam, &ch, 1);
            Fl_context_handle(FL_KEYDOWN, win);
            return 0;
        }

        case WM_KEYDOWN:
        case WM_KEYUP: {
            int fl_key = vk_to_fl_key(wparam);
            win = fl_nx_find_window(hwnd);
            if (!win || !fl_key) break; /* printable keys already handled via WM_CHAR above */
            fl_backend_set_event_state(0, 0, 0, 0, 0, 0, 0, 0, 0,
                                        translate_button_state(0), fl_key, NULL, 0);
            Fl_context_handle(msg == WM_KEYDOWN ? FL_KEYDOWN : FL_KEYUP, win);
            return 0;
        }

        case WM_SETFOCUS:
        case WM_KILLFOCUS:
            win = fl_nx_find_window(hwnd);
            if (!win) break;
            Fl_context_handle(msg == WM_SETFOCUS ? FL_FOCUS : FL_UNFOCUS, win);
            return 0;

        case WM_CLOSE:
            win = fl_nx_find_window(hwnd);
            if (!win) break;
            Fl_context_handle(FL_CLOSE, win);
            /* Deliberately not forwarded to DefWindowProc: destroying
             * the native window is left to the app's close callback
             * calling Fl_Window_hide()/whatever it decides, same
             * contract as the X11 backend's WM_DELETE_WINDOW handling. */
            return 0;

        default:
            break;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

int fl_backend_ready(void) {
    MSG msg;
    return PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE) ? 1 : 0;
}

/* -------------------------------------------------------------------
 * fd watching (Fl_add_fd()/Fl_remove_fd(), see Fl.h) and the bounded
 * wait fl_backend_wait() needs to serve Fl_add_timeout() correctly.
 *
 * g_fdwatch_hwnd is a hidden, never-shown top-level window that exists
 * purely as a message target: MwRegisterFdInput()/Output()/Except()
 * (winmain.c) take an HWND to PostMessage() a WM_FDINPUT/WM_FDOUTPUT/
 * WM_FDEXCEPT to (wParam == the ready fd) once MwSelect() sees that fd
 * become ready; SetTimer()/KillTimer() (winuser.c) target the same
 * window to bound fl_backend_wait()'s blocking GetMessage() call to
 * `timeout_secs`, since mwin has no "GetMessage with an explicit max
 * wait" call of its own -- SetTimer()+WM_TIMER is the standard Win32
 * way to get one. One shared window (rather than, say, the first
 * cfltk Fl_Window) keeps fd-watch/timer lifetime independent of
 * however many real windows the app opens or closes.
 * ------------------------------------------------------------------- */

#define FL_FD_MAX 64
typedef struct { int fd; int when; Fl_FD_Handler *cb; void *data; int active; } Fl_FD_Slot;
static Fl_FD_Slot g_fds[FL_FD_MAX];

static const char k_fdwatch_class[] = "CfltkFdWatch";
static HWND g_fdwatch_hwnd = NULL;

/* The one-shot bound on fl_backend_wait()'s GetMessage() call below --
 * scoped to g_fdwatch_hwnd, so this can't collide with a timer id an
 * application itself passes to SetTimer() on one of its own windows. */
#define FL_WAIT_TIMER_ID 1

static LRESULT CALLBACK fl_nx_fdwatch_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    int i;
    switch (msg) {
        case WM_FDINPUT:
        case WM_FDOUTPUT:
        case WM_FDEXCEPT: {
            int when = msg == WM_FDINPUT ? FL_READ : msg == WM_FDOUTPUT ? FL_WRITE : FL_EXCEPT;
            int fd = (int)wparam;
            for (i = 0; i < FL_FD_MAX; i++) {
                if (g_fds[i].active && g_fds[i].fd == fd && (g_fds[i].when & when)) {
                    g_fds[i].cb(fd, g_fds[i].data);
                    break;
                }
            }
            return 0;
        }
        case WM_TIMER:
            /* fl_backend_wait()'s own bound firing -- nothing to do,
             * GetMessage() returning at all is the entire point. */
            return 0;
        default:
            return DefWindowProc(hwnd, msg, wparam, lparam);
    }
}

void fl_nx_fdwatch_init(void) {
    WNDCLASS wc;

    if (g_fdwatch_hwnd) return;

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = fl_nx_fdwatch_wndproc;
    wc.lpszClassName = k_fdwatch_class;
    RegisterClass(&wc);

    g_fdwatch_hwnd = CreateWindowEx(0, k_fdwatch_class, "", WS_POPUP,
                                     0, 0, 1, 1, HWND_DESKTOP, NULL, 0, NULL);
    /* Deliberately never ShowWindow()'d -- a message target only. */
}

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
            break;
        }
    }
    /* Pool exhausted: silently dropped, same policy as Fl_add_timeout(). */
    if (when & FL_READ) MwRegisterFdInput(g_fdwatch_hwnd, fd);
    if (when & FL_WRITE) MwRegisterFdOutput(g_fdwatch_hwnd, fd);
    if (when & FL_EXCEPT) MwRegisterFdExcept(g_fdwatch_hwnd, fd);
}

void Fl_remove_fd(int fd, int when) {
    int i;
    if (fd < 0) return;
    if (when & FL_READ) MwUnregisterFdInput(g_fdwatch_hwnd, fd);
    if (when & FL_WRITE) MwUnregisterFdOutput(g_fdwatch_hwnd, fd);
    if (when & FL_EXCEPT) MwUnregisterFdExcept(g_fdwatch_hwnd, fd);
    for (i = 0; i < FL_FD_MAX; i++) {
        if (g_fds[i].active && g_fds[i].fd == fd) {
            g_fds[i].when &= ~when;
            if (g_fds[i].when == 0) g_fds[i].active = 0;
        }
    }
}

int fl_backend_wait(double timeout_secs) {
    MSG msg;
    int dispatched = 0;

    /* Drain whatever's already queued first -- matches the X11
     * backend's own "drain, and if that alone found something, skip
     * the actual wait" shape. */
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        dispatched = 1;
    }
    if (dispatched) return 1;

    if (timeout_secs < 1e10 && timeout_secs <= 0.0) {
        /* Non-blocking poll (Fl_check()-style): nothing was queued,
         * and the caller explicitly asked not to wait for more. */
        return 0;
    }

    if (timeout_secs >= 1e10) {
        /* "Block forever" case: mwin's GetMessage() itself blocks
         * until a message arrives (mouse/keyboard/fd-ready/WM_TIMER),
         * so this is a real blocking wait, not a busy poll. */
        if (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            dispatched = 1;
        }
    } else {
        /* Bounded wait: SetTimer() gives GetMessage() an upper bound
         * via WM_TIMER, since mwin has no direct "wait up to N ms"
         * primitive of its own. round up to at least 1ms so a small
         * positive timeout can't collapse into an unbounded wait. */
        UINT ms = (UINT)(timeout_secs * 1000.0 + 0.5);
        if (ms < 1) ms = 1;
        SetTimer(g_fdwatch_hwnd, FL_WAIT_TIMER_ID, ms, NULL);
        if (GetMessage(&msg, NULL, 0, 0)) {
            int is_our_timeout = msg.hwnd == g_fdwatch_hwnd &&
                                  msg.message == WM_TIMER &&
                                  msg.wParam == FL_WAIT_TIMER_ID;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (!is_our_timeout) dispatched = 1;
        }
        KillTimer(g_fdwatch_hwnd, FL_WAIT_TIMER_ID);
    }

    /* Whatever arrived may have unblocked more than one message
     * (e.g. mwin posting both WM_MOUSEMOVE and a follow-up paint) --
     * drain the rest before returning, same as the initial pass. */
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        dispatched = 1;
    }
    return dispatched;
}
