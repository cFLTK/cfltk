/*
 * cfltk - Fl_Text_Editor.h
 *
 * Translated from FL/Fl_Text_Editor.H. Fl_Text_Editor adds no drawable
 * state of its own -- it is Fl_Text_Display plus a key-binding table and
 * an insert/overstrike mode flag -- so `struct Fl_Text_Editor` embeds
 * `Fl_Text_Display display;` as its first member, the same
 * struct-embeds-struct inheritance pattern used throughout cfltk.
 *
 * Known differences from upstream (consistent with Fl_Text_Display.h):
 *  - No true IME/composition state machine: upstream's Fl::compose()
 *    serves two purposes -- (1) classifying an FL_KEYBOARD event as
 *    "plain text to insert" vs. "function/movement key to dispatch via
 *    the key-binding table", and (2) tracking persistent dead-key/CJK
 *    composition state (Fl::compose_state) for marked-text underlining
 *    on platforms with an XIM/IME layer. Only (1) is ported here (see
 *    text_key_state() in Fl_Text_Editor.c) since it is load-bearing for
 *    ordinary typing -- without it, Shift-produced characters (capital
 *    letters, symbols) would never reach the text-insertion path. (2)
 *    is not ported: cfltk has no XIM/IME layer on any backend, so there
 *    is no composition state to track or underline.
 *  - No custom window cursor shapes: the upstream FL_KEYBOARD case hides
 *    the mouse cursor over the widget via window()->cursor(FL_CURSOR_NONE);
 *    cfltk has no per-window cursor-shape API (see Fl_Text_Display.h), so
 *    this is skipped.
 *  - No drag-and-drop: the FL_DND_ENTER/FL_DND_DRAG/FL_DND_LEAVE/
 *    FL_DND_RELEASE cases are dropped, matching Fl_Text_Display.
 *  - No system beep: the FL_PASTE-of-nothing case that upstream answers
 *    with fl_beep() silently no-ops instead.
 *  - __APPLE__-only bindings (Meta/Cmd key equivalents, kf_meta_move,
 *    kf_m_s_move) are still translated and exposed programmatically, but
 *    are not part of the default binding table on any cfltk backend,
 *    since cfltk has no notion of FL_META being the primary modifier.
 */
#ifndef CFLTK_FL_TEXT_EDITOR_H
#define CFLTK_FL_TEXT_EDITOR_H

#include "cfltk/Fl_Text_Display.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FL_TEXT_EDITOR_ANY_STATE (-1L)

typedef struct Fl_Text_Editor Fl_Text_Editor;

typedef int (*Fl_Text_Editor_Key_Func)(int key, Fl_Text_Editor *editor);

typedef struct Fl_Text_Editor_Key_Binding {
    int key;
    int state;
    Fl_Text_Editor_Key_Func function;
    struct Fl_Text_Editor_Key_Binding *next;
} Fl_Text_Editor_Key_Binding;

struct Fl_Text_Editor {
    Fl_Text_Display display;

    int insert_mode_;
    Fl_Text_Editor_Key_Binding *key_bindings;
    Fl_Text_Editor_Key_Func default_key_function_;
};

extern const Fl_WidgetOps fl_text_editor_ops;

void Fl_Text_Editor_init(Fl_Text_Editor *self, int x, int y, int w, int h, const char *label);
Fl_Text_Editor *Fl_Text_Editor_new(int x, int y, int w, int h, const char *label);
void Fl_Text_Editor_destroy(Fl_Widget *self_w);
int Fl_Text_Editor_handle(Fl_Widget *self_w, int event);

static inline void Fl_Text_Editor_set_insert_mode(Fl_Text_Editor *self, int b) { self->insert_mode_ = b; }
static inline int Fl_Text_Editor_insert_mode(const Fl_Text_Editor *self) { return self->insert_mode_; }

void Fl_Text_Editor_add_key_binding_to(int key, int state, Fl_Text_Editor_Key_Func f, Fl_Text_Editor_Key_Binding **list);
static inline void Fl_Text_Editor_add_key_binding(Fl_Text_Editor *self, int key, int state, Fl_Text_Editor_Key_Func f) {
    Fl_Text_Editor_add_key_binding_to(key, state, f, &self->key_bindings);
}
void Fl_Text_Editor_remove_key_binding_from(int key, int state, Fl_Text_Editor_Key_Binding **list);
static inline void Fl_Text_Editor_remove_key_binding(Fl_Text_Editor *self, int key, int state) {
    Fl_Text_Editor_remove_key_binding_from(key, state, &self->key_bindings);
}
void Fl_Text_Editor_remove_all_key_bindings_from(Fl_Text_Editor_Key_Binding **list);
static inline void Fl_Text_Editor_remove_all_key_bindings(Fl_Text_Editor *self) {
    Fl_Text_Editor_remove_all_key_bindings_from(&self->key_bindings);
}
void Fl_Text_Editor_add_default_key_bindings(Fl_Text_Editor_Key_Binding **list);

Fl_Text_Editor_Key_Func Fl_Text_Editor_bound_key_function_in(int key, int state, Fl_Text_Editor_Key_Binding *list);
static inline Fl_Text_Editor_Key_Func Fl_Text_Editor_bound_key_function(const Fl_Text_Editor *self, int key, int state) {
    return Fl_Text_Editor_bound_key_function_in(key, state, self->key_bindings);
}
static inline void Fl_Text_Editor_default_key_function(Fl_Text_Editor *self, Fl_Text_Editor_Key_Func f) { self->default_key_function_ = f; }

/* Global key binding list shared by every Fl_Text_Editor instance,
 * matching upstream's static Fl_Text_Editor::global_key_bindings. */
extern Fl_Text_Editor_Key_Binding *fl_text_editor_global_key_bindings;

int Fl_Text_Editor_kf_default(int c, Fl_Text_Editor *e);
int Fl_Text_Editor_kf_ignore(int c, Fl_Text_Editor *e);
int Fl_Text_Editor_kf_backspace(int c, Fl_Text_Editor *e);
int Fl_Text_Editor_kf_enter(int c, Fl_Text_Editor *e);
int Fl_Text_Editor_kf_move(int c, Fl_Text_Editor *e);
int Fl_Text_Editor_kf_shift_move(int c, Fl_Text_Editor *e);
int Fl_Text_Editor_kf_ctrl_move(int c, Fl_Text_Editor *e);
int Fl_Text_Editor_kf_c_s_move(int c, Fl_Text_Editor *e);
int Fl_Text_Editor_kf_meta_move(int c, Fl_Text_Editor *e);
int Fl_Text_Editor_kf_m_s_move(int c, Fl_Text_Editor *e);
int Fl_Text_Editor_kf_home(int c, Fl_Text_Editor *e);
int Fl_Text_Editor_kf_end(int c, Fl_Text_Editor *e);
int Fl_Text_Editor_kf_left(int c, Fl_Text_Editor *e);
int Fl_Text_Editor_kf_up(int c, Fl_Text_Editor *e);
int Fl_Text_Editor_kf_right(int c, Fl_Text_Editor *e);
int Fl_Text_Editor_kf_down(int c, Fl_Text_Editor *e);
int Fl_Text_Editor_kf_page_up(int c, Fl_Text_Editor *e);
int Fl_Text_Editor_kf_page_down(int c, Fl_Text_Editor *e);
int Fl_Text_Editor_kf_insert(int c, Fl_Text_Editor *e);
int Fl_Text_Editor_kf_delete(int c, Fl_Text_Editor *e);
int Fl_Text_Editor_kf_copy(int c, Fl_Text_Editor *e);
int Fl_Text_Editor_kf_cut(int c, Fl_Text_Editor *e);
int Fl_Text_Editor_kf_paste(int c, Fl_Text_Editor *e);
int Fl_Text_Editor_kf_select_all(int c, Fl_Text_Editor *e);
int Fl_Text_Editor_kf_undo(int c, Fl_Text_Editor *e);

int Fl_Text_Editor_handle_key(Fl_Text_Editor *self);
void Fl_Text_Editor_maybe_do_callback(Fl_Text_Editor *self);

void Fl_Text_Editor_tab_nav(Fl_Text_Editor *self, int val);
int Fl_Text_Editor_get_tab_nav(const Fl_Text_Editor *self);

#ifdef __cplusplus
}
#endif

#endif
