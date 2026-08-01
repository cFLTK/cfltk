/*
 * cfltk - Fl_File_Input.h
 *
 * C translation of FLTK 1.3 FL/Fl_File_Input.H / src/Fl_File_Input.cxx.
 *
 * Original class : Fl_File_Input : public Fl_Input (own draw()/
 *                   handle(); a text input with a clickable directory-
 *                   breadcrumb bar drawn above the text).
 * New C structure : struct Fl_File_Input { Fl_Input input; Fl_Color
 *                    errorcolor_; char ok_entry_; uchar down_box_;
 *                    short buttons_[200]; short pressed_; }, embedding
 *                    Fl_Input as its first member.
 * Vtbl            : fl_file_input_ops (own draw()/handle()).
 * Known differences:
 *   - No window()->cursor(FL_CURSOR_INSERT/_DEFAULT) hinting on
 *     FL_MOVE/FL_ENTER -- cfltk has no cursor-shape API yet (same
 *     omission as Fl_Tile, see its header).
 */
#ifndef CFLTK_FL_FILE_INPUT_H
#define CFLTK_FL_FILE_INPUT_H

#include "cfltk/Fl_Input.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Fl_File_Input {
    Fl_Input input;
    Fl_Color errorcolor_;
    char ok_entry_;
    uchar down_box_;
    short buttons_[200];
    short pressed_;
} Fl_File_Input;

extern const Fl_WidgetOps fl_file_input_ops;

void Fl_File_Input_init(Fl_File_Input *self, int x, int y, int w, int h, const char *label);
Fl_File_Input *Fl_File_Input_new(int x, int y, int w, int h, const char *label);

void Fl_File_Input_draw(Fl_Widget *self_w);
int Fl_File_Input_handle(Fl_Widget *self_w, int event);

int Fl_File_Input_set_value(Fl_File_Input *self, const char *str, int len);
static inline int Fl_File_Input_set_value_str(Fl_File_Input *self, const char *str) {
    return Fl_File_Input_set_value(self, str, str ? (int)strlen(str) : 0);
}
static inline const char *Fl_File_Input_value(const Fl_File_Input *self) { return self->input.buffer; }

static inline uchar Fl_File_Input_down_box(const Fl_File_Input *self) { return self->down_box_; }
static inline void Fl_File_Input_set_down_box(Fl_File_Input *self, uchar b) { self->down_box_ = b; }
static inline Fl_Color Fl_File_Input_errorcolor(const Fl_File_Input *self) { return self->errorcolor_; }
static inline void Fl_File_Input_set_errorcolor(Fl_File_Input *self, Fl_Color c) { self->errorcolor_ = c; }

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_FILE_INPUT_H */
