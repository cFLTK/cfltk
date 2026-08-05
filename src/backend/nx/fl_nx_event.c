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
 * Click-multiplicity (double/triple-click) tracking, drag-and-drop,
 * and Fl_add_fd()/Fl_remove_fd() timers (present in the X11 backend)
 * are not yet ported. Each is a real, separate follow-up, not
 * silently dropped.
 */
#include <nuttx/config.h> /* must be first -- see fl_nx_window.c's comment */
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

int fl_backend_wait(double timeout_secs) {
    MSG msg;
    int dispatched = 0;

    if (timeout_secs >= 1e10) {
        /* "Block forever" case: mwin's GetMessage() itself blocks
         * until a message arrives, so this is a real blocking wait,
         * not a busy poll. */
        if (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            dispatched = 1;
        }
        return dispatched;
    }

    /* Timed/non-blocking case: mwin's PeekMessage() has no timeout
     * parameter of its own, so this drains whatever is already queued
     * without actually sleeping up to timeout_secs. Good enough for a
     * blank window's own show/close cycle; a real timed wait is a
     * Timers-milestone follow-up (see fl_x11_event.c's select()-based
     * version for the shape it should take here once Fl_add_fd()/
     * timers exist on this backend too). */
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        dispatched = 1;
    }
    return dispatched;
}

/* Fl_add_fd()/Fl_remove_fd() -- NOT YET IMPLEMENTED.
 *
 * These aren't optional the way text rendering or images are: cfltk's
 * core (Fl.c's Fl_lock()/Fl_awake() self-pipe wakeup mechanism) calls
 * Fl_add_fd() unconditionally, so *something* has to exist here for
 * the link to succeed at all, even for an app that never touches
 * threading itself -- which is why this is a stub rather than an
 * omission. A real implementation needs fl_backend_wait() to select()
 * across watched fds the same way fl_x11_event.c's version does
 * (see its own g_fds[] registry), interleaved with mwin's message
 * pump -- worth doing together with real timer support (Fl_add_
 * timeout()), not before. Until then: no fd is ever actually watched,
 * so a worker thread calling Fl_awake() from off the main thread will
 * not wake a blocked fl_backend_wait(1e20) the way it does on X11 --
 * only relevant once this backend has a threaded app to run, which
 * the empty-window milestone does not. */
void Fl_add_fd(int fd, int when, Fl_FD_Handler *cb, void *data) {
    (void)fd; (void)when; (void)cb; (void)data;
}

void Fl_remove_fd(int fd, int when) {
    (void)fd; (void)when;
}
