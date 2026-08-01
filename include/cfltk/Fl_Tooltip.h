/*
 * cfltk - Fl_Tooltip.h
 *
 * C translation of FLTK 1.3 FL/Fl_Tooltip.H / src/Fl_Tooltip.cxx.
 *
 * Original class : Fl_Tooltip (static-methods-only class, never
 *                   instantiated -- a global subsystem, like Fl itself).
 * New C structure : free functions operating on file-static state in
 *                   src/core/Fl_Tooltip.c, exactly mirroring Fl.h's own
 *                   translation of the Fl class.
 * Ownership       : the popup window (a plain border-0 Fl_Window, same
 *                   override-redirect trick as the menu popup engine --
 *                   see fl_menu_popup.c) is created lazily on first use
 *                   and lives for the process's lifetime; cfltk never
 *                   frees it. Tooltip text (`tip`) is never copied,
 *                   exactly like upstream: the caller (Fl_Widget_tooltip
 *                   or Fl_Widget_copy_tooltip) owns the string's storage.
 * Known differences:
 *   - No word-wrap to wrap_width(): a tooltip longer than wrap_width()
 *     is simply capped/clipped at that width rather than reflowed onto
 *     more lines. Explicit '\n' in the tooltip text still starts a new
 *     line. See docs/DESIGN.md.
 *   - No Fl::option(OPTION_SHOW_TOOLTIPS) mirroring: enabled()/enable()
 *     use their own dedicated static flag, matching how Fl.h already
 *     handles Fl_visible_focus()/Fl_scrollbar_size() instead of a
 *     general Fl::option() mechanism.
 *   - Fl_Tooltip::current_window() (__APPLE__-only upstream) is omitted.
 */
#ifndef CFLTK_FL_TOOLTIP_H
#define CFLTK_FL_TOOLTIP_H

#include "cfltk/Enumerations.h"
#include "cfltk/Fl_Widget.h"

#ifdef __cplusplus
extern "C" {
#endif

float Fl_Tooltip_delay(void);
void Fl_Tooltip_set_delay(float f);
float Fl_Tooltip_hoverdelay(void);
void Fl_Tooltip_set_hoverdelay(float f);

int Fl_Tooltip_enabled(void);
void Fl_Tooltip_enable(int b);
void Fl_Tooltip_disable(void);

Fl_Font Fl_Tooltip_font(void);
void Fl_Tooltip_set_font(Fl_Font f);
Fl_Fontsize Fl_Tooltip_size(void);
void Fl_Tooltip_set_size(Fl_Fontsize s);
Fl_Color Fl_Tooltip_color(void);
void Fl_Tooltip_set_color(Fl_Color c);
Fl_Color Fl_Tooltip_textcolor(void);
void Fl_Tooltip_set_textcolor(Fl_Color c);
int Fl_Tooltip_margin_width(void);
void Fl_Tooltip_set_margin_width(int v);
int Fl_Tooltip_margin_height(void);
void Fl_Tooltip_set_margin_height(int v);
int Fl_Tooltip_wrap_width(void);
void Fl_Tooltip_set_wrap_width(int v);

Fl_Widget *Fl_Tooltip_current(void);
/* Acts as though enter(w) happened but does not pop up a tooltip --
 * used to suppress a stale tooltip reappearing (e.g. on FL_PUSH). */
void Fl_Tooltip_set_current(Fl_Widget *w);

/* Called by Fl.c's event dispatch (Fl_set_belowmouse(), FL_PUSH,
 * FL_KEYDOWN) -- upstream's Fl_Tooltip::enter_()/exit_(), always
 * installed here instead of lazily via upstream's enter/exit function
 * pointers (which only exist so Fl.cxx doesn't have to link
 * Fl_Tooltip.cxx unless a tooltip is actually set; cfltk's Fl_Tooltip.c
 * is a normal always-linked translation unit, so that indirection has
 * no purpose here). */
void Fl_Tooltip_enter(Fl_Widget *w);
void Fl_Tooltip_exit(Fl_Widget *w);
/* You may use this to provide tooltips for internal pieces of a
 * compound widget; call after Fl_set_belowmouse() to your widget, then
 * figure out what internal part the mouse is over. (x,w) are accepted
 * for source fidelity but unused, exactly like upstream. */
void Fl_Tooltip_enter_area(Fl_Widget *wid, int x, int y, int w, int h, const char *tip);

/* Called from Fl_context_widget_deleted() (Fl.c): clears `current`/
 * hides the popup if either pointed at the widget being destroyed.
 * Matches upstream's fl_throw_focus() tooltip cleanup. */
void Fl_Tooltip_widget_deleted(Fl_Widget *w);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_TOOLTIP_H */
