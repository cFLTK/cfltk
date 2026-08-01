/*
 * cfltk - Fl_Text_Display.h
 *
 * C translation of FLTK 1.3 FL/Fl_Text_Display.H.
 *
 * Original class : Fl_Text_Display : public Fl_Group (own draw()/
 *                   handle()/resize(); the rich text viewer -- word
 *                   wrap, mixed fonts/colors via a parallel "style
 *                   buffer", line numbers, scrolling, selection
 *                   highlighting, cursor rendering. Ported from the
 *                   NEdit text editor engine; genuinely one of the
 *                   most intricate single classes in FLTK).
 * New C structure : struct Fl_Text_Display { Fl_Group group;
 *                    ...every mFoo field from upstream, kept with its
 *                    upstream name (just dropping the 'm' prefix) so
 *                    this file and Fl_Text_Display.cxx can be
 *                    cross-referenced line-for-line if a bug ever needs
 *                    tracking down -- a deliberate exception to cfltk's
 *                    usual field-renaming, justified by this class's
 *                    unusual size and algorithmic density... };
 *                    embeds Fl_Group as its first member, same
 *                    layering as every other group-based widget.
 * Vtbl            : fl_text_display_ops -- draw()/handle()/resize() of
 *                    its own; destroy() removes the buffer's modify/
 *                    predelete callbacks before the group teardown.
 * Ownership       : does NOT own the attached Fl_Text_Buffer (multiple
 *                    displays can share one buffer, exactly like
 *                    upstream) or the style buffer/table passed to
 *                    highlight_data(). Owns its two scrollbars as
 *                    normal, heap-allocated Fl_Group children (created
 *                    via Fl_Scrollbar_new() and auto-added like any
 *                    other widget -- unlike Fl_Scroll/Fl_Browser_,
 *                    upstream's own Fl_Text_Display genuinely uses
 *                    real pointer-owned children here, not embedded
 *                    struct members, so there is no destroy-ordering
 *                    kludge to replicate).
 * Known differences:
 *   - No drag-and-drop (dropping the DRAG_START_DND-initiates-OS-DND
 *     path and the FL_DND_* event cases -- cfltk has no DND, see
 *     docs/DESIGN.md). Clicking inside an existing selection still
 *     correctly collapses it and moves the cursor there; only
 *     "drag selected text to another app" is missing, and no backend
 *     sends FL_DND_* events anyway.
 *   - No custom mouse cursor shapes over the text area (upstream calls
 *     window()->cursor(FL_CURSOR_INSERT) etc. -- cfltk's Fl_Window has
 *     no cursor-shape API yet).
 *   - No printing-surface awareness (upstream checks
 *     Fl_Surface_Device::surface() in draw() to fill the background
 *     differently when rendering to a printer; cfltk has no printing
 *     subsystem, draw() always assumes screen display).
 *   - No system beep (fl_beep(), used nowhere critical upstream either).
 *   - IME/composition (Fl::compose()) is not ported -- Fl_Text_Editor's
 *     handle_key() goes straight to the printable-ASCII-character path,
 *     consistent with cfltk's existing IME-free Fl_Input.
 */
#ifndef CFLTK_FL_TEXT_DISPLAY_H
#define CFLTK_FL_TEXT_DISPLAY_H

#include "cfltk/Fl_Group.h"
#include "cfltk/Fl_Scrollbar.h"
#include "cfltk/Fl_Text_Buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

/* cursor_style() values */
#define FL_TEXT_DISPLAY_NORMAL_CURSOR 0
#define FL_TEXT_DISPLAY_CARET_CURSOR  1
#define FL_TEXT_DISPLAY_DIM_CURSOR    2
#define FL_TEXT_DISPLAY_BLOCK_CURSOR  3
#define FL_TEXT_DISPLAY_HEAVY_CURSOR  4
#define FL_TEXT_DISPLAY_SIMPLE_CURSOR 5

/* position_to_xy()/xy_to_position() PosType */
#define FL_TEXT_DISPLAY_CURSOR_POS    0
#define FL_TEXT_DISPLAY_CHARACTER_POS 1

/* drag types -- match Fl_event_clicks() (0 = single, 1 = double, 2 = triple) */
#define FL_TEXT_DISPLAY_DRAG_NONE     (-2)
#define FL_TEXT_DISPLAY_DRAG_START_DND (-1)
#define FL_TEXT_DISPLAY_DRAG_CHAR     0
#define FL_TEXT_DISPLAY_DRAG_WORD     1
#define FL_TEXT_DISPLAY_DRAG_LINE     2

/* wrap_mode() values */
#define FL_TEXT_DISPLAY_WRAP_NONE      0
#define FL_TEXT_DISPLAY_WRAP_AT_COLUMN 1
#define FL_TEXT_DISPLAY_WRAP_AT_PIXEL  2
#define FL_TEXT_DISPLAY_WRAP_AT_BOUNDS 3

typedef struct Fl_Text_Display_Style {
    Fl_Color color;
    Fl_Font font;
    Fl_Fontsize size;
    unsigned attr; /* currently unused, matches upstream */
} Fl_Text_Display_Style;

typedef struct Fl_Text_Display Fl_Text_Display;

typedef void (*Fl_Text_Display_Unfinished_Style_Cb)(int, void *);

typedef struct Fl_Text_Display_Rect { int x, y, w, h; } Fl_Text_Display_Rect;

struct Fl_Text_Display {
    Fl_Group group;

    int damage_range1_start, damage_range1_end;
    int damage_range2_start, damage_range2_end;
    int mCursorPos;
    int mCursorOn;
    int mCursorOldY;
    int mCursorToHint;
    int mCursorStyle;
    int mCursorPreferredXPos;
    int mNVisibleLines;
    int mNBufferLines;
    Fl_Text_Buffer *mBuffer;
    Fl_Text_Buffer *mStyleBuffer;
    int mFirstChar, mLastChar;
    int mContinuousWrap;
    int mWrapMarginPix;
    int *mLineStarts;
    int mTopLineNum;
    int mAbsTopLineNum;
    int mNeedAbsTopLineNum;
    int mHorizOffset;
    int mTopLineNumHint;
    int mHorizOffsetHint;
    int mNStyles;
    const Fl_Text_Display_Style *mStyleTable;
    char mUnfinishedStyle;
    Fl_Text_Display_Unfinished_Style_Cb mUnfinishedHighlightCB;
    void *mHighlightCBArg;

    int mMaxsize;

    int mSuppressResync;
    int mNLinesDeleted;
    int mModifyingTabDistance;

    double mColumnScale;

    Fl_Color mCursor_color;

    Fl_Scrollbar *mHScrollBar;
    Fl_Scrollbar *mVScrollBar;
    int scrollbar_width_;
    Fl_Align scrollbar_align_;
    int dragPos, dragType, dragging;
    int display_insert_position_hint;
    Fl_Text_Display_Rect text_area;

    int shortcut_;

    Fl_Font textfont_;
    Fl_Fontsize textsize_;
    Fl_Color textcolor_;

    int mLineNumLeft, mLineNumWidth;

    Fl_Font linenumber_font_;
    Fl_Fontsize linenumber_size_;
    Fl_Color linenumber_fgcolor_;
    Fl_Color linenumber_bgcolor_;
    Fl_Align linenumber_align_;
    char *linenumber_format_; /* owned, strdup'd */
};

extern const Fl_WidgetOps fl_text_display_ops;

void Fl_Text_Display_init(Fl_Text_Display *self, int x, int y, int w, int h, const char *label);
Fl_Text_Display *Fl_Text_Display_new(int x, int y, int w, int h, const char *label);
void Fl_Text_Display_destroy(Fl_Widget *self);

void Fl_Text_Display_draw(Fl_Widget *self);
int Fl_Text_Display_handle(Fl_Widget *self, int event);
void Fl_Text_Display_resize(Fl_Widget *self, int x, int y, int w, int h);

void Fl_Text_Display_set_buffer(Fl_Text_Display *self, Fl_Text_Buffer *buf);
static inline Fl_Text_Buffer *Fl_Text_Display_buffer(const Fl_Text_Display *self) { return self->mBuffer; }

void Fl_Text_Display_redisplay_range(Fl_Text_Display *self, int start, int end);
void Fl_Text_Display_scroll(Fl_Text_Display *self, int topLineNum, int horizOffset);
void Fl_Text_Display_insert(Fl_Text_Display *self, const char *text);
void Fl_Text_Display_overstrike(Fl_Text_Display *self, const char *text);
void Fl_Text_Display_set_insert_position(Fl_Text_Display *self, int newPos);
static inline int Fl_Text_Display_insert_position(const Fl_Text_Display *self) { return self->mCursorPos; }
int Fl_Text_Display_position_to_xy(const Fl_Text_Display *self, int pos, int *x, int *y);
int Fl_Text_Display_xy_to_position(const Fl_Text_Display *self, int x, int y, int posType);
int Fl_Text_Display_position_to_linecol(const Fl_Text_Display *self, int pos, int *lineNum, int *column);

int Fl_Text_Display_in_selection(const Fl_Text_Display *self, int x, int y);
void Fl_Text_Display_show_insert_position(Fl_Text_Display *self);

int Fl_Text_Display_move_right(Fl_Text_Display *self);
int Fl_Text_Display_move_left(Fl_Text_Display *self);
int Fl_Text_Display_move_up(Fl_Text_Display *self);
int Fl_Text_Display_move_down(Fl_Text_Display *self);
int Fl_Text_Display_count_lines(const Fl_Text_Display *self, int start, int end, int start_pos_is_line_start);
int Fl_Text_Display_line_start(const Fl_Text_Display *self, int pos);
int Fl_Text_Display_line_end(const Fl_Text_Display *self, int startPos, int startPosIsLineStart);
int Fl_Text_Display_skip_lines(Fl_Text_Display *self, int startPos, int nLines, int startPosIsLineStart);
int Fl_Text_Display_rewind_lines(Fl_Text_Display *self, int startPos, int nLines);
void Fl_Text_Display_next_word(Fl_Text_Display *self);
void Fl_Text_Display_previous_word(Fl_Text_Display *self);

void Fl_Text_Display_show_cursor(Fl_Text_Display *self, int b);
static inline void Fl_Text_Display_hide_cursor(Fl_Text_Display *self) { Fl_Text_Display_show_cursor(self, 0); }
void Fl_Text_Display_set_cursor_style(Fl_Text_Display *self, int style);
static inline Fl_Color Fl_Text_Display_cursor_color(const Fl_Text_Display *self) { return self->mCursor_color; }
static inline void Fl_Text_Display_set_cursor_color(Fl_Text_Display *self, Fl_Color n) { self->mCursor_color = n; }

static inline int Fl_Text_Display_scrollbar_width(const Fl_Text_Display *self) { return self->scrollbar_width_; }
static inline void Fl_Text_Display_set_scrollbar_width(Fl_Text_Display *self, int w) { self->scrollbar_width_ = w; }
static inline Fl_Align Fl_Text_Display_scrollbar_align(const Fl_Text_Display *self) { return self->scrollbar_align_; }
static inline void Fl_Text_Display_set_scrollbar_align(Fl_Text_Display *self, Fl_Align a) { self->scrollbar_align_ = a; }

static inline int Fl_Text_Display_word_start(const Fl_Text_Display *self, int pos) { return Fl_Text_Buffer_word_start(self->mBuffer, pos); }
static inline int Fl_Text_Display_word_end(const Fl_Text_Display *self, int pos) { return Fl_Text_Buffer_word_end(self->mBuffer, pos); }

void Fl_Text_Display_highlight_data(Fl_Text_Display *self, Fl_Text_Buffer *styleBuffer,
                                     const Fl_Text_Display_Style *styleTable, int nStyles,
                                     char unfinishedStyle, Fl_Text_Display_Unfinished_Style_Cb unfinishedHighlightCB,
                                     void *cbArg);
int Fl_Text_Display_position_style(const Fl_Text_Display *self, int lineStartPos, int lineLen, int lineIndex);

static inline int Fl_Text_Display_shortcut(const Fl_Text_Display *self) { return self->shortcut_; }
static inline void Fl_Text_Display_set_shortcut(Fl_Text_Display *self, int s) { self->shortcut_ = s; }

static inline Fl_Font Fl_Text_Display_textfont(const Fl_Text_Display *self) { return self->textfont_; }
void Fl_Text_Display_set_textfont(Fl_Text_Display *self, Fl_Font s);
static inline Fl_Fontsize Fl_Text_Display_textsize(const Fl_Text_Display *self) { return self->textsize_; }
void Fl_Text_Display_set_textsize(Fl_Text_Display *self, Fl_Fontsize s);
static inline Fl_Color Fl_Text_Display_textcolor(const Fl_Text_Display *self) { return self->textcolor_; }
static inline void Fl_Text_Display_set_textcolor(Fl_Text_Display *self, Fl_Color n) { self->textcolor_ = n; }

int Fl_Text_Display_wrapped_column(const Fl_Text_Display *self, int row, int column);
int Fl_Text_Display_wrapped_row(const Fl_Text_Display *self, int row);
void Fl_Text_Display_wrap_mode(Fl_Text_Display *self, int wrap, int wrapMargin);

double Fl_Text_Display_x_to_col(const Fl_Text_Display *self, double x);
double Fl_Text_Display_col_to_x(const Fl_Text_Display *self, double col);

void Fl_Text_Display_set_linenumber_width(Fl_Text_Display *self, int width);
static inline int Fl_Text_Display_linenumber_width(const Fl_Text_Display *self) { return self->mLineNumWidth; }
static inline Fl_Font Fl_Text_Display_linenumber_font(const Fl_Text_Display *self) { return self->linenumber_font_; }
static inline void Fl_Text_Display_set_linenumber_font(Fl_Text_Display *self, Fl_Font v) { self->linenumber_font_ = v; }
static inline Fl_Fontsize Fl_Text_Display_linenumber_size(const Fl_Text_Display *self) { return self->linenumber_size_; }
static inline void Fl_Text_Display_set_linenumber_size(Fl_Text_Display *self, Fl_Fontsize v) { self->linenumber_size_ = v; }
static inline Fl_Color Fl_Text_Display_linenumber_fgcolor(const Fl_Text_Display *self) { return self->linenumber_fgcolor_; }
static inline void Fl_Text_Display_set_linenumber_fgcolor(Fl_Text_Display *self, Fl_Color v) { self->linenumber_fgcolor_ = v; }
static inline Fl_Color Fl_Text_Display_linenumber_bgcolor(const Fl_Text_Display *self) { return self->linenumber_bgcolor_; }
static inline void Fl_Text_Display_set_linenumber_bgcolor(Fl_Text_Display *self, Fl_Color v) { self->linenumber_bgcolor_ = v; }
static inline Fl_Align Fl_Text_Display_linenumber_align(const Fl_Text_Display *self) { return self->linenumber_align_; }
static inline void Fl_Text_Display_set_linenumber_align(Fl_Text_Display *self, Fl_Align v) { self->linenumber_align_ = v; }
static inline const char *Fl_Text_Display_linenumber_format(const Fl_Text_Display *self) { return self->linenumber_format_; }
void Fl_Text_Display_set_linenumber_format(Fl_Text_Display *self, const char *val);

/* Internal use (by Fl_Text_Editor and the shared drag helper below);
 * exposed the way upstream's "protected" members are visible to a
 * subclass -- struct fields have no access control in C. */
void Fl_Text_Display_display_insert(Fl_Text_Display *self);
Fl_Group *Fl_Text_Display_as_group(Fl_Widget *self);

/* Upstream: friend void fl_text_drag_me(int, Fl_Text_Display*) -- shared
 * by mouse-drag selection and Fl_Text_Editor's shift+arrow-key selection. */
void Fl_Text_Display_drag_me(Fl_Text_Display *self, int pos);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_TEXT_DISPLAY_H */
