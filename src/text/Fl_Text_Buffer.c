/*
 * cfltk - Fl_Text_Buffer.c
 * See include/cfltk/Fl_Text_Buffer.h for the class-conversion notes.
 * Translated from src/Fl_Text_Buffer.cxx.
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cfltk/Fl_Text_Buffer.h"
#include "cfltk/fl_utf8.h"

static int imax(int a, int b) { return a >= b ? a : b; }
static int imin(int a, int b) { return a <= b ? a : b; }

/* -------------------------------------------------------------------
 * Fl_Text_Selection
 * ---------------------------------------------------------------- */

static void selection_set(Fl_Text_Selection *self, int startpos, int endpos) {
    self->selected = (startpos != endpos);
    self->start = imin(startpos, endpos);
    self->end = imax(startpos, endpos);
}

static void selection_update(Fl_Text_Selection *self, int pos, int nDeleted, int nInserted) {
    if (!self->selected || pos > self->end) return;
    if (pos + nDeleted <= self->start) {
        self->start += nInserted - nDeleted;
        self->end += nInserted - nDeleted;
    } else if (pos <= self->start && pos + nDeleted >= self->end) {
        self->start = pos;
        self->end = pos;
        self->selected = 0;
    } else if (pos <= self->start && pos + nDeleted < self->end) {
        self->start = pos;
        self->end = nInserted + self->end - nDeleted;
    } else if (pos < self->end) {
        self->end += nInserted - nDeleted;
        if (self->end <= self->start) self->selected = 0;
    }
}

int Fl_Text_Selection_includes(const Fl_Text_Selection *self, int pos) {
    return self->selected && pos >= self->start && pos < self->end;
}

int Fl_Text_Selection_position(const Fl_Text_Selection *self, int *start, int *end) {
    if (!self->selected) return 0;
    *start = self->start;
    *end = self->end;
    return 1;
}

/* -------------------------------------------------------------------
 * Global undo state -- a single slot shared by whichever buffer last
 * modified text. This is upstream's own design (file-static in
 * Fl_Text_Buffer.cxx), not a cfltk simplification -- see header note.
 * ---------------------------------------------------------------- */

static char *undobuffer = NULL;
static int undobufferlength = 0;
static Fl_Text_Buffer *undowidget = NULL;
static int undoat = 0;     /* points after insertion */
static int undocut = 0;    /* number of characters deleted there */
static int undoinsert = 0; /* number of characters inserted */
static int undoyankcut = 0;

static void undobuffersize(int n) {
    if (n > undobufferlength) {
        if (undobuffer) {
            do { undobufferlength *= 2; } while (undobufferlength < n);
            undobuffer = (char *)realloc(undobuffer, (size_t)undobufferlength);
        } else {
            undobufferlength = n + 9;
            undobuffer = (char *)malloc((size_t)undobufferlength);
        }
    }
}

/* -------------------------------------------------------------------
 * Construction
 * ---------------------------------------------------------------- */

Fl_Text_Buffer *Fl_Text_Buffer_new(int requested_size, int preferred_gap_size) {
    Fl_Text_Buffer *self = (Fl_Text_Buffer *)malloc(sizeof(Fl_Text_Buffer));
    self->length = 0;
    self->preferred_gap_size = preferred_gap_size;
    self->buf = (char *)malloc((size_t)(requested_size + self->preferred_gap_size));
    self->gap_start = 0;
    self->gap_end = requested_size + self->preferred_gap_size;
    self->tab_dist = 8;
    self->primary.selected = 0;
    self->primary.start = self->primary.end = 0;
    self->secondary.selected = 0;
    self->secondary.start = self->secondary.end = 0;
    self->highlight.selected = 0;
    self->highlight.start = self->highlight.end = 0;
    self->modify_procs = NULL;
    self->modify_cbargs = NULL;
    self->n_modify_procs = 0;
    self->n_predelete_procs = 0;
    self->predelete_procs = NULL;
    self->predelete_cbargs = NULL;
    self->cursor_pos_hint = 0;
    self->can_undo = 1;
    self->input_file_was_transcoded = 0;
    return self;
}

void Fl_Text_Buffer_free(Fl_Text_Buffer *self) {
    if (!self) return;
    free(self->buf);
    free(self->modify_procs);
    free(self->modify_cbargs);
    free(self->predelete_procs);
    free(self->predelete_cbargs);
    if (undowidget == self) undowidget = NULL;
    free(self);
}

/* -------------------------------------------------------------------
 * Text access
 * ---------------------------------------------------------------- */

const char *Fl_Text_Buffer_address(const Fl_Text_Buffer *self, int pos) {
    return (pos < self->gap_start) ? self->buf + pos : self->buf + pos + self->gap_end - self->gap_start;
}

char *Fl_Text_Buffer_text(const Fl_Text_Buffer *self) {
    char *t = (char *)malloc((size_t)self->length + 1);
    memcpy(t, self->buf, (size_t)self->gap_start);
    memcpy(t + self->gap_start, self->buf + self->gap_end, (size_t)(self->length - self->gap_start));
    t[self->length] = '\0';
    return t;
}

static void call_predelete_callbacks(const Fl_Text_Buffer *self, int pos, int nDeleted);
static void call_modify_callbacks(const Fl_Text_Buffer *self, int pos, int nDeleted, int nInserted, int nRestyled, const char *deletedText);
static void update_selections(Fl_Text_Buffer *self, int pos, int nDeleted, int nInserted);

void Fl_Text_Buffer_set_text(Fl_Text_Buffer *self, const char *t) {
    char *deletedText;
    int deletedLength, insertedLength;

    if (!t) t = "";

    call_predelete_callbacks(self, 0, self->length);

    deletedText = Fl_Text_Buffer_text(self);
    deletedLength = self->length;
    free(self->buf);

    insertedLength = (int)strlen(t);
    self->buf = (char *)malloc((size_t)(insertedLength + self->preferred_gap_size));
    self->length = insertedLength;
    self->gap_start = insertedLength;
    self->gap_end = self->gap_start + self->preferred_gap_size;
    memcpy(self->buf, t, (size_t)insertedLength);

    update_selections(self, 0, deletedLength, 0);
    call_modify_callbacks(self, 0, deletedLength, insertedLength, 0, deletedText);
    free(deletedText);
}

char *Fl_Text_Buffer_text_range(const Fl_Text_Buffer *self, int start, int end) {
    char *s;
    int copiedLength;

    if (start < 0 || start > self->length) {
        s = (char *)malloc(1);
        s[0] = '\0';
        return s;
    }
    if (end < start) { int tmp = start; start = end; end = tmp; }
    if (end > self->length) end = self->length;
    copiedLength = end - start;
    s = (char *)malloc((size_t)copiedLength + 1);

    if (end <= self->gap_start) {
        memcpy(s, self->buf + start, (size_t)copiedLength);
    } else if (start >= self->gap_start) {
        memcpy(s, self->buf + start + (self->gap_end - self->gap_start), (size_t)copiedLength);
    } else {
        int part1Length = self->gap_start - start;
        memcpy(s, self->buf + start, (size_t)part1Length);
        memcpy(s + part1Length, self->buf + self->gap_end, (size_t)(copiedLength - part1Length));
    }
    s[copiedLength] = '\0';
    return s;
}

unsigned int Fl_Text_Buffer_char_at(const Fl_Text_Buffer *self, int pos) {
    const char *src;
    if (pos < 0 || pos >= self->length) return '\0';
    src = Fl_Text_Buffer_address(self, pos);
    return fl_utf8decode(src, 0, 0);
}

char Fl_Text_Buffer_byte_at(const Fl_Text_Buffer *self, int pos) {
    if (pos < 0 || pos >= self->length) return '\0';
    return *Fl_Text_Buffer_address(self, pos);
}

/* -------------------------------------------------------------------
 * Gap management (internal)
 * ---------------------------------------------------------------- */

static void move_gap(Fl_Text_Buffer *self, int pos) {
    int gapLen = self->gap_end - self->gap_start;
    if (pos > self->gap_start) memmove(&self->buf[self->gap_start], &self->buf[self->gap_end], (size_t)(pos - self->gap_start));
    else memmove(&self->buf[pos + gapLen], &self->buf[pos], (size_t)(self->gap_start - pos));
    self->gap_end += pos - self->gap_start;
    self->gap_start += pos - self->gap_start;
}

static void reallocate_with_gap(Fl_Text_Buffer *self, int newGapStart, int newGapLen) {
    char *newBuf = (char *)malloc((size_t)(self->length + newGapLen));
    int newGapEnd = newGapStart + newGapLen;

    if (newGapStart <= self->gap_start) {
        memcpy(newBuf, self->buf, (size_t)newGapStart);
        memcpy(&newBuf[newGapEnd], &self->buf[newGapStart], (size_t)(self->gap_start - newGapStart));
        memcpy(&newBuf[newGapEnd + self->gap_start - newGapStart], &self->buf[self->gap_end], (size_t)(self->length - self->gap_start));
    } else {
        memcpy(newBuf, self->buf, (size_t)self->gap_start);
        memcpy(&newBuf[self->gap_start], &self->buf[self->gap_end], (size_t)(newGapStart - self->gap_start));
        memcpy(&newBuf[newGapEnd], &self->buf[self->gap_end + newGapStart - self->gap_start], (size_t)(self->length - newGapStart));
    }
    free(self->buf);
    self->buf = newBuf;
    self->gap_start = newGapStart;
    self->gap_end = newGapEnd;
}

static int insert_(Fl_Text_Buffer *self, int pos, const char *text) {
    int insertedLength;
    if (!text || !*text) return 0;
    insertedLength = (int)strlen(text);

    if (insertedLength > self->gap_end - self->gap_start) reallocate_with_gap(self, pos, insertedLength + self->preferred_gap_size);
    else if (pos != self->gap_start) move_gap(self, pos);

    memcpy(&self->buf[pos], text, (size_t)insertedLength);
    self->gap_start += insertedLength;
    self->length += insertedLength;
    update_selections(self, pos, 0, insertedLength);

    if (self->can_undo) {
        if (undowidget == self && undoat == pos && undoinsert) {
            undoinsert += insertedLength;
        } else {
            undoinsert = insertedLength;
            undoyankcut = (undoat == pos) ? undocut : 0;
        }
        undoat = pos + insertedLength;
        undocut = 0;
        undowidget = self;
    }
    return insertedLength;
}

static void remove_(Fl_Text_Buffer *self, int start, int end) {
    if (self->can_undo) {
        if (undowidget == self && undoat == end && undocut) {
            undobuffersize(undocut + end - start + 1);
            memmove(undobuffer + end - start, undobuffer, (size_t)undocut);
            undocut += end - start;
        } else {
            undocut = end - start;
            undobuffersize(undocut);
        }
        undoat = start;
        undoinsert = 0;
        undoyankcut = 0;
        undowidget = self;
    }

    if (start > self->gap_start) {
        if (self->can_undo) memcpy(undobuffer, self->buf + (self->gap_end - self->gap_start) + start, (size_t)(end - start));
        move_gap(self, start);
    } else if (end < self->gap_start) {
        if (self->can_undo) memcpy(undobuffer, self->buf + start, (size_t)(end - start));
        move_gap(self, end);
    } else {
        int prelen = self->gap_start - start;
        if (self->can_undo) {
            memcpy(undobuffer, self->buf + start, (size_t)prelen);
            memcpy(undobuffer + prelen, self->buf + self->gap_end, (size_t)(end - start - prelen));
        }
    }

    self->gap_end += end - self->gap_start;
    self->gap_start -= self->gap_start - start;
    self->length -= end - start;
    update_selections(self, start, end - start, 0);
}

/* -------------------------------------------------------------------
 * Public modification API
 * ---------------------------------------------------------------- */

void Fl_Text_Buffer_insert(Fl_Text_Buffer *self, int pos, const char *text) {
    int nInserted;
    if (!text || !*text) return;
    if (pos > self->length) pos = self->length;
    if (pos < 0) pos = 0;

    call_predelete_callbacks(self, pos, 0);
    nInserted = insert_(self, pos, text);
    self->cursor_pos_hint = pos + nInserted;
    call_modify_callbacks(self, pos, 0, nInserted, 0, NULL);
}

void Fl_Text_Buffer_replace(Fl_Text_Buffer *self, int start, int end, const char *text) {
    char *deletedText;
    int nInserted;
    if (!text) return;
    if (start < 0) start = 0;
    if (end > self->length) end = self->length;

    call_predelete_callbacks(self, start, end - start);
    deletedText = Fl_Text_Buffer_text_range(self, start, end);
    remove_(self, start, end);
    nInserted = insert_(self, start, text);
    self->cursor_pos_hint = start + nInserted;
    call_modify_callbacks(self, start, end - start, nInserted, 0, deletedText);
    free(deletedText);
}

void Fl_Text_Buffer_remove(Fl_Text_Buffer *self, int start, int end) {
    char *deletedText;
    if (start > end) { int tmp = start; start = end; end = tmp; }
    if (start > self->length) start = self->length;
    if (start < 0) start = 0;
    if (end > self->length) end = self->length;
    if (end < 0) end = 0;
    if (start == end) return;

    call_predelete_callbacks(self, start, end - start);
    deletedText = Fl_Text_Buffer_text_range(self, start, end);
    remove_(self, start, end);
    self->cursor_pos_hint = start;
    call_modify_callbacks(self, start, end - start, 0, 0, deletedText);
    free(deletedText);
}

void Fl_Text_Buffer_copy(Fl_Text_Buffer *self, Fl_Text_Buffer *fromBuf, int fromStart, int fromEnd, int toPos) {
    int copiedLength = fromEnd - fromStart;

    if (copiedLength > self->gap_end - self->gap_start) reallocate_with_gap(self, toPos, copiedLength + self->preferred_gap_size);
    else if (toPos != self->gap_start) move_gap(self, toPos);

    if (fromEnd <= fromBuf->gap_start) {
        memcpy(&self->buf[toPos], &fromBuf->buf[fromStart], (size_t)copiedLength);
    } else if (fromStart >= fromBuf->gap_start) {
        memcpy(&self->buf[toPos], &fromBuf->buf[fromStart + (fromBuf->gap_end - fromBuf->gap_start)], (size_t)copiedLength);
    } else {
        int part1Length = fromBuf->gap_start - fromStart;
        memcpy(&self->buf[toPos], &fromBuf->buf[fromStart], (size_t)part1Length);
        memcpy(&self->buf[toPos + part1Length], &fromBuf->buf[fromBuf->gap_end], (size_t)(copiedLength - part1Length));
    }
    self->gap_start += copiedLength;
    self->length += copiedLength;
    update_selections(self, toPos, 0, copiedLength);
}

/* -------------------------------------------------------------------
 * Undo
 * ---------------------------------------------------------------- */

int Fl_Text_Buffer_undo(Fl_Text_Buffer *self, int *cursorPos) {
    int ilen, xlen, b;

    if (undowidget != self || (!undocut && !undoinsert && !self->can_undo)) return 0;

    ilen = undocut;
    xlen = undoinsert;
    b = undoat - xlen;

    if (xlen && undoyankcut && !ilen) ilen = undoyankcut;

    if (xlen && ilen) {
        char *tmp;
        undobuffersize(ilen + 1);
        undobuffer[ilen] = 0;
        tmp = (char *)malloc((size_t)ilen + 1);
        memcpy(tmp, undobuffer, (size_t)ilen + 1);
        Fl_Text_Buffer_replace(self, b, undoat, tmp);
        if (cursorPos) *cursorPos = self->cursor_pos_hint;
        free(tmp);
    } else if (xlen) {
        Fl_Text_Buffer_remove(self, b, undoat);
        if (cursorPos) *cursorPos = self->cursor_pos_hint;
    } else if (ilen) {
        undobuffersize(ilen + 1);
        undobuffer[ilen] = 0;
        Fl_Text_Buffer_insert(self, undoat, undobuffer);
        if (cursorPos) *cursorPos = self->cursor_pos_hint;
        undoyankcut = 0;
    }
    return 1;
}

void Fl_Text_Buffer_set_can_undo(Fl_Text_Buffer *self, char flag) {
    self->can_undo = flag;
    if (!self->can_undo && undowidget == self) undowidget = NULL;
}

/* -------------------------------------------------------------------
 * Tab distance
 * ---------------------------------------------------------------- */

void Fl_Text_Buffer_set_tab_distance(Fl_Text_Buffer *self, int tabDist) {
    char *deletedText;
    call_predelete_callbacks(self, 0, self->length);
    self->tab_dist = tabDist;
    deletedText = Fl_Text_Buffer_text(self);
    call_modify_callbacks(self, 0, self->length, self->length, 0, deletedText);
    free(deletedText);
}

/* -------------------------------------------------------------------
 * Selections (shared helper for primary/secondary/highlight)
 * ---------------------------------------------------------------- */

static void redisplay_selection(const Fl_Text_Buffer *self, const Fl_Text_Selection *oldSel, const Fl_Text_Selection *newSel) {
    int oldStart = oldSel->start, newStart = newSel->start;
    int oldEnd = oldSel->end, newEnd = newSel->end;
    int ch1Start, ch1End, ch2Start, ch2End;

    if (!oldSel->selected && !newSel->selected) return;
    if (!oldSel->selected) { call_modify_callbacks(self, newStart, 0, 0, newEnd - newStart, NULL); return; }
    if (!newSel->selected) { call_modify_callbacks(self, oldStart, 0, 0, oldEnd - oldStart, NULL); return; }
    if (oldEnd < newStart || newEnd < oldStart) {
        call_modify_callbacks(self, oldStart, 0, 0, oldEnd - oldStart, NULL);
        call_modify_callbacks(self, newStart, 0, 0, newEnd - newStart, NULL);
        return;
    }
    ch1Start = imin(oldStart, newStart);
    ch2End = imax(oldEnd, newEnd);
    ch1End = imax(oldStart, newStart);
    ch2Start = imin(oldEnd, newEnd);
    if (ch1Start != ch1End) call_modify_callbacks(self, ch1Start, 0, 0, ch1End - ch1Start, NULL);
    if (ch2Start != ch2End) call_modify_callbacks(self, ch2Start, 0, 0, ch2End - ch2Start, NULL);
}

static char *selection_text_(const Fl_Text_Buffer *self, const Fl_Text_Selection *sel) {
    int start, end;
    if (!Fl_Text_Selection_position(sel, &start, &end)) {
        char *s = (char *)malloc(1);
        *s = '\0';
        return s;
    }
    return Fl_Text_Buffer_text_range(self, start, end);
}

static void remove_selection_(Fl_Text_Buffer *self, Fl_Text_Selection *sel) {
    int start, end;
    if (!Fl_Text_Selection_position(sel, &start, &end)) return;
    Fl_Text_Buffer_remove(self, start, end);
}

static void replace_selection_(Fl_Text_Buffer *self, Fl_Text_Selection *sel, const char *text) {
    Fl_Text_Selection oldSelection = *sel;
    int start, end;
    if (!Fl_Text_Selection_position(sel, &start, &end)) return;
    Fl_Text_Buffer_replace(self, start, end, text);
    sel->selected = 0;
    redisplay_selection(self, &oldSelection, sel);
}

void Fl_Text_Buffer_select(Fl_Text_Buffer *self, int start, int end) {
    Fl_Text_Selection oldSelection = self->primary;
    selection_set(&self->primary, start, end);
    redisplay_selection(self, &oldSelection, &self->primary);
}

void Fl_Text_Buffer_unselect(Fl_Text_Buffer *self) {
    Fl_Text_Selection oldSelection = self->primary;
    self->primary.selected = 0;
    redisplay_selection(self, &oldSelection, &self->primary);
}

int Fl_Text_Buffer_selection_position(Fl_Text_Buffer *self, int *start, int *end) { return Fl_Text_Selection_position(&self->primary, start, end); }
char *Fl_Text_Buffer_selection_text(Fl_Text_Buffer *self) { return selection_text_(self, &self->primary); }
void Fl_Text_Buffer_remove_selection(Fl_Text_Buffer *self) { remove_selection_(self, &self->primary); }
void Fl_Text_Buffer_replace_selection(Fl_Text_Buffer *self, const char *text) { replace_selection_(self, &self->primary, text); }

void Fl_Text_Buffer_secondary_select(Fl_Text_Buffer *self, int start, int end) {
    Fl_Text_Selection oldSelection = self->secondary;
    selection_set(&self->secondary, start, end);
    redisplay_selection(self, &oldSelection, &self->secondary);
}

void Fl_Text_Buffer_secondary_unselect(Fl_Text_Buffer *self) {
    Fl_Text_Selection oldSelection = self->secondary;
    self->secondary.selected = 0;
    redisplay_selection(self, &oldSelection, &self->secondary);
}

int Fl_Text_Buffer_secondary_selection_position(Fl_Text_Buffer *self, int *start, int *end) { return Fl_Text_Selection_position(&self->secondary, start, end); }
char *Fl_Text_Buffer_secondary_selection_text(Fl_Text_Buffer *self) { return selection_text_(self, &self->secondary); }
void Fl_Text_Buffer_remove_secondary_selection(Fl_Text_Buffer *self) { remove_selection_(self, &self->secondary); }
void Fl_Text_Buffer_replace_secondary_selection(Fl_Text_Buffer *self, const char *text) { replace_selection_(self, &self->secondary, text); }

void Fl_Text_Buffer_highlight(Fl_Text_Buffer *self, int start, int end) {
    Fl_Text_Selection oldSelection = self->highlight;
    selection_set(&self->highlight, start, end);
    redisplay_selection(self, &oldSelection, &self->highlight);
}

void Fl_Text_Buffer_unhighlight(Fl_Text_Buffer *self) {
    Fl_Text_Selection oldSelection = self->highlight;
    self->highlight.selected = 0;
    redisplay_selection(self, &oldSelection, &self->highlight);
}

int Fl_Text_Buffer_highlight_position(Fl_Text_Buffer *self, int *start, int *end) { return Fl_Text_Selection_position(&self->highlight, start, end); }
char *Fl_Text_Buffer_highlight_text(Fl_Text_Buffer *self) { return selection_text_(self, &self->highlight); }

/* -------------------------------------------------------------------
 * Callback lists
 * ---------------------------------------------------------------- */

void Fl_Text_Buffer_add_modify_callback(Fl_Text_Buffer *self, Fl_Text_Modify_Cb cb, void *cbArg) {
    Fl_Text_Modify_Cb *newProcs = (Fl_Text_Modify_Cb *)malloc(sizeof(Fl_Text_Modify_Cb) * (size_t)(self->n_modify_procs + 1));
    void **newArgs = (void **)malloc(sizeof(void *) * (size_t)(self->n_modify_procs + 1));
    int i;
    for (i = 0; i < self->n_modify_procs; i++) {
        newProcs[i + 1] = self->modify_procs[i];
        newArgs[i + 1] = self->modify_cbargs[i];
    }
    free(self->modify_procs);
    free(self->modify_cbargs);
    newProcs[0] = cb;
    newArgs[0] = cbArg;
    self->n_modify_procs++;
    self->modify_procs = newProcs;
    self->modify_cbargs = newArgs;
}

void Fl_Text_Buffer_remove_modify_callback(Fl_Text_Buffer *self, Fl_Text_Modify_Cb cb, void *cbArg) {
    int i, toRemove = -1;
    Fl_Text_Modify_Cb *newProcs;
    void **newArgs;

    for (i = 0; i < self->n_modify_procs; i++) {
        if (self->modify_procs[i] == cb && self->modify_cbargs[i] == cbArg) { toRemove = i; break; }
    }
    if (toRemove == -1) {
        fprintf(stderr, "Fl_Text_Buffer_remove_modify_callback(): can't find modify CB to remove\n");
        return;
    }
    self->n_modify_procs--;
    if (self->n_modify_procs == 0) {
        free(self->modify_procs); self->modify_procs = NULL;
        free(self->modify_cbargs); self->modify_cbargs = NULL;
        return;
    }
    newProcs = (Fl_Text_Modify_Cb *)malloc(sizeof(Fl_Text_Modify_Cb) * (size_t)self->n_modify_procs);
    newArgs = (void **)malloc(sizeof(void *) * (size_t)self->n_modify_procs);
    for (i = 0; i < toRemove; i++) { newProcs[i] = self->modify_procs[i]; newArgs[i] = self->modify_cbargs[i]; }
    for (; i < self->n_modify_procs; i++) { newProcs[i] = self->modify_procs[i + 1]; newArgs[i] = self->modify_cbargs[i + 1]; }
    free(self->modify_procs);
    free(self->modify_cbargs);
    self->modify_procs = newProcs;
    self->modify_cbargs = newArgs;
}

void Fl_Text_Buffer_add_predelete_callback(Fl_Text_Buffer *self, Fl_Text_Predelete_Cb cb, void *cbArg) {
    Fl_Text_Predelete_Cb *newProcs = (Fl_Text_Predelete_Cb *)malloc(sizeof(Fl_Text_Predelete_Cb) * (size_t)(self->n_predelete_procs + 1));
    void **newArgs = (void **)malloc(sizeof(void *) * (size_t)(self->n_predelete_procs + 1));
    int i;
    for (i = 0; i < self->n_predelete_procs; i++) {
        newProcs[i + 1] = self->predelete_procs[i];
        newArgs[i + 1] = self->predelete_cbargs[i];
    }
    free(self->predelete_procs);
    free(self->predelete_cbargs);
    newProcs[0] = cb;
    newArgs[0] = cbArg;
    self->n_predelete_procs++;
    self->predelete_procs = newProcs;
    self->predelete_cbargs = newArgs;
}

void Fl_Text_Buffer_remove_predelete_callback(Fl_Text_Buffer *self, Fl_Text_Predelete_Cb cb, void *cbArg) {
    int i, toRemove = -1;
    Fl_Text_Predelete_Cb *newProcs;
    void **newArgs;

    for (i = 0; i < self->n_predelete_procs; i++) {
        if (self->predelete_procs[i] == cb && self->predelete_cbargs[i] == cbArg) { toRemove = i; break; }
    }
    if (toRemove == -1) {
        fprintf(stderr, "Fl_Text_Buffer_remove_predelete_callback(): can't find pre-delete CB to remove\n");
        return;
    }
    self->n_predelete_procs--;
    if (self->n_predelete_procs == 0) {
        free(self->predelete_procs); self->predelete_procs = NULL;
        free(self->predelete_cbargs); self->predelete_cbargs = NULL;
        return;
    }
    newProcs = (Fl_Text_Predelete_Cb *)malloc(sizeof(Fl_Text_Predelete_Cb) * (size_t)self->n_predelete_procs);
    newArgs = (void **)malloc(sizeof(void *) * (size_t)self->n_predelete_procs);
    for (i = 0; i < toRemove; i++) { newProcs[i] = self->predelete_procs[i]; newArgs[i] = self->predelete_cbargs[i]; }
    for (; i < self->n_predelete_procs; i++) { newProcs[i] = self->predelete_procs[i + 1]; newArgs[i] = self->predelete_cbargs[i + 1]; }
    free(self->predelete_procs);
    free(self->predelete_cbargs);
    self->predelete_procs = newProcs;
    self->predelete_cbargs = newArgs;
}

void Fl_Text_Buffer_call_modify_callbacks_ex(const Fl_Text_Buffer *self, int pos, int nDeleted, int nInserted, int nRestyled, const char *deletedText) {
    call_modify_callbacks(self, pos, nDeleted, nInserted, nRestyled, deletedText);
}
void Fl_Text_Buffer_call_predelete_callbacks_ex(const Fl_Text_Buffer *self, int pos, int nDeleted) {
    call_predelete_callbacks(self, pos, nDeleted);
}

static void call_modify_callbacks(const Fl_Text_Buffer *self, int pos, int nDeleted, int nInserted, int nRestyled, const char *deletedText) {
    int i;
    for (i = 0; i < self->n_modify_procs; i++) (*self->modify_procs[i])(pos, nInserted, nDeleted, nRestyled, deletedText, self->modify_cbargs[i]);
}

static void call_predelete_callbacks(const Fl_Text_Buffer *self, int pos, int nDeleted) {
    int i;
    for (i = 0; i < self->n_predelete_procs; i++) (*self->predelete_procs[i])(pos, nDeleted, self->predelete_cbargs[i]);
}

static void update_selections(Fl_Text_Buffer *self, int pos, int nDeleted, int nInserted) {
    selection_update(&self->primary, pos, nDeleted, nInserted);
    selection_update(&self->secondary, pos, nDeleted, nInserted);
    selection_update(&self->highlight, pos, nDeleted, nInserted);
}

/* -------------------------------------------------------------------
 * Line/word queries
 * ---------------------------------------------------------------- */

char *Fl_Text_Buffer_line_text(const Fl_Text_Buffer *self, int pos) {
    return Fl_Text_Buffer_text_range(self, Fl_Text_Buffer_line_start(self, pos), Fl_Text_Buffer_line_end(self, pos));
}

int Fl_Text_Buffer_line_start(const Fl_Text_Buffer *self, int pos) {
    int foundPos;
    if (!Fl_Text_Buffer_findchar_backward(self, pos, '\n', &foundPos)) return 0;
    return foundPos + 1;
}

int Fl_Text_Buffer_line_end(const Fl_Text_Buffer *self, int pos) {
    int foundPos;
    if (!Fl_Text_Buffer_findchar_forward(self, pos, '\n', &foundPos)) return self->length;
    return foundPos;
}

int Fl_Text_Buffer_word_start(const Fl_Text_Buffer *self, int pos) {
    while (pos > 0 && (isalnum((int)Fl_Text_Buffer_char_at(self, pos)) || Fl_Text_Buffer_char_at(self, pos) == '_')) {
        pos = Fl_Text_Buffer_prev_char(self, pos);
    }
    if (!(isalnum((int)Fl_Text_Buffer_char_at(self, pos)) || Fl_Text_Buffer_char_at(self, pos) == '_')) pos = Fl_Text_Buffer_next_char(self, pos);
    return pos;
}

int Fl_Text_Buffer_word_end(const Fl_Text_Buffer *self, int pos) {
    while (pos < self->length && (isalnum((int)Fl_Text_Buffer_char_at(self, pos)) || Fl_Text_Buffer_char_at(self, pos) == '_')) {
        pos = Fl_Text_Buffer_next_char(self, pos);
    }
    return pos;
}

int Fl_Text_Buffer_count_displayed_characters(const Fl_Text_Buffer *self, int lineStartPos, int targetPos) {
    int charCount = 0, pos = lineStartPos;
    while (pos < targetPos) { pos = Fl_Text_Buffer_next_char(self, pos); charCount++; }
    return charCount;
}

int Fl_Text_Buffer_skip_displayed_characters(Fl_Text_Buffer *self, int lineStartPos, int nChars) {
    int pos = lineStartPos, charCount;
    for (charCount = 0; charCount < nChars && pos < self->length; charCount++) {
        unsigned int c = Fl_Text_Buffer_char_at(self, pos);
        if (c == '\n') return pos;
        pos = Fl_Text_Buffer_next_char(self, pos);
    }
    return pos;
}

int Fl_Text_Buffer_count_lines(const Fl_Text_Buffer *self, int startPos, int endPos) {
    int gapLen = self->gap_end - self->gap_start;
    int lineCount = 0, pos = startPos;
    while (pos < self->gap_start) {
        if (pos == endPos) return lineCount;
        if (self->buf[pos++] == '\n') lineCount++;
    }
    while (pos < self->length) {
        if (pos == endPos) return lineCount;
        if (self->buf[pos++ + gapLen] == '\n') lineCount++;
    }
    return lineCount;
}

int Fl_Text_Buffer_skip_lines(Fl_Text_Buffer *self, int startPos, int nLines) {
    int gapLen, pos, lineCount;
    if (nLines == 0) return startPos;
    gapLen = self->gap_end - self->gap_start;
    pos = startPos;
    lineCount = 0;
    while (pos < self->gap_start) {
        if (self->buf[pos++] == '\n') {
            lineCount++;
            if (lineCount == nLines) return pos;
        }
    }
    while (pos < self->length) {
        if (self->buf[pos++ + gapLen] == '\n') {
            lineCount++;
            if (lineCount >= nLines) return pos;
        }
    }
    return pos;
}

int Fl_Text_Buffer_rewind_lines(Fl_Text_Buffer *self, int startPos, int nLines) {
    int pos = startPos - 1;
    int gapLen, lineCount;
    if (pos <= 0) return 0;
    gapLen = self->gap_end - self->gap_start;
    lineCount = -1;
    while (pos >= self->gap_start) {
        if (self->buf[pos + gapLen] == '\n') {
            if (++lineCount >= nLines) return pos + 1;
        }
        pos--;
    }
    while (pos >= 0) {
        if (self->buf[pos] == '\n') {
            if (++lineCount >= nLines) return pos + 1;
        }
        pos--;
    }
    return 0;
}

int Fl_Text_Buffer_findchar_forward(const Fl_Text_Buffer *self, int startPos, unsigned searchChar, int *foundPos) {
    if (startPos >= self->length) { *foundPos = self->length; return 0; }
    if (startPos < 0) startPos = 0;
    for (; startPos < self->length; startPos = Fl_Text_Buffer_next_char(self, startPos)) {
        if (searchChar == Fl_Text_Buffer_char_at(self, startPos)) { *foundPos = startPos; return 1; }
    }
    *foundPos = self->length;
    return 0;
}

int Fl_Text_Buffer_findchar_backward(const Fl_Text_Buffer *self, int startPos, unsigned int searchChar, int *foundPos) {
    if (startPos <= 0) { *foundPos = 0; return 0; }
    if (startPos > self->length) startPos = self->length;
    for (startPos = Fl_Text_Buffer_prev_char(self, startPos); startPos >= 0; startPos = Fl_Text_Buffer_prev_char(self, startPos)) {
        if (searchChar == Fl_Text_Buffer_char_at(self, startPos)) { *foundPos = startPos; return 1; }
    }
    *foundPos = 0;
    return 0;
}

int Fl_Text_Buffer_search_forward(const Fl_Text_Buffer *self, int startPos, const char *searchString, int *foundPos, int matchCase) {
    int bp;
    const char *sp;
    if (!searchString) return 0;
    if (matchCase) {
        while (startPos < self->length) {
            bp = startPos;
            sp = searchString;
            for (;;) {
                char c = *sp;
                if (!c) { *foundPos = startPos; return 1; }
                {
                    int l = fl_utf8len1(c);
                    if (memcmp(sp, Fl_Text_Buffer_address(self, bp), (size_t)l)) break;
                    sp += l; bp += l;
                }
            }
            startPos = Fl_Text_Buffer_next_char(self, startPos);
        }
    } else {
        while (startPos < self->length) {
            bp = startPos;
            sp = searchString;
            for (;;) {
                int l;
                unsigned int b, s;
                if (!*sp) { *foundPos = startPos; return 1; }
                b = Fl_Text_Buffer_char_at(self, bp);
                s = fl_utf8decode(sp, 0, &l);
                if (fl_tolower(b) != fl_tolower(s)) break;
                sp += l;
                bp = Fl_Text_Buffer_next_char(self, bp);
            }
            startPos = Fl_Text_Buffer_next_char(self, startPos);
        }
    }
    return 0;
}

int Fl_Text_Buffer_search_backward(const Fl_Text_Buffer *self, int startPos, const char *searchString, int *foundPos, int matchCase) {
    int bp;
    const char *sp;
    if (!searchString) return 0;
    if (matchCase) {
        while (startPos >= 0) {
            bp = startPos;
            sp = searchString;
            for (;;) {
                char c = *sp;
                if (!c) { *foundPos = startPos; return 1; }
                {
                    int l = fl_utf8len1(c);
                    if (memcmp(sp, Fl_Text_Buffer_address(self, bp), (size_t)l)) break;
                    sp += l; bp += l;
                }
            }
            startPos = Fl_Text_Buffer_prev_char(self, startPos);
        }
    } else {
        while (startPos >= 0) {
            bp = startPos;
            sp = searchString;
            for (;;) {
                int l;
                unsigned int b, s;
                if (!*sp) { *foundPos = startPos; return 1; }
                b = Fl_Text_Buffer_char_at(self, bp);
                s = fl_utf8decode(sp, 0, &l);
                if (fl_tolower(b) != fl_tolower(s)) break;
                sp += l;
                bp = Fl_Text_Buffer_next_char(self, bp);
            }
            startPos = Fl_Text_Buffer_prev_char(self, startPos);
        }
    }
    return 0;
}

/* -------------------------------------------------------------------
 * UTF-8 navigation
 * ---------------------------------------------------------------- */

int Fl_Text_Buffer_prev_char_clipped(const Fl_Text_Buffer *self, int pos) {
    char c;
    if (pos <= 0) return 0;
    do {
        pos--;
        if (pos == 0) return 0;
        c = Fl_Text_Buffer_byte_at(self, pos);
    } while ((c & 0xc0) == 0x80);
    return pos;
}

int Fl_Text_Buffer_prev_char(const Fl_Text_Buffer *self, int pos) {
    if (pos == 0) return -1;
    return Fl_Text_Buffer_prev_char_clipped(self, pos);
}

int Fl_Text_Buffer_next_char(const Fl_Text_Buffer *self, int pos) {
    int n = fl_utf8len1(Fl_Text_Buffer_byte_at(self, pos));
    pos += n;
    if (pos >= self->length) return self->length;
    return pos;
}

int Fl_Text_Buffer_utf8_align(const Fl_Text_Buffer *self, int pos) {
    char c = Fl_Text_Buffer_byte_at(self, pos);
    while ((c & 0xc0) == 0x80) {
        pos--;
        c = Fl_Text_Buffer_byte_at(self, pos);
    }
    return pos;
}
