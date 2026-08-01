/*
 * cfltk - Fl_Text_Editor.c
 * See include/cfltk/Fl_Text_Editor.h for the class-conversion notes.
 * Translated from src/Fl_Text_Editor.cxx.
 */
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "cfltk/Fl_Text_Editor.h"
#include "cfltk/Fl.h"

Fl_Text_Editor_Key_Binding *fl_text_editor_global_key_bindings = NULL;

static Fl_Group *fl_text_editor_as_group(Fl_Widget *self_w) { return (Fl_Group *)self_w; }

const Fl_WidgetOps fl_text_editor_ops = {
    Fl_Text_Display_draw,
    Fl_Text_Editor_handle,
    Fl_Text_Display_resize,
    NULL, NULL,
    Fl_Text_Editor_destroy,
    fl_text_editor_as_group,
    NULL
};

/* -------------------------------------------------------------------
 * Construction
 * ---------------------------------------------------------------- */

void Fl_Text_Editor_init(Fl_Text_Editor *self, int x, int y, int w, int h, const char *label) {
    Fl_Text_Display_init(&self->display, x, y, w, h, label);
    self->display.group.widget.ops = &fl_text_editor_ops;

    self->display.mCursorOn = 1;
    self->insert_mode_ = 1;
    self->key_bindings = NULL;

    Fl_Text_Editor_add_default_key_bindings(&self->key_bindings);
    Fl_Text_Editor_default_key_function(self, Fl_Text_Editor_kf_default);
}

Fl_Text_Editor *Fl_Text_Editor_new(int x, int y, int w, int h, const char *label) {
    Fl_Text_Editor *self = (Fl_Text_Editor *)malloc(sizeof(Fl_Text_Editor));
    Fl_Text_Editor_init(self, x, y, w, h, label);
    return self;
}

void Fl_Text_Editor_destroy(Fl_Widget *self_w) {
    Fl_Text_Editor *self = (Fl_Text_Editor *)self_w;
    Fl_Text_Editor_remove_all_key_bindings(self);
    Fl_Text_Display_destroy(self_w);
}

/* -------------------------------------------------------------------
 * Default key bindings
 * ---------------------------------------------------------------- */

static const struct {
    int key;
    int state;
    Fl_Text_Editor_Key_Func func;
} default_key_bindings[] = {
    { FL_Escape,    FL_TEXT_EDITOR_ANY_STATE, Fl_Text_Editor_kf_ignore     },
    { FL_Enter,     FL_TEXT_EDITOR_ANY_STATE, Fl_Text_Editor_kf_enter      },
    { FL_KP_Enter,  FL_TEXT_EDITOR_ANY_STATE, Fl_Text_Editor_kf_enter      },
    { FL_BackSpace, FL_TEXT_EDITOR_ANY_STATE, Fl_Text_Editor_kf_backspace  },
    { FL_Insert,    FL_TEXT_EDITOR_ANY_STATE, Fl_Text_Editor_kf_insert     },
    { FL_Delete,    FL_TEXT_EDITOR_ANY_STATE, Fl_Text_Editor_kf_delete     },
    { FL_Home,      0,                        Fl_Text_Editor_kf_move       },
    { FL_End,       0,                        Fl_Text_Editor_kf_move       },
    { FL_Left,      0,                        Fl_Text_Editor_kf_move       },
    { FL_Up,        0,                        Fl_Text_Editor_kf_move       },
    { FL_Right,     0,                        Fl_Text_Editor_kf_move       },
    { FL_Down,      0,                        Fl_Text_Editor_kf_move       },
    { FL_Page_Up,   0,                        Fl_Text_Editor_kf_move       },
    { FL_Page_Down, 0,                        Fl_Text_Editor_kf_move       },
    { FL_Home,      FL_SHIFT,                 Fl_Text_Editor_kf_shift_move },
    { FL_End,       FL_SHIFT,                 Fl_Text_Editor_kf_shift_move },
    { FL_Left,      FL_SHIFT,                 Fl_Text_Editor_kf_shift_move },
    { FL_Up,        FL_SHIFT,                 Fl_Text_Editor_kf_shift_move },
    { FL_Right,     FL_SHIFT,                 Fl_Text_Editor_kf_shift_move },
    { FL_Down,      FL_SHIFT,                 Fl_Text_Editor_kf_shift_move },
    { FL_Page_Up,   FL_SHIFT,                 Fl_Text_Editor_kf_shift_move },
    { FL_Page_Down, FL_SHIFT,                 Fl_Text_Editor_kf_shift_move },
    { FL_Home,      FL_CTRL,                  Fl_Text_Editor_kf_ctrl_move  },
    { FL_End,       FL_CTRL,                  Fl_Text_Editor_kf_ctrl_move  },
    { FL_Left,      FL_CTRL,                  Fl_Text_Editor_kf_ctrl_move  },
    { FL_Up,        FL_CTRL,                  Fl_Text_Editor_kf_ctrl_move  },
    { FL_Right,     FL_CTRL,                  Fl_Text_Editor_kf_ctrl_move  },
    { FL_Down,      FL_CTRL,                  Fl_Text_Editor_kf_ctrl_move  },
    { FL_Page_Up,   FL_CTRL,                  Fl_Text_Editor_kf_ctrl_move  },
    { FL_Page_Down, FL_CTRL,                  Fl_Text_Editor_kf_ctrl_move  },
    { FL_Home,      FL_CTRL | FL_SHIFT,       Fl_Text_Editor_kf_c_s_move   },
    { FL_End,       FL_CTRL | FL_SHIFT,       Fl_Text_Editor_kf_c_s_move   },
    { FL_Left,      FL_CTRL | FL_SHIFT,       Fl_Text_Editor_kf_c_s_move   },
    { FL_Up,        FL_CTRL | FL_SHIFT,       Fl_Text_Editor_kf_c_s_move   },
    { FL_Right,     FL_CTRL | FL_SHIFT,       Fl_Text_Editor_kf_c_s_move   },
    { FL_Down,      FL_CTRL | FL_SHIFT,       Fl_Text_Editor_kf_c_s_move   },
    { FL_Page_Up,   FL_CTRL | FL_SHIFT,       Fl_Text_Editor_kf_c_s_move   },
    { FL_Page_Down, FL_CTRL | FL_SHIFT,       Fl_Text_Editor_kf_c_s_move   },
    { 'z',          FL_CTRL,                  Fl_Text_Editor_kf_undo       },
    { '/',          FL_CTRL,                  Fl_Text_Editor_kf_undo       },
    { 'x',          FL_CTRL,                  Fl_Text_Editor_kf_cut        },
    { FL_Delete,    FL_SHIFT,                 Fl_Text_Editor_kf_cut        },
    { 'c',          FL_CTRL,                  Fl_Text_Editor_kf_copy       },
    { FL_Insert,    FL_CTRL,                  Fl_Text_Editor_kf_copy       },
    { 'v',          FL_CTRL,                  Fl_Text_Editor_kf_paste      },
    { FL_Insert,    FL_SHIFT,                 Fl_Text_Editor_kf_paste      },
    { 'a',          FL_CTRL,                  Fl_Text_Editor_kf_select_all },
    { 0,            0,                        NULL                         }
};

void Fl_Text_Editor_add_default_key_bindings(Fl_Text_Editor_Key_Binding **list) {
    int i;
    for (i = 0; default_key_bindings[i].key; i++)
        Fl_Text_Editor_add_key_binding_to(default_key_bindings[i].key, default_key_bindings[i].state, default_key_bindings[i].func, list);
}

Fl_Text_Editor_Key_Func Fl_Text_Editor_bound_key_function_in(int key, int state, Fl_Text_Editor_Key_Binding *list) {
    Fl_Text_Editor_Key_Binding *cur;
    for (cur = list; cur; cur = cur->next)
        if (cur->key == key && (cur->state == FL_TEXT_EDITOR_ANY_STATE || cur->state == state)) break;
    return cur ? cur->function : NULL;
}

void Fl_Text_Editor_remove_all_key_bindings_from(Fl_Text_Editor_Key_Binding **list) {
    Fl_Text_Editor_Key_Binding *cur, *next;
    for (cur = *list; cur; cur = next) {
        next = cur->next;
        free(cur);
    }
    *list = NULL;
}

void Fl_Text_Editor_remove_key_binding_from(int key, int state, Fl_Text_Editor_Key_Binding **list) {
    Fl_Text_Editor_Key_Binding *cur, *last = NULL;
    for (cur = *list; cur; last = cur, cur = cur->next)
        if (cur->key == key && cur->state == state) break;
    if (!cur) return;
    if (last) last->next = cur->next;
    else *list = cur->next;
    free(cur);
}

void Fl_Text_Editor_add_key_binding_to(int key, int state, Fl_Text_Editor_Key_Func function, Fl_Text_Editor_Key_Binding **list) {
    Fl_Text_Editor_Key_Binding *kb = (Fl_Text_Editor_Key_Binding *)malloc(sizeof(Fl_Text_Editor_Key_Binding));
    kb->key = key;
    kb->state = state;
    kb->function = function;
    kb->next = *list;
    *list = kb;
}

/* -------------------------------------------------------------------
 * kf_* key functions
 * ---------------------------------------------------------------- */

static void kill_selection(Fl_Text_Editor *e) {
    Fl_Text_Buffer *buf = Fl_Text_Display_buffer(&e->display);
    if (Fl_Text_Buffer_selected(buf)) {
        Fl_Text_Display_set_insert_position(&e->display, Fl_Text_Buffer_primary_selection(buf)->start);
        Fl_Text_Buffer_remove_selection(buf);
    }
}

int Fl_Text_Editor_kf_default(int c, Fl_Text_Editor *e) {
    char s[2];
    if (!c || (!isprint(c) && c != '\t')) return 0;
    s[0] = (char)c;
    s[1] = '\0';
    kill_selection(e);
    if (Fl_Text_Editor_insert_mode(e)) Fl_Text_Display_insert(&e->display, s);
    else Fl_Text_Display_overstrike(&e->display, s);
    Fl_Text_Display_show_insert_position(&e->display);
    Fl_Widget_set_changed(&e->display.group.widget);
    if (Fl_Widget_when(&e->display.group.widget) & FL_WHEN_CHANGED) Fl_Widget_do_callback(&e->display.group.widget);
    return 1;
}

int Fl_Text_Editor_kf_ignore(int c, Fl_Text_Editor *e) {
    (void)c; (void)e;
    return 0;
}

int Fl_Text_Editor_kf_backspace(int c, Fl_Text_Editor *e) {
    Fl_Text_Buffer *buf = Fl_Text_Display_buffer(&e->display);
    (void)c;
    if (!Fl_Text_Buffer_selected(buf) && Fl_Text_Display_move_left(&e->display)) {
        int p1 = Fl_Text_Display_insert_position(&e->display);
        int p2 = Fl_Text_Buffer_next_char(buf, p1);
        Fl_Text_Buffer_select(buf, p1, p2);
    }
    kill_selection(e);
    Fl_Text_Display_show_insert_position(&e->display);
    Fl_Widget_set_changed(&e->display.group.widget);
    if (Fl_Widget_when(&e->display.group.widget) & FL_WHEN_CHANGED) Fl_Widget_do_callback(&e->display.group.widget);
    return 1;
}

int Fl_Text_Editor_kf_enter(int c, Fl_Text_Editor *e) {
    (void)c;
    kill_selection(e);
    Fl_Text_Display_insert(&e->display, "\n");
    Fl_Text_Display_show_insert_position(&e->display);
    Fl_Widget_set_changed(&e->display.group.widget);
    if (Fl_Widget_when(&e->display.group.widget) & FL_WHEN_CHANGED) Fl_Widget_do_callback(&e->display.group.widget);
    return 1;
}

int Fl_Text_Editor_kf_move(int c, Fl_Text_Editor *e) {
    Fl_Text_Buffer *buf = Fl_Text_Display_buffer(&e->display);
    int selected = Fl_Text_Buffer_selected(buf);
    int i;
    if (!selected) e->display.dragPos = Fl_Text_Display_insert_position(&e->display);
    Fl_Text_Buffer_unselect(buf);
    Fl_copy("", 0, 0);
    switch (c) {
        case FL_Home:
            Fl_Text_Display_set_insert_position(&e->display, Fl_Text_Buffer_line_start(buf, Fl_Text_Display_insert_position(&e->display)));
            break;
        case FL_End:
            Fl_Text_Display_set_insert_position(&e->display, Fl_Text_Buffer_line_end(buf, Fl_Text_Display_insert_position(&e->display)));
            break;
        case FL_Left:
            Fl_Text_Display_move_left(&e->display);
            break;
        case FL_Right:
            Fl_Text_Display_move_right(&e->display);
            break;
        case FL_Up:
            Fl_Text_Display_move_up(&e->display);
            break;
        case FL_Down:
            Fl_Text_Display_move_down(&e->display);
            break;
        case FL_Page_Up:
            for (i = 0; i < e->display.mNVisibleLines - 1; i++) Fl_Text_Display_move_up(&e->display);
            break;
        case FL_Page_Down:
            for (i = 0; i < e->display.mNVisibleLines - 1; i++) Fl_Text_Display_move_down(&e->display);
            break;
        default:
            break;
    }
    Fl_Text_Display_show_insert_position(&e->display);
    return 1;
}

int Fl_Text_Editor_kf_shift_move(int c, Fl_Text_Editor *e) {
    char *copy;
    Fl_Text_Editor_kf_move(c, e);
    Fl_Text_Display_drag_me(&e->display, Fl_Text_Display_insert_position(&e->display));
    copy = Fl_Text_Buffer_selection_text(Fl_Text_Display_buffer(&e->display));
    if (copy) {
        Fl_copy(copy, (int)strlen(copy), 0);
        free(copy);
    }
    return 1;
}

int Fl_Text_Editor_kf_ctrl_move(int c, Fl_Text_Editor *e) {
    Fl_Text_Buffer *buf = Fl_Text_Display_buffer(&e->display);
    if (!Fl_Text_Buffer_selected(buf)) e->display.dragPos = Fl_Text_Display_insert_position(&e->display);
    if (c != FL_Up && c != FL_Down) {
        Fl_Text_Buffer_unselect(buf);
        Fl_copy("", 0, 0);
        Fl_Text_Display_show_insert_position(&e->display);
    }
    switch (c) {
        case FL_Home:
            Fl_Text_Display_set_insert_position(&e->display, 0);
            Fl_Text_Display_scroll(&e->display, 0, 0);
            break;
        case FL_End:
            Fl_Text_Display_set_insert_position(&e->display, Fl_Text_Buffer_length(buf));
            Fl_Text_Display_scroll(&e->display, Fl_Text_Display_count_lines(&e->display, 0, Fl_Text_Buffer_length(buf), 1), 0);
            break;
        case FL_Left:
            Fl_Text_Display_previous_word(&e->display);
            break;
        case FL_Right:
            Fl_Text_Display_next_word(&e->display);
            break;
        case FL_Up:
            Fl_Text_Display_scroll(&e->display, e->display.mTopLineNum - 1, e->display.mHorizOffset);
            break;
        case FL_Down:
            Fl_Text_Display_scroll(&e->display, e->display.mTopLineNum + 1, e->display.mHorizOffset);
            break;
        case FL_Page_Up:
            Fl_Text_Display_set_insert_position(&e->display, e->display.mLineStarts[0]);
            break;
        case FL_Page_Down:
            Fl_Text_Display_set_insert_position(&e->display, e->display.mLineStarts[e->display.mNVisibleLines - 2]);
            break;
        default:
            break;
    }
    return 1;
}

int Fl_Text_Editor_kf_meta_move(int c, Fl_Text_Editor *e) {
    Fl_Text_Buffer *buf = Fl_Text_Display_buffer(&e->display);
    if (!Fl_Text_Buffer_selected(buf)) e->display.dragPos = Fl_Text_Display_insert_position(&e->display);
    if (c != FL_Up && c != FL_Down) {
        Fl_Text_Buffer_unselect(buf);
        Fl_copy("", 0, 0);
        Fl_Text_Display_show_insert_position(&e->display);
    }
    switch (c) {
        case FL_Up:
            Fl_Text_Display_set_insert_position(&e->display, 0);
            Fl_Text_Display_scroll(&e->display, 0, 0);
            break;
        case FL_Down:
            Fl_Text_Display_set_insert_position(&e->display, Fl_Text_Buffer_length(buf));
            Fl_Text_Display_scroll(&e->display, Fl_Text_Display_count_lines(&e->display, 0, Fl_Text_Buffer_length(buf), 1), 0);
            break;
        case FL_Left:
            Fl_Text_Editor_kf_move(FL_Home, e);
            break;
        case FL_Right:
            Fl_Text_Editor_kf_move(FL_End, e);
            break;
        default:
            break;
    }
    return 1;
}

int Fl_Text_Editor_kf_m_s_move(int c, Fl_Text_Editor *e) {
    Fl_Text_Editor_kf_meta_move(c, e);
    Fl_Text_Display_drag_me(&e->display, Fl_Text_Display_insert_position(&e->display));
    return 1;
}

int Fl_Text_Editor_kf_c_s_move(int c, Fl_Text_Editor *e) {
    Fl_Text_Editor_kf_ctrl_move(c, e);
    Fl_Text_Display_drag_me(&e->display, Fl_Text_Display_insert_position(&e->display));
    return 1;
}

int Fl_Text_Editor_kf_home(int c, Fl_Text_Editor *e) { (void)c; return Fl_Text_Editor_kf_move(FL_Home, e); }
int Fl_Text_Editor_kf_end(int c, Fl_Text_Editor *e) { (void)c; return Fl_Text_Editor_kf_move(FL_End, e); }
int Fl_Text_Editor_kf_left(int c, Fl_Text_Editor *e) { (void)c; return Fl_Text_Editor_kf_move(FL_Left, e); }
int Fl_Text_Editor_kf_up(int c, Fl_Text_Editor *e) { (void)c; return Fl_Text_Editor_kf_move(FL_Up, e); }
int Fl_Text_Editor_kf_right(int c, Fl_Text_Editor *e) { (void)c; return Fl_Text_Editor_kf_move(FL_Right, e); }
int Fl_Text_Editor_kf_down(int c, Fl_Text_Editor *e) { (void)c; return Fl_Text_Editor_kf_move(FL_Down, e); }
int Fl_Text_Editor_kf_page_up(int c, Fl_Text_Editor *e) { (void)c; return Fl_Text_Editor_kf_move(FL_Page_Up, e); }
int Fl_Text_Editor_kf_page_down(int c, Fl_Text_Editor *e) { (void)c; return Fl_Text_Editor_kf_move(FL_Page_Down, e); }

int Fl_Text_Editor_kf_insert(int c, Fl_Text_Editor *e) {
    (void)c;
    Fl_Text_Editor_set_insert_mode(e, Fl_Text_Editor_insert_mode(e) ? 0 : 1);
    return 1;
}

int Fl_Text_Editor_kf_delete(int c, Fl_Text_Editor *e) {
    Fl_Text_Buffer *buf = Fl_Text_Display_buffer(&e->display);
    (void)c;
    if (!Fl_Text_Buffer_selected(buf)) {
        int p1 = Fl_Text_Display_insert_position(&e->display);
        int p2 = Fl_Text_Buffer_next_char(buf, p1);
        Fl_Text_Buffer_select(buf, p1, p2);
    }
    kill_selection(e);
    Fl_Text_Display_show_insert_position(&e->display);
    Fl_Widget_set_changed(&e->display.group.widget);
    if (Fl_Widget_when(&e->display.group.widget) & FL_WHEN_CHANGED) Fl_Widget_do_callback(&e->display.group.widget);
    return 1;
}

int Fl_Text_Editor_kf_copy(int c, Fl_Text_Editor *e) {
    char *copy;
    (void)c;
    if (!Fl_Text_Buffer_selected(Fl_Text_Display_buffer(&e->display))) return 1;
    copy = Fl_Text_Buffer_selection_text(Fl_Text_Display_buffer(&e->display));
    if (*copy) Fl_copy(copy, (int)strlen(copy), 1);
    free(copy);
    Fl_Text_Display_show_insert_position(&e->display);
    return 1;
}

int Fl_Text_Editor_kf_cut(int c, Fl_Text_Editor *e) {
    Fl_Text_Editor_kf_copy(c, e);
    kill_selection(e);
    Fl_Widget_set_changed(&e->display.group.widget);
    if (Fl_Widget_when(&e->display.group.widget) & FL_WHEN_CHANGED) Fl_Widget_do_callback(&e->display.group.widget);
    return 1;
}

int Fl_Text_Editor_kf_paste(int c, Fl_Text_Editor *e) {
    (void)c;
    kill_selection(e);
    Fl_paste(&e->display.group.widget, 1);
    Fl_Text_Display_show_insert_position(&e->display);
    Fl_Widget_set_changed(&e->display.group.widget);
    if (Fl_Widget_when(&e->display.group.widget) & FL_WHEN_CHANGED) Fl_Widget_do_callback(&e->display.group.widget);
    return 1;
}

int Fl_Text_Editor_kf_select_all(int c, Fl_Text_Editor *e) {
    Fl_Text_Buffer *buf = Fl_Text_Display_buffer(&e->display);
    char *copy;
    (void)c;
    Fl_Text_Buffer_select(buf, 0, Fl_Text_Buffer_length(buf));
    copy = Fl_Text_Buffer_selection_text(buf);
    if (*copy) Fl_copy(copy, (int)strlen(copy), 0);
    free(copy);
    return 1;
}

int Fl_Text_Editor_kf_undo(int c, Fl_Text_Editor *e) {
    Fl_Text_Buffer *buf = Fl_Text_Display_buffer(&e->display);
    int crsr, ret;
    (void)c;
    Fl_Text_Buffer_unselect(buf);
    Fl_copy("", 0, 0);
    ret = Fl_Text_Buffer_undo(buf, &crsr);
    Fl_Text_Display_set_insert_position(&e->display, crsr);
    Fl_Text_Display_show_insert_position(&e->display);
    Fl_Widget_set_changed(&e->display.group.widget);
    if (Fl_Widget_when(&e->display.group.widget) & FL_WHEN_CHANGED) Fl_Widget_do_callback(&e->display.group.widget);
    return ret;
}

/* -------------------------------------------------------------------
 * handle_key() / maybe_do_callback() / handle()
 *
 * Upstream's handle_key() checks Fl::compose() FIRST, before ever
 * consulting the key-binding table: for ordinary printable-text
 * keystrokes (including Shift-produced capitals and symbols, which
 * upstream's own kf_default() can only reach for UNmodified keys
 * because of the "if (default_key_function_ && !state)" guard further
 * down), Fl::compose() short-circuits straight to inserting
 * Fl::event_text() and returns -- the key-binding table is consulted
 * only when compose() says "this is a function/movement key, not
 * text". Dropping compose() entirely (as an early draft of this file
 * did) silently broke every Shift-modified character: kf_default's own
 * "!state" guard rejected them since Shift is part of the masked
 * state, and they were never in the key-binding table either.
 *
 * text_key_state() below reimplements upstream's non-Apple
 * Fl::compose() classification (see src/Fl_compose.cxx): true for
 * plain printable/high-bit text, false for control characters and for
 * Alt/Meta/Ctrl-modified non-high-bit keys (which must fall through to
 * the key-binding table instead, e.g. Ctrl+c/Ctrl+v). What is NOT
 * ported is the persistent XIM dead-key/CJK composition state machine
 * (Fl::compose_state, marked-text underlining) -- true multi-keystroke
 * IME composition -- since cfltk has no XIM/IME layer on any backend
 * (see Fl_Text_Editor.h known-differences note).
 * ---------------------------------------------------------------- */

static int text_key_state(void) {
    int state = Fl_event_state();
    unsigned char ascii = (unsigned char)Fl_event_text()[0];
    if ((state & (FL_ALT | FL_META | FL_CTRL)) && !(ascii & 128)) return 0;
    return (ascii & ~31) && ascii != 127;
}

int Fl_Text_Editor_handle_key(Fl_Text_Editor *self) {
    Fl_Text_Display *d = &self->display;
    int key, state, c;
    Fl_Text_Editor_Key_Func f;

    if (text_key_state()) {
        Fl_Widget *w = &d->group.widget;
        kill_selection(self);
        if (Fl_event_length()) {
            if (Fl_Text_Editor_insert_mode(self)) Fl_Text_Display_insert(d, Fl_event_text());
            else Fl_Text_Display_overstrike(d, Fl_event_text());
        }
        Fl_Text_Display_show_insert_position(d);
        Fl_Widget_set_changed(w);
        if (Fl_Widget_when(w) & FL_WHEN_CHANGED) Fl_Widget_do_callback(w);
        return 1;
    }

    key = Fl_event_key();
    state = Fl_event_state();
    c = Fl_event_text()[0];
    state &= (FL_SHIFT | FL_CTRL | FL_ALT | FL_META);

    f = Fl_Text_Editor_bound_key_function_in(key, state, fl_text_editor_global_key_bindings);
    if (!f) f = Fl_Text_Editor_bound_key_function(self, key, state);
    if (f) return f(key, self);
    if (self->default_key_function_ && !state) return self->default_key_function_(c, self);
    return 0;
}

void Fl_Text_Editor_maybe_do_callback(Fl_Text_Editor *self) {
    Fl_Widget *w = &self->display.group.widget;
    if (Fl_Widget_changed(w) || (Fl_Widget_when(w) & FL_WHEN_NOT_CHANGED)) Fl_Widget_do_callback(w);
}

int Fl_Text_Editor_handle(Fl_Widget *self_w, int event) {
    Fl_Text_Editor *self = (Fl_Text_Editor *)self_w;
    Fl_Text_Display *d = &self->display;

    if (!Fl_Text_Display_buffer(d)) return 0;

    switch (event) {
        case FL_FOCUS:
            Fl_Text_Display_show_cursor(d, d->mCursorOn);
            if (Fl_Text_Buffer_selected(Fl_Text_Display_buffer(d))) Fl_Widget_redraw(self_w);
            Fl_set_focus(self_w);
            return 1;

        case FL_UNFOCUS:
            Fl_Text_Display_show_cursor(d, d->mCursorOn);
            if (Fl_Text_Buffer_selected(Fl_Text_Display_buffer(d))) Fl_Widget_redraw(self_w);
            /* fall through */
        case FL_HIDE:
            if (Fl_Widget_when(self_w) & FL_WHEN_RELEASE) Fl_Text_Editor_maybe_do_callback(self);
            return 1;

        case FL_KEYBOARD:
            return Fl_Text_Editor_handle_key(self);

        case FL_PASTE:
            if (!Fl_event_text()) return 1; /* no system beep, see header */
            Fl_Text_Buffer_remove_selection(Fl_Text_Display_buffer(d));
            if (Fl_Text_Editor_insert_mode(self)) Fl_Text_Display_insert(d, Fl_event_text());
            else Fl_Text_Display_overstrike(d, Fl_event_text());
            Fl_Text_Display_show_insert_position(d);
            Fl_Widget_set_changed(self_w);
            if (Fl_Widget_when(self_w) & FL_WHEN_CHANGED) Fl_Widget_do_callback(self_w);
            return 1;

        case FL_ENTER:
            Fl_Text_Display_show_cursor(d, d->mCursorOn);
            return 1;

        case FL_PUSH:
            if (Fl_event_button() == 2) {
                int pos;
                if (Fl_Group_handle(self_w, event)) return 1;
                d->dragType = FL_TEXT_DISPLAY_DRAG_NONE;
                if (Fl_Text_Buffer_selected(Fl_Text_Display_buffer(d))) Fl_Text_Buffer_unselect(Fl_Text_Display_buffer(d));
                pos = Fl_Text_Display_xy_to_position(d, Fl_event_x(), Fl_event_y(), FL_TEXT_DISPLAY_CURSOR_POS);
                Fl_Text_Display_set_insert_position(d, pos);
                Fl_paste(self_w, 0);
                Fl_set_focus(self_w);
                Fl_Widget_set_changed(self_w);
                if (Fl_Widget_when(self_w) & FL_WHEN_CHANGED) Fl_Widget_do_callback(self_w);
                return 1;
            }
            break;

        case FL_SHORTCUT:
            if (!(Fl_Text_Display_shortcut(d) ? Fl_test_shortcut((Fl_Shortcut)Fl_Text_Display_shortcut(d)) : Fl_Widget_test_shortcut(self_w))) return 0;
            if (Fl_visible_focus() && Fl_Widget_handle(self_w, FL_FOCUS)) { Fl_set_focus(self_w); return 1; }
            break;

        default:
            break;
    }

    return Fl_Text_Display_handle(self_w, event);
}

/* -------------------------------------------------------------------
 * Tab-navigation convenience wrapper (ABI-version-gated in upstream;
 * cfltk has no ABI-version macro, so this is always available).
 * ---------------------------------------------------------------- */

void Fl_Text_Editor_tab_nav(Fl_Text_Editor *self, int val) {
    if (val) Fl_Text_Editor_add_key_binding(self, FL_Tab, 0, Fl_Text_Editor_kf_ignore);
    else Fl_Text_Editor_remove_key_binding(self, FL_Tab, 0);
}

int Fl_Text_Editor_get_tab_nav(const Fl_Text_Editor *self) {
    return Fl_Text_Editor_bound_key_function(self, FL_Tab, 0) == Fl_Text_Editor_kf_ignore ? 1 : 0;
}
