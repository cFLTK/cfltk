/*
 * cfltk - Fl_Menu_Window.h
 *
 * C translation of FLTK 1.3 FL/Fl_Menu_Window.H.
 *
 * Original class : Fl_Menu_Window : public Fl_Single_Window -- a
 *                   window type used for menus/popups, historically
 *                   able to draw into hardware overlay planes so a
 *                   popup didn't force the rest of the screen to
 *                   redraw.
 * New C structure : struct Fl_Menu_Window { Fl_Single_Window window; }.
 *                    Reuses fl_window_ops verbatim -- no widget-level
 *                    behavior differs from Fl_Single_Window in cfltk.
 * Known differences:
 *   - No hardware overlay-plane support (`set_overlay()`/
 *     `clear_overlay()`/`overlay()` still work -- they just toggle
 *     the existing FL_WIDGET_NO_OVERLAY widget flag -- but nothing
 *     reads it to change how drawing happens). Hardware overlay planes
 *     are a 1990s X11 server feature essentially unavailable on any
 *     modern compositing display; upstream's own overlay path already
 *     silently falls back to normal drawing when the server doesn't
 *     support it, which is effectively always true today. This class
 *     is otherwise identical to Fl_Single_Window; cfltk's own menu
 *     popup engine (src/menu/fl_menu_popup.c) and Fl_Tooltip both
 *     already get correct borderless-popup behavior from a plain
 *     Fl_Window with Fl_Window_set_border(win, 0) -- Fl_Menu_Window is
 *     provided for API-name parity with upstream, not because it adds
 *     capability nothing else already has.
 */
#ifndef CFLTK_FL_MENU_WINDOW_H
#define CFLTK_FL_MENU_WINDOW_H

#include "cfltk/Fl_Single_Window.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Fl_Menu_Window {
    Fl_Single_Window window;
} Fl_Menu_Window;

void Fl_Menu_Window_init(Fl_Menu_Window *self, int x, int y, int w, int h, const char *label);
Fl_Menu_Window *Fl_Menu_Window_new(int x, int y, int w, int h, const char *label);

static inline unsigned int Fl_Menu_Window_overlay(const Fl_Menu_Window *self) {
    return !(self->window.window.group.widget.flags & FL_WIDGET_NO_OVERLAY);
}
static inline void Fl_Menu_Window_set_overlay(Fl_Menu_Window *self) {
    self->window.window.group.widget.flags &= ~(unsigned)FL_WIDGET_NO_OVERLAY;
}
static inline void Fl_Menu_Window_clear_overlay(Fl_Menu_Window *self) {
    self->window.window.group.widget.flags |= FL_WIDGET_NO_OVERLAY;
}

#ifdef __cplusplus
}
#endif

#endif
