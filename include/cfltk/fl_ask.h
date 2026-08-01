/*
 * cfltk - fl_ask.h
 *
 * C translation of FLTK 1.3 FL/fl_ask.H / src/fl_ask.cxx.
 *
 * Original class : none (free functions sharing one lazily-built,
 *                   reused dialog window -- upstream's own "innards()"
 *                   design, kept as-is).
 * New C structure : file-static widget pointers in src/dialogs/fl_ask.c,
 *                   built once by makeform() and resized/relabeled for
 *                   each call, exactly mirroring upstream.
 * Ownership       : the dialog window and its widgets live for the
 *                   process's lifetime once first built; cfltk never
 *                   frees them. Message/button text is never copied
 *                   (matches upstream): callers' strings, and the
 *                   internal vsnprintf() buffer, only need to stay
 *                   valid for the duration of the (blocking) call.
 * Known differences:
 *   - No Fl::grab() save/restore around showing the dialog: cfltk has
 *     no tracked "current grab" at the Fl:: level (only the raw,
 *     menu-popup-specific fl_backend_grab()/_ungrab(), which fl_ask has
 *     no reason to interact with) -- see docs/DESIGN.md.
 *   - The dialog window is not truly modal: cfltk has no Fl::modal()
 *     event-redirection stack (documented elsewhere as a known
 *     difference), so other windows remain independently clickable
 *     while a dialog is open. The dialog still blocks the calling
 *     function until closed, which is what every real caller depends
 *     on.
 *   - size_range() is not called on the dialog window (not implemented
 *     for Fl_Window yet, and moot anyway since cfltk has no interactive
 *     window-manager-driven resize path to lock down).
 *   - fl_message_hotspot() is split into a setter and
 *     fl_message_hotspot_get() (a getter), since C has no overloading.
 */
#ifndef CFLTK_FL_ASK_H
#define CFLTK_FL_ASK_H

#include "cfltk/Enumerations.h"
#include "cfltk/Fl_Widget.h"

#ifdef __cplusplus
extern "C" {
#endif

enum Fl_Beep {
    FL_BEEP_DEFAULT = 0,
    FL_BEEP_MESSAGE,
    FL_BEEP_ERROR,
    FL_BEEP_QUESTION,
    FL_BEEP_PASSWORD,
    FL_BEEP_NOTIFICATION
};

#ifdef __GNUC__
#define CFLTK_PRINTF_ATTR(fmt_idx, args_idx) __attribute__((__format__(__printf__, fmt_idx, args_idx)))
#else
#define CFLTK_PRINTF_ATTR(fmt_idx, args_idx)
#endif

void fl_beep(int type);
void fl_message(const char *fmt, ...) CFLTK_PRINTF_ATTR(1, 2);
void fl_alert(const char *fmt, ...) CFLTK_PRINTF_ATTR(1, 2);
/* Deprecated upstream too (uses "Yes"/"No", which doesn't conform to
 * FLTK's own HIG) -- kept for source compatibility; prefer fl_choice(). */
int fl_ask(const char *fmt, ...) CFLTK_PRINTF_ATTR(1, 2);
int fl_choice(const char *fmt, const char *b0, const char *b1, const char *b2, ...) CFLTK_PRINTF_ATTR(1, 5);
int fl_choice_n(const char *fmt, const char *b0, const char *b1, const char *b2, ...) CFLTK_PRINTF_ATTR(1, 5);
const char *fl_input(const char *fmt, const char *defstr, ...) CFLTK_PRINTF_ATTR(1, 3);
const char *fl_password(const char *fmt, const char *defstr, ...) CFLTK_PRINTF_ATTR(1, 3);

Fl_Widget *fl_message_icon(void);
extern Fl_Font fl_message_font_;
extern Fl_Fontsize fl_message_size_;
void fl_message_font(Fl_Font f, Fl_Fontsize s);

void fl_message_hotspot(int enable);
int fl_message_hotspot_get(void);

void fl_message_title(const char *title);
void fl_message_title_default(const char *title);

/* Pointers you can use to change cfltk to a foreign language, exactly
 * like upstream. */
extern const char *fl_no;
extern const char *fl_yes;
extern const char *fl_ok;
extern const char *fl_cancel;
extern const char *fl_close;

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_ASK_H */
