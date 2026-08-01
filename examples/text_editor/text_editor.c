/*
 * cfltk example: text_editor
 *
 * Exercises the Fl_Text_Buffer / Fl_Text_Display / Fl_Text_Editor
 * family: an editable Fl_Text_Editor with a naive from-scratch syntax
 * highlighter (built on Fl_Text_Display_highlight_data()'s style-buffer
 * mechanism), a second read-only Fl_Text_Display attached to the SAME
 * Fl_Text_Buffer (demonstrating that a buffer can be shown in more
 * than one display at once, kept in sync via the buffer's modify
 * callbacks), a live character-count Fl_Output, and a button that
 * toggles word wrap via Fl_Text_Display_wrap_mode().
 *
 * The highlighter is a deliberately simple whole-buffer re-tokenizer
 * (re-run on every edit) recognizing: line comments ("//..."), string
 * literals ("..."), and a small fixed C keyword list. It exists to
 * exercise the style-buffer code path end-to-end, not to be a real
 * editor's highlighter.
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cfltk/Fl.h"
#include "cfltk/Fl_Window.h"
#include "cfltk/Fl_Box.h"
#include "cfltk/Fl_Button.h"
#include "cfltk/Fl_Output.h"
#include "cfltk/Fl_Text_Editor.h"

static const char *keywords[] = {
    "int", "char", "void", "return", "if", "else", "while", "for",
    "struct", "const", "static", "double", "float", NULL
};

static int is_keyword(const char *s, int len) {
    int i;
    for (i = 0; keywords[i]; i++) {
        if ((int)strlen(keywords[i]) == len && strncmp(keywords[i], s, (size_t)len) == 0) return 1;
    }
    return 0;
}

/* Style classes, indexed as 'A' + n by Fl_Text_Display_position_style(). */
enum { STYLE_PLAIN = 0, STYLE_COMMENT, STYLE_KEYWORD, STYLE_STRING };

static const Fl_Text_Display_Style style_table[] = {
    { FL_FOREGROUND_COLOR, FL_COURIER, 14, 0 },       /* A: plain    */
    { FL_DARK_GREEN,       FL_COURIER_ITALIC, 14, 0 }, /* B: comment  */
    { FL_BLUE,             FL_COURIER_BOLD, 14, 0 },   /* C: keyword  */
    { FL_DARK_RED,         FL_COURIER, 14, 0 },        /* D: string   */
};

static Fl_Text_Buffer *style_buf;
static Fl_Input *status;

static void restyle(Fl_Text_Buffer *tbuf) {
    char *text = Fl_Text_Buffer_text(tbuf);
    int len = (int)strlen(text);
    char *style = (char *)malloc((size_t)len + 1);
    int i = 0;
    char buf[64];

    while (i < len) {
        if (text[i] == '/' && i + 1 < len && text[i + 1] == '/') {
            int start = i;
            while (i < len && text[i] != '\n') i++;
            memset(style + start, 'A' + STYLE_COMMENT, (size_t)(i - start));
        } else if (text[i] == '"') {
            int start = i++;
            while (i < len && text[i] != '"' && text[i] != '\n') i++;
            if (i < len && text[i] == '"') i++;
            memset(style + start, 'A' + STYLE_STRING, (size_t)(i - start));
        } else if (isalpha((unsigned char)text[i]) || text[i] == '_') {
            int start = i;
            while (i < len && (isalnum((unsigned char)text[i]) || text[i] == '_')) i++;
            memset(style + start, is_keyword(text + start, i - start) ? 'A' + STYLE_KEYWORD : 'A' + STYLE_PLAIN, (size_t)(i - start));
        } else {
            style[i] = 'A' + STYLE_PLAIN;
            i++;
        }
    }
    style[len] = '\0';

    Fl_Text_Buffer_set_text(style_buf, style);
    free(style);

    snprintf(buf, sizeof(buf), "%d characters", len);
    Fl_Input_set_value_str(status, buf);

    free(text);
}

static void buffer_changed_cb(int pos, int nInserted, int nDeleted, int nRestyled, const char *deletedText, void *data) {
    Fl_Text_Buffer *tbuf = (Fl_Text_Buffer *)data;
    (void)pos; (void)nInserted; (void)nDeleted; (void)nRestyled; (void)deletedText;
    restyle(tbuf);
}

static Fl_Text_Editor *editor;
static int wrapped = 0;

static void toggle_wrap_cb(Fl_Widget *w, void *data) {
    (void)data;
    wrapped = !wrapped;
    Fl_Text_Display_wrap_mode(&editor->display, wrapped ? FL_TEXT_DISPLAY_WRAP_AT_BOUNDS : FL_TEXT_DISPLAY_WRAP_NONE, 0);
    Fl_Widget_set_label(w, wrapped ? "Word wrap: on" : "Word wrap: off");
}

int main(void) {
    Fl_Window *window = Fl_Window_new(0, 0, 760, 560, "cfltk text editor");
    Fl_Text_Buffer *tbuf = Fl_Text_Buffer_new(0, 0);
    Fl_Text_Display *readonly;
    Fl_Button *wrap_btn;
    Fl_Box *lbl1, *lbl2;

    static const char initial[] =
        "// cfltk syntax-highlighting demo\n"
        "#include <stdio.h>\n\n"
        "int main(void) {\n"
        "    const char *msg = \"hello, cfltk\";\n"
        "    for (int i = 0; i < 3; i++) {\n"
        "        printf(\"%s\\n\", msg);\n"
        "    }\n"
        "    return 0;\n"
        "}\n";

    style_buf = Fl_Text_Buffer_new(0, 0);

    lbl1 = Fl_Box_new(20, 10, 400, 20, "Fl_Text_Editor (editable, syntax-highlighted)");
    Fl_Widget_set_align(&lbl1->widget, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    editor = Fl_Text_Editor_new(20, 35, 720, 300, NULL);
    Fl_Text_Display_set_buffer(&editor->display, tbuf);
    Fl_Text_Display_highlight_data(&editor->display, style_buf, style_table, 4, 'A' + STYLE_PLAIN, NULL, NULL);

    lbl2 = Fl_Box_new(20, 345, 400, 20, "Fl_Text_Display (read-only, same buffer)");
    Fl_Widget_set_align(&lbl2->widget, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    readonly = Fl_Text_Display_new(20, 370, 720, 120, NULL);
    Fl_Text_Display_set_buffer(readonly, tbuf);

    wrap_btn = Fl_Button_new(20, 500, 160, 30, "Word wrap: off");
    Fl_Widget_set_callback(&wrap_btn->widget, toggle_wrap_cb, NULL);

    status = Fl_Output_new(200, 500, 200, 30, NULL);

    Fl_Text_Buffer_add_modify_callback(tbuf, buffer_changed_cb, tbuf);
    Fl_Text_Buffer_set_text(tbuf, initial);
    Fl_Text_Display_set_insert_position(&editor->display, 0);

    Fl_Group_end(&window->group);
    Fl_Widget_show(FL_WIDGET(window));

    return Fl_run();
}
