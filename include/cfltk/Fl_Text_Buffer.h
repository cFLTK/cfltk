/*
 * cfltk - Fl_Text_Buffer.h
 *
 * C translation of FLTK 1.3 FL/Fl_Text_Buffer.H.
 *
 * Original class : Fl_Text_Buffer (plain data class, not a widget --
 *                   no Fl_Widget/Fl_Group embedding). Manages UTF-8
 *                   text storage for one or more Fl_Text_Display/
 *                   Fl_Text_Editor widgets via a gap buffer (a single
 *                   contiguous allocation with a movable empty "gap"
 *                   at the edit point, so repeated typing at one spot
 *                   doesn't repeatedly shift the whole buffer), plus
 *                   three named selections (primary/secondary/
 *                   highlight), a one-slot undo log, and two callback
 *                   lists (modify, predelete) that attached displays
 *                   use to stay in sync.
 * New C structure : struct Fl_Text_Buffer { ... every mFoo field from
 *                    upstream, renamed without the 'm' prefix ... };
 *                    struct Fl_Text_Selection { int start, end;
 *                    int selected; }. Plain heap-allocated structs,
 *                    no vtable of any kind -- this class has no virtual
 *                    functions upstream either.
 * Ownership       : owns its internal char buffer and undo log (both
 *                    process-wide static undo state, matching upstream's
 *                    own file-static undo variables -- see "Known
 *                    differences" below); does not own attached
 *                    displays or their callbacks, just the two
 *                    registration lists.
 * Known differences:
 *   - Undo is a single global slot shared by whichever Fl_Text_Buffer
 *     last modified text (static file-scope state in
 *     src/Fl_Text_Buffer.cxx), not a per-buffer undo stack. This is
 *     upstream's own design, not a cfltk simplification -- ported
 *     verbatim, including the surprising consequence that editing one
 *     buffer invalidates undo for whatever buffer was edited before it.
 *   - insertfile()/outputfile() (and therefore appendfile()/loadfile()/
 *     savefile()) are not ported -- thin wrappers over fopen()/fread()/
 *     fwrite() a caller can write directly against insert()/text_range().
 *     The CP1252-transcoding-on-load behavior they added (for files that
 *     turn out not to be valid UTF-8) is also not ported; feed insert()
 *     already-UTF-8 text.
 *   - Case-insensitive search (search_forward/backward with
 *     matchCase=0) only case-folds ASCII letters -- see fl_utf8.h.
 */
#ifndef CFLTK_FL_TEXT_BUFFER_H
#define CFLTK_FL_TEXT_BUFFER_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FL_TEXT_MAX_EXP_CHAR_LEN 20

typedef struct Fl_Text_Selection {
    int start;    /* byte offset to first selected character */
    int end;      /* byte offset one past the last selected character */
    int selected; /* boolean */
} Fl_Text_Selection;

int Fl_Text_Selection_includes(const Fl_Text_Selection *self, int pos);
/* Returns non-zero (and fills the start/end outputs) if selected. */
int Fl_Text_Selection_position(const Fl_Text_Selection *self, int *start, int *end);
static inline int Fl_Text_Selection_start(const Fl_Text_Selection *self) { return self->start; }
static inline int Fl_Text_Selection_end(const Fl_Text_Selection *self) { return self->end; }
static inline int Fl_Text_Selection_selected(const Fl_Text_Selection *self) { return self->selected; }

typedef void (*Fl_Text_Modify_Cb)(int pos, int nInserted, int nDeleted, int nRestyled, const char *deletedText, void *cbArg);
typedef void (*Fl_Text_Predelete_Cb)(int pos, int nDeleted, void *cbArg);

typedef struct Fl_Text_Buffer {
    Fl_Text_Selection primary;
    Fl_Text_Selection secondary;
    Fl_Text_Selection highlight;
    int length;   /* bytes of text currently in the buffer */
    char *buf;    /* allocated storage, length + (gap_end - gap_start) bytes */
    int gap_start; /* first byte of the gap */
    int gap_end;   /* first byte after the gap */
    int tab_dist;  /* hardware tab width, in characters */
    int n_modify_procs;
    Fl_Text_Modify_Cb *modify_procs;
    void **modify_cbargs;
    int n_predelete_procs;
    Fl_Text_Predelete_Cb *predelete_procs;
    void **predelete_cbargs;
    int cursor_pos_hint; /* where a modification suggests the cursor go next */
    char can_undo;
    int preferred_gap_size;
    int input_file_was_transcoded; /* always 0 -- see header note (no file I/O ported) */
} Fl_Text_Buffer;

Fl_Text_Buffer *Fl_Text_Buffer_new(int requested_size, int preferred_gap_size);
void Fl_Text_Buffer_free(Fl_Text_Buffer *self);

static inline int Fl_Text_Buffer_length(const Fl_Text_Buffer *self) { return self->length; }
/* Returns a newly malloc'd, nul-terminated copy of the whole buffer. */
char *Fl_Text_Buffer_text(const Fl_Text_Buffer *self);
void Fl_Text_Buffer_set_text(Fl_Text_Buffer *self, const char *text);
/* Returns a newly malloc'd, nul-terminated copy of [start,end). */
char *Fl_Text_Buffer_text_range(const Fl_Text_Buffer *self, int start, int end);

unsigned int Fl_Text_Buffer_char_at(const Fl_Text_Buffer *self, int pos);
char Fl_Text_Buffer_byte_at(const Fl_Text_Buffer *self, int pos);
const char *Fl_Text_Buffer_address(const Fl_Text_Buffer *self, int pos);

void Fl_Text_Buffer_insert(Fl_Text_Buffer *self, int pos, const char *text);
static inline void Fl_Text_Buffer_append(Fl_Text_Buffer *self, const char *t) { Fl_Text_Buffer_insert(self, Fl_Text_Buffer_length(self), t); }
void Fl_Text_Buffer_remove(Fl_Text_Buffer *self, int start, int end);
void Fl_Text_Buffer_replace(Fl_Text_Buffer *self, int start, int end, const char *text);
void Fl_Text_Buffer_copy(Fl_Text_Buffer *self, Fl_Text_Buffer *from_buf, int from_start, int from_end, int to_pos);

int Fl_Text_Buffer_undo(Fl_Text_Buffer *self, int *cursor_pos);
void Fl_Text_Buffer_set_can_undo(Fl_Text_Buffer *self, char flag);

static inline int Fl_Text_Buffer_tab_distance(const Fl_Text_Buffer *self) { return self->tab_dist; }
void Fl_Text_Buffer_set_tab_distance(Fl_Text_Buffer *self, int tab_dist);

void Fl_Text_Buffer_select(Fl_Text_Buffer *self, int start, int end);
static inline int Fl_Text_Buffer_selected(const Fl_Text_Buffer *self) { return self->primary.selected; }
void Fl_Text_Buffer_unselect(Fl_Text_Buffer *self);
int Fl_Text_Buffer_selection_position(Fl_Text_Buffer *self, int *start, int *end);
char *Fl_Text_Buffer_selection_text(Fl_Text_Buffer *self);
void Fl_Text_Buffer_remove_selection(Fl_Text_Buffer *self);
void Fl_Text_Buffer_replace_selection(Fl_Text_Buffer *self, const char *text);

void Fl_Text_Buffer_secondary_select(Fl_Text_Buffer *self, int start, int end);
static inline int Fl_Text_Buffer_secondary_selected(const Fl_Text_Buffer *self) { return self->secondary.selected; }
void Fl_Text_Buffer_secondary_unselect(Fl_Text_Buffer *self);
int Fl_Text_Buffer_secondary_selection_position(Fl_Text_Buffer *self, int *start, int *end);
char *Fl_Text_Buffer_secondary_selection_text(Fl_Text_Buffer *self);
void Fl_Text_Buffer_remove_secondary_selection(Fl_Text_Buffer *self);
void Fl_Text_Buffer_replace_secondary_selection(Fl_Text_Buffer *self, const char *text);

void Fl_Text_Buffer_highlight(Fl_Text_Buffer *self, int start, int end);
static inline int Fl_Text_Buffer_highlighted(const Fl_Text_Buffer *self) { return self->highlight.selected; }
void Fl_Text_Buffer_unhighlight(Fl_Text_Buffer *self);
int Fl_Text_Buffer_highlight_position(Fl_Text_Buffer *self, int *start, int *end);
char *Fl_Text_Buffer_highlight_text(Fl_Text_Buffer *self);

void Fl_Text_Buffer_add_modify_callback(Fl_Text_Buffer *self, Fl_Text_Modify_Cb cb, void *cbarg);
void Fl_Text_Buffer_remove_modify_callback(Fl_Text_Buffer *self, Fl_Text_Modify_Cb cb, void *cbarg);
/* Internal (also used by Fl_Text_Buffer.c itself after a real edit);
 * the public zero-arg call_modify_callbacks() below is the (0,0,0,0,NULL)
 * "just re-sync everything" case upstream exposes as an overload. */
void Fl_Text_Buffer_call_modify_callbacks_ex(const Fl_Text_Buffer *self, int pos, int nDeleted, int nInserted, int nRestyled, const char *deletedText);
static inline void Fl_Text_Buffer_call_modify_callbacks(const Fl_Text_Buffer *self) {
    Fl_Text_Buffer_call_modify_callbacks_ex(self, 0, 0, 0, 0, NULL);
}

void Fl_Text_Buffer_add_predelete_callback(Fl_Text_Buffer *self, Fl_Text_Predelete_Cb cb, void *cbarg);
void Fl_Text_Buffer_remove_predelete_callback(Fl_Text_Buffer *self, Fl_Text_Predelete_Cb cb, void *cbarg);
void Fl_Text_Buffer_call_predelete_callbacks_ex(const Fl_Text_Buffer *self, int pos, int nDeleted);
static inline void Fl_Text_Buffer_call_predelete_callbacks(const Fl_Text_Buffer *self) {
    Fl_Text_Buffer_call_predelete_callbacks_ex(self, 0, 0);
}

char *Fl_Text_Buffer_line_text(const Fl_Text_Buffer *self, int pos);
int Fl_Text_Buffer_line_start(const Fl_Text_Buffer *self, int pos);
int Fl_Text_Buffer_line_end(const Fl_Text_Buffer *self, int pos);
int Fl_Text_Buffer_word_start(const Fl_Text_Buffer *self, int pos);
int Fl_Text_Buffer_word_end(const Fl_Text_Buffer *self, int pos);

int Fl_Text_Buffer_count_displayed_characters(const Fl_Text_Buffer *self, int line_start_pos, int target_pos);
int Fl_Text_Buffer_skip_displayed_characters(Fl_Text_Buffer *self, int line_start_pos, int n_chars);
int Fl_Text_Buffer_count_lines(const Fl_Text_Buffer *self, int start_pos, int end_pos);
int Fl_Text_Buffer_skip_lines(Fl_Text_Buffer *self, int start_pos, int n_lines);
int Fl_Text_Buffer_rewind_lines(Fl_Text_Buffer *self, int start_pos, int n_lines);

int Fl_Text_Buffer_findchar_forward(const Fl_Text_Buffer *self, int start_pos, unsigned search_char, int *found_pos);
int Fl_Text_Buffer_findchar_backward(const Fl_Text_Buffer *self, int start_pos, unsigned search_char, int *found_pos);
int Fl_Text_Buffer_search_forward(const Fl_Text_Buffer *self, int start_pos, const char *search_string, int *found_pos, int match_case);
int Fl_Text_Buffer_search_backward(const Fl_Text_Buffer *self, int start_pos, const char *search_string, int *found_pos, int match_case);

static inline const Fl_Text_Selection *Fl_Text_Buffer_primary_selection(const Fl_Text_Buffer *self) { return &self->primary; }
static inline Fl_Text_Selection *Fl_Text_Buffer_primary_selection_mut(Fl_Text_Buffer *self) { return &self->primary; }
static inline const Fl_Text_Selection *Fl_Text_Buffer_secondary_selection(const Fl_Text_Buffer *self) { return &self->secondary; }
static inline const Fl_Text_Selection *Fl_Text_Buffer_highlight_selection(const Fl_Text_Buffer *self) { return &self->highlight; }

int Fl_Text_Buffer_prev_char(const Fl_Text_Buffer *self, int ix);
int Fl_Text_Buffer_prev_char_clipped(const Fl_Text_Buffer *self, int ix);
int Fl_Text_Buffer_next_char(const Fl_Text_Buffer *self, int ix);
static inline int Fl_Text_Buffer_next_char_clipped(const Fl_Text_Buffer *self, int ix) { return Fl_Text_Buffer_next_char(self, ix); }
int Fl_Text_Buffer_utf8_align(const Fl_Text_Buffer *self, int pos);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_TEXT_BUFFER_H */
