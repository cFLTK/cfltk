/*
 * cfltk - Fl_Text_Display.c
 * See include/cfltk/Fl_Text_Display.h for the class-conversion notes.
 * Translated from src/Fl_Text_Display.cxx.
 */
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cfltk/Fl_Text_Display.h"
#include "cfltk/Fl.h"
#include "cfltk/fl_draw.h"
#include "cfltk/fl_utf8.h"

static int imax(int a, int b) { return a >= b ? a : b; }
static int imin(int a, int b) { return a <= b ? a : b; }

#define TOP_MARGIN 1
#define BOTTOM_MARGIN 1
#define LEFT_MARGIN 3
#define RIGHT_MARGIN 3
#define NO_HINT (-1)

#define FILL_MASK 0x0100
#define SECONDARY_MASK 0x0200
#define PRIMARY_MASK 0x0400
#define HIGHLIGHT_MASK 0x0800
#define BG_ONLY_MASK 0x1000
#define TEXT_ONLY_MASK 0x2000
#define STYLE_LOOKUP_MASK 0xff

enum { H_DRAW_LINE, H_FIND_INDEX, H_FIND_INDEX_FROM_ZERO, H_GET_WIDTH };

/* Smooth-scroll-while-dragging-off-the-edge timer state; upstream keeps
 * these as file statics too (only one drag can be in progress at a
 * time process-wide). */
static int scroll_direction = 0;
static int scroll_amount = 0;
static int scroll_y = 0;
static int scroll_x = 0;

static inline Fl_Widget *sb_widget(Fl_Scrollbar *s) { return &s->slider.valuator.widget; }

static void update_child(Fl_Widget *w) {
    if (Fl_Widget_damage(w)) { Fl_Widget_draw(w); Fl_Widget_clear_damage(w, 0); }
}

/* -------------------------------------------------------------------
 * Forward declarations (the file is one big mutually-recursive graph,
 * matching upstream's own header-declared-methods-defined-in-any-order
 * shape).
 * ---------------------------------------------------------------- */
static int position_to_line(const Fl_Text_Display *self, int pos, int *lineNum);
static int handle_vline(const Fl_Text_Display *self, int mode, int lineStartPos, int lineLen,
                         int leftChar, int rightChar, int Y, int bottomClip, int leftClip, int rightClip);
static void draw_vline(Fl_Text_Display *self, int visLineNum, int leftClip, int rightClip, int leftCharIndex, int rightCharIndex);
static void draw_string(const Fl_Text_Display *self, int style, int X, int Y, int toX, const char *string, int nChars);
static void clear_rect(const Fl_Text_Display *self, int style, int X, int Y, int width, int height);
static void draw_cursor(Fl_Text_Display *self, int X, int Y);
static int find_x(const Fl_Text_Display *self, const char *s, int len, int style, int x);
static double string_width(const Fl_Text_Display *self, const char *string, int length, int style);
static int xy_to_position(const Fl_Text_Display *self, int X, int Y, int posType);
static void offset_line_starts(Fl_Text_Display *self, int newTopLineNum);
static void update_line_starts(Fl_Text_Display *self, int pos, int charsInserted, int charsDeleted,
                                int linesInserted, int linesDeleted, int *scrolled);
static void calc_line_starts(Fl_Text_Display *self, int startLine, int endLine);
static void calc_last_char(Fl_Text_Display *self);
static int scroll_(Fl_Text_Display *self, int topLineNum, int horizOffset);
static void update_v_scrollbar(Fl_Text_Display *self);
static void update_h_scrollbar(Fl_Text_Display *self);
static int measure_vline(const Fl_Text_Display *self, int visLineNum);
static int longest_vline(const Fl_Text_Display *self);
static int empty_vlines(const Fl_Text_Display *self);
static int vline_length(const Fl_Text_Display *self, int visLineNum);
static void find_wrap_range(Fl_Text_Display *self, const char *deletedText, int pos, int nInserted, int nDeleted,
                             int *modRangeStart, int *modRangeEnd, int *linesInserted, int *linesDeleted);
static void measure_deleted_lines(Fl_Text_Display *self, int pos, int nDeleted);
static void wrapped_line_counter(const Fl_Text_Display *self, Fl_Text_Buffer *buf, int startPos, int maxPos,
                                  int maxLines, int startPosIsLineStart, int styleBufOffset,
                                  int *retPos, int *retLines, int *retLineStart, int *retLineEnd,
                                  int countLastLineMissingNewLine);
static void find_line_end(const Fl_Text_Display *self, int startPos, int startPosIsLineStart, int *lineEnd, int *nextLineStart);
static double measure_proportional_character(const Fl_Text_Display *self, const char *s, int xPix, int pos);
static int wrap_uses_character(const Fl_Text_Display *self, int lineEndPos);
static void extend_range_for_styles(Fl_Text_Display *self, int *startpos, int *endpos);
static void draw_line_numbers(Fl_Text_Display *self, int clearAll);
static void buffer_predelete_cb(int pos, int nDeleted, void *cbArg);
static void buffer_modified_cb(int pos, int nInserted, int nDeleted, int nRestyled, const char *deletedText, void *cbArg);
static void v_scrollbar_cb(Fl_Widget *w, void *data);
static void h_scrollbar_cb(Fl_Widget *w, void *data);
static void scroll_timer_cb(void *data);

static int countlines(const char *string) {
    const char *c;
    int n = 0;
    if (!string) return 0;
    for (c = string; *c; c++) if (*c == '\n') n++;
    return n;
}

static int maintaining_absolute_top_line_number(const Fl_Text_Display *self) {
    return self->mContinuousWrap && (self->mLineNumWidth != 0 || self->mNeedAbsTopLineNum);
}

static void reset_absolute_top_line_number(Fl_Text_Display *self);
static void absolute_top_line_number(Fl_Text_Display *self, int oldFirstChar);

/* -------------------------------------------------------------------
 * Construction
 * ---------------------------------------------------------------- */

static Fl_Group *Fl_Text_Display_as_group_impl(Fl_Widget *self) { return (Fl_Group *)self; }
Fl_Group *Fl_Text_Display_as_group(Fl_Widget *self) { return Fl_Text_Display_as_group_impl(self); }

const Fl_WidgetOps fl_text_display_ops = {
    Fl_Text_Display_draw,
    Fl_Text_Display_handle,
    Fl_Text_Display_resize,
    NULL, NULL,
    Fl_Text_Display_destroy,
    Fl_Text_Display_as_group_impl,
    NULL
};

void Fl_Text_Display_init(Fl_Text_Display *self, int x, int y, int w, int h, const char *label) {
    Fl_Group_init(&self->group, x, y, w, h, label);
    self->group.widget.ops = &fl_text_display_ops;

    self->mMaxsize = 0;
    self->damage_range1_start = self->damage_range1_end = -1;
    self->damage_range2_start = self->damage_range2_end = -1;
    self->dragPos = self->dragging = 0;
    self->dragType = FL_TEXT_DISPLAY_DRAG_CHAR;
    self->display_insert_position_hint = 0;
    self->shortcut_ = 0;

    Fl_Widget_set_colors(&self->group.widget, FL_BACKGROUND2_COLOR, FL_SELECTION_COLOR);
    self->group.widget.box = FL_DOWN_FRAME;
    self->textsize_ = FL_NORMAL_SIZE;
    self->textcolor_ = FL_FOREGROUND_COLOR;
    self->textfont_ = FL_HELVETICA;
    self->group.widget.flags |= FL_WIDGET_SHORTCUT_LABEL;

    self->text_area.x = self->text_area.y = self->text_area.w = self->text_area.h = 0;

    self->mVScrollBar = Fl_Scrollbar_new(0, 0, 1, 1, NULL);
    Fl_Widget_set_callback(sb_widget(self->mVScrollBar), v_scrollbar_cb, self);
    self->mHScrollBar = Fl_Scrollbar_new(0, 0, 1, 1, NULL);
    Fl_Widget_set_callback(sb_widget(self->mHScrollBar), h_scrollbar_cb, self);
    Fl_Widget_set_type(sb_widget(self->mHScrollBar), FL_HORIZONTAL);

    Fl_Group_end(&self->group);

    self->scrollbar_width_ = Fl_scrollbar_size();
    self->scrollbar_align_ = FL_ALIGN_BOTTOM_RIGHT;

    self->mCursorOn = 0;
    self->mCursorPos = 0;
    self->mCursorOldY = -100;
    self->mCursorToHint = NO_HINT;
    self->mCursorStyle = FL_TEXT_DISPLAY_NORMAL_CURSOR;
    self->mCursorPreferredXPos = -1;
    self->mBuffer = NULL;
    self->mFirstChar = 0;
    self->mLastChar = 0;
    self->mNBufferLines = 0;
    self->mTopLineNum = self->mTopLineNumHint = 1;
    self->mAbsTopLineNum = 1;
    self->mNeedAbsTopLineNum = 0;
    self->mHorizOffset = self->mHorizOffsetHint = 0;

    self->mCursor_color = FL_FOREGROUND_COLOR;

    self->mStyleBuffer = NULL;
    self->mStyleTable = NULL;
    self->mNStyles = 0;
    self->mNVisibleLines = 1;
    self->mLineStarts = (int *)malloc(sizeof(int) * (size_t)self->mNVisibleLines);
    self->mLineStarts[0] = 0;
    self->mSuppressResync = 0;
    self->mNLinesDeleted = 0;
    self->mModifyingTabDistance = 0;

    self->mUnfinishedStyle = 0;
    self->mUnfinishedHighlightCB = NULL;
    self->mHighlightCBArg = NULL;

    self->mLineNumLeft = self->mLineNumWidth = 0;
    self->mContinuousWrap = 0;
    self->mWrapMarginPix = 0;
    self->mColumnScale = 0;

    self->linenumber_font_ = FL_HELVETICA;
    self->linenumber_size_ = FL_NORMAL_SIZE;
    self->linenumber_fgcolor_ = FL_INACTIVE_COLOR;
    self->linenumber_bgcolor_ = 53;
    self->linenumber_align_ = FL_ALIGN_RIGHT;
    {
        static const char default_fmt[] = "%d";
        self->linenumber_format_ = (char *)malloc(sizeof(default_fmt));
        memcpy(self->linenumber_format_, default_fmt, sizeof(default_fmt));
    }
}

Fl_Text_Display *Fl_Text_Display_new(int x, int y, int w, int h, const char *label) {
    Fl_Text_Display *self = (Fl_Text_Display *)malloc(sizeof(Fl_Text_Display));
    Fl_Text_Display_init(self, x, y, w, h, label);
    return self;
}

void Fl_Text_Display_destroy(Fl_Widget *self_w) {
    Fl_Text_Display *self = (Fl_Text_Display *)self_w;
    if (scroll_direction) {
        Fl_remove_timeout(scroll_timer_cb, self);
        scroll_direction = 0;
    }
    if (self->mBuffer) {
        Fl_Text_Buffer_remove_modify_callback(self->mBuffer, buffer_modified_cb, self);
        Fl_Text_Buffer_remove_predelete_callback(self->mBuffer, buffer_predelete_cb, self);
    }
    free(self->mLineStarts);
    free(self->linenumber_format_);
    Fl_Group_destroy(self_w);
}

/* -------------------------------------------------------------------
 * Line number accessors
 * ---------------------------------------------------------------- */

void Fl_Text_Display_set_linenumber_width(Fl_Text_Display *self, int width) {
    if (width < 0) return;
    self->mLineNumWidth = width;
    Fl_Widget_resize(&self->group.widget, self->group.widget.x, self->group.widget.y, self->group.widget.w, self->group.widget.h);
}

void Fl_Text_Display_set_linenumber_format(Fl_Text_Display *self, const char *val) {
    free(self->linenumber_format_);
    if (val) {
        size_t n = strlen(val) + 1;
        self->linenumber_format_ = (char *)malloc(n);
        memcpy(self->linenumber_format_, val, n);
    } else {
        self->linenumber_format_ = NULL;
    }
}

/* -------------------------------------------------------------------
 * Buffer attachment
 * ---------------------------------------------------------------- */

void Fl_Text_Display_set_buffer(Fl_Text_Display *self, Fl_Text_Buffer *buf) {
    Fl_Widget *self_w = &self->group.widget;
    if (buf == self->mBuffer) return;
    if (self->mBuffer != NULL) {
        char *deletedText = Fl_Text_Buffer_text(self->mBuffer);
        buffer_modified_cb(0, 0, Fl_Text_Buffer_length(self->mBuffer), 0, deletedText, self);
        free(deletedText);
        self->mNBufferLines = 0;
        Fl_Text_Buffer_remove_modify_callback(self->mBuffer, buffer_modified_cb, self);
        Fl_Text_Buffer_remove_predelete_callback(self->mBuffer, buffer_predelete_cb, self);
    }

    self->mBuffer = buf;
    if (self->mBuffer) {
        Fl_Text_Buffer_add_modify_callback(self->mBuffer, buffer_modified_cb, self);
        Fl_Text_Buffer_add_predelete_callback(self->mBuffer, buffer_predelete_cb, self);
        buffer_modified_cb(0, Fl_Text_Buffer_length(buf), 0, 0, NULL, self);
    }

    Fl_Widget_resize(self_w, self_w->x, self_w->y, self_w->w, self_w->h);
}

void Fl_Text_Display_highlight_data(Fl_Text_Display *self, Fl_Text_Buffer *styleBuffer,
                                     const Fl_Text_Display_Style *styleTable, int nStyles,
                                     char unfinishedStyle, Fl_Text_Display_Unfinished_Style_Cb unfinishedHighlightCB,
                                     void *cbArg) {
    self->mStyleBuffer = styleBuffer;
    self->mStyleTable = styleTable;
    self->mNStyles = nStyles;
    self->mUnfinishedStyle = unfinishedStyle;
    self->mUnfinishedHighlightCB = unfinishedHighlightCB;
    self->mHighlightCBArg = cbArg;
    self->mColumnScale = 0;

    Fl_Text_Buffer_set_can_undo(self->mStyleBuffer, 0);
    Fl_Widget_set_damage(&self->group.widget, FL_DAMAGE_EXPOSE);
}

static int longest_vline(const Fl_Text_Display *self) {
    int longest = 0, i;
    for (i = 0; i < self->mNVisibleLines; i++) longest = imax(longest, measure_vline(self, i));
    return longest;
}

/* -------------------------------------------------------------------
 * resize()
 * ---------------------------------------------------------------- */

void Fl_Text_Display_resize(Fl_Widget *self_w, int X, int Y, int W, int H) {
    Fl_Text_Display *self = (Fl_Text_Display *)self_w;
    unsigned int hscrollbarvisible, vscrollbarvisible;
    int oldTAWidth;
    int i;
    int again;

    Fl_Widget_default_resize(self_w, X, Y, W, H);
    if (!self->mBuffer) return;

    hscrollbarvisible = Fl_Widget_visible(sb_widget(self->mHScrollBar));
    vscrollbarvisible = Fl_Widget_visible(sb_widget(self->mVScrollBar));

    oldTAWidth = self->text_area.w;

    X += fl_box_dx(self_w->box);
    Y += fl_box_dy(self_w->box);
    W -= fl_box_dw(self_w->box);
    H -= fl_box_dh(self_w->box);

    self->text_area.x = X + LEFT_MARGIN + self->mLineNumWidth;
    self->text_area.y = Y + TOP_MARGIN;
    self->text_area.w = W - LEFT_MARGIN - RIGHT_MARGIN - self->mLineNumWidth;
    self->text_area.h = H - TOP_MARGIN - BOTTOM_MARGIN;

    fl_font(self->textfont_, self->textsize_);
    self->mMaxsize = fl_height();
    for (i = 0; i < self->mNStyles; i++) {
        fl_font(self->mStyleTable[i].font, self->mStyleTable[i].size);
        self->mMaxsize = imax(self->mMaxsize, fl_height());
    }

    Fl_Widget_clear_visible(sb_widget(self->mVScrollBar));
    Fl_Widget_clear_visible(sb_widget(self->mHScrollBar));

    if (self->mContinuousWrap && !self->mWrapMarginPix) {
        int nvlines = (self->text_area.h + self->mMaxsize - 1) / self->mMaxsize;
        int nlines = Fl_Text_Buffer_count_lines(self->mBuffer, 0, Fl_Text_Buffer_length(self->mBuffer));
        if (nvlines < 1) nvlines = 1;
        if (nlines >= nvlines - 1) {
            Fl_Widget_set_visible(sb_widget(self->mVScrollBar));
            self->text_area.w -= self->scrollbar_width_;
        }
    }

    again = 1;
    while (again) {
        again = 0;
        if (self->mContinuousWrap && !self->mWrapMarginPix && self->text_area.w != oldTAWidth) {
            int oldFirstChar = self->mFirstChar;
            self->mNBufferLines = Fl_Text_Display_count_lines(self, 0, Fl_Text_Buffer_length(self->mBuffer), 1);
            self->mFirstChar = Fl_Text_Display_line_start(self, self->mFirstChar);
            self->mTopLineNum = Fl_Text_Display_count_lines(self, 0, self->mFirstChar, 1) + 1;
            absolute_top_line_number(self, oldFirstChar);
        }

        oldTAWidth = self->text_area.w;

        {
            int nvlines = (self->text_area.h + self->mMaxsize - 1) / self->mMaxsize;
            if (nvlines < 1) nvlines = 1;
            if (self->mNVisibleLines != nvlines) {
                self->mNVisibleLines = nvlines;
                free(self->mLineStarts);
                self->mLineStarts = (int *)malloc(sizeof(int) * (size_t)self->mNVisibleLines);
            }
        }

        calc_line_starts(self, 0, self->mNVisibleLines);
        calc_last_char(self);

        if (self->scrollbar_width_) {
            if (!Fl_Widget_visible(sb_widget(self->mVScrollBar)) &&
                (self->scrollbar_align_ & (FL_ALIGN_LEFT | FL_ALIGN_RIGHT)) &&
                self->mNBufferLines >= self->mNVisibleLines - 1) {
                Fl_Widget_set_visible(sb_widget(self->mVScrollBar));
                self->text_area.w -= self->scrollbar_width_;
                again = 1;
            }

            if (!Fl_Widget_visible(sb_widget(self->mHScrollBar)) &&
                (self->scrollbar_align_ & (FL_ALIGN_TOP | FL_ALIGN_BOTTOM)) &&
                (Fl_Widget_visible(sb_widget(self->mVScrollBar)) || longest_vline(self) > self->text_area.w)) {
                int wrap_at_bounds = self->mContinuousWrap && (self->mWrapMarginPix < self->text_area.w);
                if (!wrap_at_bounds) {
                    Fl_Widget_set_visible(sb_widget(self->mHScrollBar));
                    self->text_area.h -= self->scrollbar_width_;
                    again = 1;
                }
            }
        }
    }

    self->text_area.x = X + self->mLineNumWidth + LEFT_MARGIN;
    if (Fl_Widget_visible(sb_widget(self->mVScrollBar)) && (self->scrollbar_align_ & FL_ALIGN_LEFT))
        self->text_area.x += self->scrollbar_width_;

    self->text_area.y = Y + TOP_MARGIN;
    if (Fl_Widget_visible(sb_widget(self->mHScrollBar)) && (self->scrollbar_align_ & FL_ALIGN_TOP))
        self->text_area.y += self->scrollbar_width_;

    if (Fl_Widget_visible(sb_widget(self->mVScrollBar))) {
        if (self->scrollbar_align_ & FL_ALIGN_LEFT) {
            Fl_Widget_resize(sb_widget(self->mVScrollBar), self->text_area.x - LEFT_MARGIN - self->scrollbar_width_,
                              self->text_area.y - TOP_MARGIN, self->scrollbar_width_, self->text_area.h + TOP_MARGIN + BOTTOM_MARGIN);
        } else {
            Fl_Widget_resize(sb_widget(self->mVScrollBar), X + W - self->scrollbar_width_,
                              self->text_area.y - TOP_MARGIN, self->scrollbar_width_, self->text_area.h + TOP_MARGIN + BOTTOM_MARGIN);
        }
    }

    if (Fl_Widget_visible(sb_widget(self->mHScrollBar))) {
        if (self->scrollbar_align_ & FL_ALIGN_TOP) {
            Fl_Widget_resize(sb_widget(self->mHScrollBar), self->text_area.x - LEFT_MARGIN, Y,
                              self->text_area.w + LEFT_MARGIN + RIGHT_MARGIN, self->scrollbar_width_);
        } else {
            Fl_Widget_resize(sb_widget(self->mHScrollBar), self->text_area.x - LEFT_MARGIN, Y + H - self->scrollbar_width_,
                              self->text_area.w + LEFT_MARGIN + RIGHT_MARGIN, self->scrollbar_width_);
        }
    }

    if (self->mTopLineNumHint != self->mTopLineNum || self->mHorizOffsetHint != self->mHorizOffset)
        scroll_(self, self->mTopLineNumHint, self->mHorizOffsetHint);

    if (self->mNBufferLines < self->mNVisibleLines || self->mBuffer == NULL || Fl_Text_Buffer_length(self->mBuffer) == 0) {
        scroll_(self, 1, self->mHorizOffset);
    } else {
        while (self->mNVisibleLines >= 2 && self->mLineStarts[self->mNVisibleLines - 2] == -1 &&
               scroll_(self, self->mTopLineNum - 1, self->mHorizOffset)) { }
    }

    if (self->display_insert_position_hint) Fl_Text_Display_display_insert(self);

    {
        int maxhoffset = imax(0, longest_vline(self) - self->text_area.w);
        if (self->mHorizOffset > maxhoffset) scroll_(self, self->mTopLineNumHint, maxhoffset);
    }

    self->mTopLineNumHint = self->mTopLineNum;
    self->mHorizOffsetHint = self->mHorizOffset;
    self->display_insert_position_hint = 0;

    if (self->mContinuousWrap ||
        hscrollbarvisible != (unsigned)Fl_Widget_visible(sb_widget(self->mHScrollBar)) ||
        vscrollbarvisible != (unsigned)Fl_Widget_visible(sb_widget(self->mVScrollBar)))
        Fl_Widget_redraw(self_w);

    update_v_scrollbar(self);
    update_h_scrollbar(self);
}

/* -------------------------------------------------------------------
 * Damage / redraw ranges
 * ---------------------------------------------------------------- */

void Fl_Text_Display_redisplay_range(Fl_Text_Display *self, int startpos, int endpos) {
    if (self->damage_range1_start == -1 && self->damage_range1_end == -1) {
        self->damage_range1_start = startpos;
        self->damage_range1_end = endpos;
    } else if ((startpos >= self->damage_range1_start && startpos <= self->damage_range1_end) ||
               (endpos >= self->damage_range1_start && endpos <= self->damage_range1_end)) {
        self->damage_range1_start = imin(self->damage_range1_start, startpos);
        self->damage_range1_end = imax(self->damage_range1_end, endpos);
    } else if (self->damage_range2_start == -1 && self->damage_range2_end == -1) {
        self->damage_range2_start = startpos;
        self->damage_range2_end = endpos;
    } else {
        self->damage_range2_start = imin(self->damage_range2_start, startpos);
        self->damage_range2_end = imax(self->damage_range2_end, endpos);
    }
    Fl_Widget_set_damage(&self->group.widget, FL_DAMAGE_SCROLL);
}

static void draw_range(Fl_Text_Display *self, int startpos, int endpos) {
    int i, startLine, lastLine, startIndex, endIndex;

    startpos = Fl_Text_Buffer_utf8_align(self->mBuffer, startpos);
    endpos = Fl_Text_Buffer_utf8_align(self->mBuffer, endpos);

    if (endpos < self->mFirstChar || (startpos > self->mLastChar && !empty_vlines(self))) return;

    if (startpos < 0) startpos = 0;
    if (startpos > Fl_Text_Buffer_length(self->mBuffer)) startpos = Fl_Text_Buffer_length(self->mBuffer);
    if (endpos < 0) endpos = 0;
    if (endpos > Fl_Text_Buffer_length(self->mBuffer)) endpos = Fl_Text_Buffer_length(self->mBuffer);

    if (startpos < self->mFirstChar) startpos = self->mFirstChar;
    if (!position_to_line(self, startpos, &startLine)) startLine = self->mNVisibleLines - 1;
    if (endpos >= self->mLastChar) {
        lastLine = self->mNVisibleLines - 1;
    } else {
        if (!position_to_line(self, endpos, &lastLine)) lastLine = self->mNVisibleLines - 1;
    }

    startIndex = self->mLineStarts[startLine] == -1 ? 0 : startpos - self->mLineStarts[startLine];
    if (endpos >= self->mLastChar) endIndex = INT_MAX;
    else if (self->mLineStarts[lastLine] == -1) endIndex = 0;
    else endIndex = endpos - self->mLineStarts[lastLine];

    if (startLine == lastLine) {
        draw_vline(self, startLine, 0, INT_MAX, startIndex, endIndex);
        return;
    }

    draw_vline(self, startLine, 0, INT_MAX, startIndex, INT_MAX);
    for (i = startLine + 1; i < lastLine; i++) draw_vline(self, i, 0, INT_MAX, 0, INT_MAX);
    draw_vline(self, lastLine, 0, INT_MAX, 0, endIndex);
}

static void draw_text(Fl_Text_Display *self, int left, int top, int width, int height) {
    int fontHeight = self->mMaxsize ? self->mMaxsize : self->textsize_;
    int firstLine = (top - self->text_area.y - fontHeight + 1) / fontHeight;
    int lastLine = (top + height - self->text_area.y) / fontHeight + 1;
    int line;

    fl_push_clip(left, top, width, height);
    for (line = firstLine; line <= lastLine; line++) draw_vline(self, line, left, left + width, 0, INT_MAX);
    fl_pop_clip();
}

/* -------------------------------------------------------------------
 * Cursor / insert position
 * ---------------------------------------------------------------- */

void Fl_Text_Display_set_insert_position(Fl_Text_Display *self, int newPos) {
    if (newPos == self->mCursorPos) return;
    if (newPos < 0) newPos = 0;
    if (newPos > Fl_Text_Buffer_length(self->mBuffer)) newPos = Fl_Text_Buffer_length(self->mBuffer);

    self->mCursorPreferredXPos = -1;

    Fl_Text_Display_redisplay_range(self, Fl_Text_Buffer_prev_char_clipped(self->mBuffer, self->mCursorPos), Fl_Text_Buffer_next_char(self->mBuffer, self->mCursorPos));
    self->mCursorPos = newPos;
    Fl_Text_Display_redisplay_range(self, Fl_Text_Buffer_prev_char_clipped(self->mBuffer, self->mCursorPos), Fl_Text_Buffer_next_char(self->mBuffer, self->mCursorPos));
}

void Fl_Text_Display_show_cursor(Fl_Text_Display *self, int b) {
    self->mCursorOn = b;
    if (!self->mBuffer) return;
    Fl_Text_Display_redisplay_range(self, Fl_Text_Buffer_prev_char_clipped(self->mBuffer, self->mCursorPos), Fl_Text_Buffer_next_char(self->mBuffer, self->mCursorPos));
}

void Fl_Text_Display_set_cursor_style(Fl_Text_Display *self, int style) {
    self->mCursorStyle = style;
    if (self->mCursorOn) Fl_Text_Display_show_cursor(self, 1);
}

void Fl_Text_Display_wrap_mode(Fl_Text_Display *self, int wrap, int wrapMargin) {
    switch (wrap) {
        case FL_TEXT_DISPLAY_WRAP_NONE:
            self->mWrapMarginPix = 0;
            self->mContinuousWrap = 0;
            break;
        case FL_TEXT_DISPLAY_WRAP_AT_PIXEL:
            self->mWrapMarginPix = wrapMargin;
            self->mContinuousWrap = 1;
            break;
        case FL_TEXT_DISPLAY_WRAP_AT_BOUNDS:
            self->mWrapMarginPix = 0;
            self->mContinuousWrap = 1;
            break;
        case FL_TEXT_DISPLAY_WRAP_AT_COLUMN:
        default:
            self->mWrapMarginPix = (int)Fl_Text_Display_col_to_x(self, wrapMargin);
            self->mContinuousWrap = 1;
            break;
    }

    if (self->mBuffer) {
        self->mNBufferLines = Fl_Text_Display_count_lines(self, 0, Fl_Text_Buffer_length(self->mBuffer), 1);
        self->mFirstChar = Fl_Text_Display_line_start(self, self->mFirstChar);
        self->mTopLineNum = Fl_Text_Display_count_lines(self, 0, self->mFirstChar, 1) + 1;
        reset_absolute_top_line_number(self);
        calc_line_starts(self, 0, self->mNVisibleLines);
        calc_last_char(self);
    } else {
        self->mNBufferLines = 0;
        self->mFirstChar = 0;
        self->mTopLineNum = 1;
        self->mAbsTopLineNum = 1;
    }

    Fl_Widget_resize(&self->group.widget, self->group.widget.x, self->group.widget.y, self->group.widget.w, self->group.widget.h);
}

void Fl_Text_Display_insert(Fl_Text_Display *self, const char *text) {
    int pos = self->mCursorPos;
    self->mCursorToHint = (int)(pos + strlen(text));
    Fl_Text_Buffer_insert(self->mBuffer, pos, text);
    self->mCursorToHint = NO_HINT;
}

void Fl_Text_Display_overstrike(Fl_Text_Display *self, const char *text) {
    int startPos = self->mCursorPos;
    Fl_Text_Buffer *buf = self->mBuffer;
    int lineStart = Fl_Text_Buffer_line_start(buf, startPos);
    int textLen = (int)strlen(text);
    int p, endPos, indent, startIndent, endIndent, i;
    const char *c;
    unsigned int ch;
    char *paddedText = NULL;

    startIndent = Fl_Text_Buffer_count_displayed_characters(buf, lineStart, startPos);
    indent = startIndent;
    for (c = text; *c != '\0'; c += fl_utf8len1(*c)) indent++;
    endIndent = indent;

    indent = startIndent;
    for (p = startPos;; p = Fl_Text_Buffer_next_char(buf, p)) {
        if (p == Fl_Text_Buffer_length(buf)) break;
        ch = Fl_Text_Buffer_char_at(buf, p);
        if (ch == '\n') break;
        indent++;
        if (indent == endIndent) {
            p = Fl_Text_Buffer_next_char(buf, p);
            break;
        } else if (indent > endIndent) {
            if (ch != '\t') {
                p = Fl_Text_Buffer_next_char(buf, p);
                paddedText = (char *)malloc((size_t)(textLen + FL_TEXT_MAX_EXP_CHAR_LEN + 1));
                memcpy(paddedText, text, (size_t)textLen + 1);
                for (i = 0; i < indent - endIndent; i++) paddedText[textLen + i] = ' ';
                paddedText[textLen + i] = '\0';
            }
            break;
        }
    }
    endPos = p;

    self->mCursorToHint = startPos + textLen;
    Fl_Text_Buffer_replace(buf, startPos, endPos, paddedText == NULL ? text : paddedText);
    self->mCursorToHint = NO_HINT;
    free(paddedText);
}

int Fl_Text_Display_position_to_xy(const Fl_Text_Display *self, int pos, int *X, int *Y) {
    int lineStartPos, fontHeight, visLineNum;

    if ((pos < self->mFirstChar) || (pos > self->mLastChar && !empty_vlines(self)) || (pos > Fl_Text_Buffer_length(self->mBuffer))) {
        *X = *Y = 0;
        return 0;
    }

    if (!position_to_line(self, pos, &visLineNum) || visLineNum < 0 || visLineNum > self->mNBufferLines) {
        *X = *Y = 0;
        return 0;
    }

    fontHeight = self->mMaxsize;
    *Y = self->text_area.y + visLineNum * fontHeight;

    lineStartPos = self->mLineStarts[visLineNum];
    if (lineStartPos == -1) {
        *X = self->text_area.x - self->mHorizOffset;
        return 1;
    }
    *X = self->text_area.x + handle_vline(self, H_GET_WIDTH, lineStartPos, pos - lineStartPos, 0, 0, 0, 0, 0, 0) - self->mHorizOffset;
    return 1;
}

int Fl_Text_Display_position_to_linecol(const Fl_Text_Display *self, int pos, int *lineNum, int *column) {
    int retVal;
    if (self->mContinuousWrap) {
        if (!maintaining_absolute_top_line_number(self) || pos < self->mFirstChar || pos > self->mLastChar) return 0;
        *lineNum = self->mAbsTopLineNum + Fl_Text_Buffer_count_lines(self->mBuffer, self->mFirstChar, pos);
        *column = Fl_Text_Buffer_count_displayed_characters(self->mBuffer, Fl_Text_Buffer_line_start(self->mBuffer, pos), pos);
        return 1;
    }
    retVal = position_to_line(self, pos, lineNum);
    if (retVal) {
        *column = Fl_Text_Buffer_count_displayed_characters(self->mBuffer, self->mLineStarts[*lineNum], pos);
        *lineNum += self->mTopLineNum;
    }
    return retVal;
}

int Fl_Text_Display_in_selection(const Fl_Text_Display *self, int X, int Y) {
    int pos = xy_to_position(self, X, Y, FL_TEXT_DISPLAY_CHARACTER_POS);
    return Fl_Text_Selection_includes(Fl_Text_Buffer_primary_selection(self->mBuffer), pos);
}

int Fl_Text_Display_wrapped_column(const Fl_Text_Display *self, int row, int column) {
    int lineStart, dispLineStart;
    if (!self->mContinuousWrap || row < 0 || row > self->mNVisibleLines) return column;
    dispLineStart = self->mLineStarts[row];
    if (dispLineStart == -1) return column;
    lineStart = Fl_Text_Buffer_line_start(self->mBuffer, dispLineStart);
    return column + Fl_Text_Buffer_count_displayed_characters(self->mBuffer, lineStart, dispLineStart);
}

int Fl_Text_Display_wrapped_row(const Fl_Text_Display *self, int row) {
    if (!self->mContinuousWrap || row < 0 || row > self->mNVisibleLines) return row;
    return Fl_Text_Buffer_count_lines(self->mBuffer, self->mFirstChar, self->mLineStarts[row]);
}

void Fl_Text_Display_display_insert(Fl_Text_Display *self) {
    int hOffset = self->mHorizOffset;
    int topLine = self->mTopLineNum;
    int X, Y;

    if (Fl_Text_Display_insert_position(self) < self->mFirstChar) {
        topLine -= Fl_Text_Display_count_lines(self, Fl_Text_Display_insert_position(self), self->mFirstChar, 0);
    } else if (self->mNVisibleLines >= 2 && self->mLineStarts[self->mNVisibleLines - 2] != -1) {
        int lastChar = Fl_Text_Display_line_end(self, self->mLineStarts[self->mNVisibleLines - 2], 1);
        if (Fl_Text_Display_insert_position(self) >= lastChar)
            topLine += Fl_Text_Display_count_lines(self, lastChar - (wrap_uses_character(self, self->mLastChar) ? 0 : 1), Fl_Text_Display_insert_position(self), 0);
    }

    if (!Fl_Text_Display_position_to_xy(self, self->mCursorPos, &X, &Y)) {
        scroll_(self, topLine, hOffset);
        if (!Fl_Text_Display_position_to_xy(self, self->mCursorPos, &X, &Y)) return;
    }
    if (X > self->text_area.x + self->text_area.w) hOffset += X - (self->text_area.x + self->text_area.w);
    else if (X < self->text_area.x) hOffset += X - self->text_area.x;

    if (topLine != self->mTopLineNum || hOffset != self->mHorizOffset) scroll_(self, topLine, hOffset);
}

void Fl_Text_Display_show_insert_position(Fl_Text_Display *self) {
    self->display_insert_position_hint = 1;
    Fl_Widget_resize(&self->group.widget, self->group.widget.x, self->group.widget.y, self->group.widget.w, self->group.widget.h);
}

/* -------------------------------------------------------------------
 * Cursor movement
 * ---------------------------------------------------------------- */

int Fl_Text_Display_move_right(Fl_Text_Display *self) {
    int p, q;
    if (self->mCursorPos >= Fl_Text_Buffer_length(self->mBuffer)) return 0;
    p = Fl_Text_Display_insert_position(self);
    q = Fl_Text_Buffer_next_char(self->mBuffer, p);
    Fl_Text_Display_set_insert_position(self, q);
    return 1;
}

int Fl_Text_Display_move_left(Fl_Text_Display *self) {
    int p, q;
    if (self->mCursorPos <= 0) return 0;
    p = Fl_Text_Display_insert_position(self);
    q = Fl_Text_Buffer_prev_char_clipped(self->mBuffer, p);
    Fl_Text_Display_set_insert_position(self, q);
    return 1;
}

int Fl_Text_Display_move_up(Fl_Text_Display *self) {
    int lineStartPos, xPos, prevLineStartPos, newPos, visLineNum, lineEnd;

    if (position_to_line(self, self->mCursorPos, &visLineNum)) {
        lineStartPos = self->mLineStarts[visLineNum];
    } else {
        lineStartPos = Fl_Text_Buffer_line_start(self->mBuffer, self->mCursorPos);
        visLineNum = -1;
    }
    if (lineStartPos == 0) return 0;

    if (self->mCursorPreferredXPos >= 0) xPos = self->mCursorPreferredXPos;
    else xPos = handle_vline(self, H_GET_WIDTH, lineStartPos, self->mCursorPos - lineStartPos, 0, 0, 0, 0, 0, INT_MAX);

    if (visLineNum != -1 && visLineNum != 0) prevLineStartPos = self->mLineStarts[visLineNum - 1];
    else prevLineStartPos = Fl_Text_Display_rewind_lines(self, lineStartPos, 1);

    lineEnd = Fl_Text_Display_line_end(self, prevLineStartPos, 1);
    newPos = handle_vline(self, H_FIND_INDEX_FROM_ZERO, prevLineStartPos, lineEnd - prevLineStartPos, 0, 0, 0, 0, 0, xPos);

    Fl_Text_Display_set_insert_position(self, newPos);
    self->mCursorPreferredXPos = xPos;
    return 1;
}

int Fl_Text_Display_move_down(Fl_Text_Display *self) {
    int lineStartPos, xPos, newPos, visLineNum, nextLineStartPos, lineEnd;

    if (self->mCursorPos == Fl_Text_Buffer_length(self->mBuffer)) return 0;

    if (position_to_line(self, self->mCursorPos, &visLineNum)) {
        lineStartPos = self->mLineStarts[visLineNum];
    } else {
        lineStartPos = Fl_Text_Buffer_line_start(self->mBuffer, self->mCursorPos);
        visLineNum = -1;
    }
    if (self->mCursorPreferredXPos >= 0) xPos = self->mCursorPreferredXPos;
    else xPos = handle_vline(self, H_GET_WIDTH, lineStartPos, self->mCursorPos - lineStartPos, 0, 0, 0, 0, 0, INT_MAX);

    nextLineStartPos = Fl_Text_Display_skip_lines(self, lineStartPos, 1, 1);
    lineEnd = Fl_Text_Display_line_end(self, nextLineStartPos, 1);
    newPos = handle_vline(self, H_FIND_INDEX_FROM_ZERO, nextLineStartPos, lineEnd - nextLineStartPos, 0, 0, 0, 0, 0, xPos);

    Fl_Text_Display_set_insert_position(self, newPos);
    self->mCursorPreferredXPos = xPos;
    return 1;
}

int Fl_Text_Display_count_lines(const Fl_Text_Display *self, int startPos, int endPos, int startPosIsLineStart) {
    int retLines, retPos, retLineStart, retLineEnd;
    if (!self->mContinuousWrap) return Fl_Text_Buffer_count_lines(self->mBuffer, startPos, endPos);
    wrapped_line_counter(self, self->mBuffer, startPos, endPos, INT_MAX, startPosIsLineStart, 0, &retPos, &retLines, &retLineStart, &retLineEnd, 1);
    return retLines;
}

int Fl_Text_Display_skip_lines(Fl_Text_Display *self, int startPos, int nLines, int startPosIsLineStart) {
    int retLines, retPos, retLineStart, retLineEnd;
    if (!self->mContinuousWrap) return Fl_Text_Buffer_skip_lines(self->mBuffer, startPos, nLines);
    if (nLines == 0) return startPos;
    wrapped_line_counter(self, self->mBuffer, startPos, Fl_Text_Buffer_length(self->mBuffer), nLines, startPosIsLineStart, 0, &retPos, &retLines, &retLineStart, &retLineEnd, 1);
    return retPos;
}

int Fl_Text_Display_line_end(const Fl_Text_Display *self, int startPos, int startPosIsLineStart) {
    int retLines, retPos, retLineStart, retLineEnd;
    if (!self->mContinuousWrap) return Fl_Text_Buffer_line_end(self->mBuffer, startPos);
    if (startPos == Fl_Text_Buffer_length(self->mBuffer)) return startPos;
    wrapped_line_counter(self, self->mBuffer, startPos, Fl_Text_Buffer_length(self->mBuffer), 1, startPosIsLineStart, 0, &retPos, &retLines, &retLineStart, &retLineEnd, 1);
    return retLineEnd;
}

int Fl_Text_Display_line_start(const Fl_Text_Display *self, int pos) {
    int retLines, retPos, retLineStart, retLineEnd;
    if (!self->mContinuousWrap) return Fl_Text_Buffer_line_start(self->mBuffer, pos);
    wrapped_line_counter(self, self->mBuffer, Fl_Text_Buffer_line_start(self->mBuffer, pos), pos, INT_MAX, 1, 0, &retPos, &retLines, &retLineStart, &retLineEnd, 1);
    return retLineStart;
}

int Fl_Text_Display_rewind_lines(Fl_Text_Display *self, int startPos, int nLines) {
    Fl_Text_Buffer *buf = self->mBuffer;
    int pos, lineStart, retLines, retPos, retLineStart, retLineEnd;

    if (!self->mContinuousWrap) return Fl_Text_Buffer_rewind_lines(buf, startPos, nLines);

    pos = startPos;
    for (;;) {
        lineStart = Fl_Text_Buffer_line_start(buf, pos);
        wrapped_line_counter(self, buf, lineStart, pos, INT_MAX, 1, 0, &retPos, &retLines, &retLineStart, &retLineEnd, 0);
        if (retLines > nLines) return Fl_Text_Display_skip_lines(self, lineStart, retLines - nLines, 1);
        nLines -= retLines;
        pos = lineStart - 1;
        if (pos < 0) return 0;
        nLines -= 1;
    }
}

static int fl_isseparator(unsigned int c) { return c != '$' && c != '_' && (isspace((int)c) || ispunct((int)c)); }

void Fl_Text_Display_next_word(Fl_Text_Display *self) {
    int pos = Fl_Text_Display_insert_position(self);
    while (pos < Fl_Text_Buffer_length(self->mBuffer) && !fl_isseparator(Fl_Text_Buffer_char_at(self->mBuffer, pos))) pos = Fl_Text_Buffer_next_char(self->mBuffer, pos);
    while (pos < Fl_Text_Buffer_length(self->mBuffer) && fl_isseparator(Fl_Text_Buffer_char_at(self->mBuffer, pos))) pos = Fl_Text_Buffer_next_char(self->mBuffer, pos);
    Fl_Text_Display_set_insert_position(self, pos);
}

void Fl_Text_Display_previous_word(Fl_Text_Display *self) {
    int pos = Fl_Text_Display_insert_position(self);
    if (pos == 0) return;
    pos = Fl_Text_Buffer_prev_char(self->mBuffer, pos);
    while (pos && fl_isseparator(Fl_Text_Buffer_char_at(self->mBuffer, pos))) pos = Fl_Text_Buffer_prev_char(self->mBuffer, pos);
    while (pos && !fl_isseparator(Fl_Text_Buffer_char_at(self->mBuffer, pos))) pos = Fl_Text_Buffer_prev_char(self->mBuffer, pos);
    if (fl_isseparator(Fl_Text_Buffer_char_at(self->mBuffer, pos))) pos = Fl_Text_Buffer_next_char(self->mBuffer, pos);
    Fl_Text_Display_set_insert_position(self, pos);
}

/* -------------------------------------------------------------------
 * Buffer callbacks
 * ---------------------------------------------------------------- */

static void buffer_predelete_cb(int pos, int nDeleted, void *cbArg) {
    Fl_Text_Display *textD = (Fl_Text_Display *)cbArg;
    if (textD->mContinuousWrap) {
        measure_deleted_lines(textD, pos, nDeleted);
    } else {
        textD->mSuppressResync = 0;
    }
}

static void buffer_modified_cb(int pos, int nInserted, int nDeleted, int nRestyled, const char *deletedText, void *cbArg) {
    Fl_Text_Display *textD = (Fl_Text_Display *)cbArg;
    Fl_Text_Buffer *buf = textD->mBuffer;
    int oldFirstChar = textD->mFirstChar;
    int scrolled, origCursorPos = textD->mCursorPos;
    int wrapModStart = 0, wrapModEnd = 0;
    int linesInserted, linesDeleted, startDispPos, endDispPos;

    if (nInserted != 0 || nDeleted != 0) textD->mCursorPreferredXPos = -1;

    if (textD->mContinuousWrap) {
        find_wrap_range(textD, deletedText, pos, nInserted, nDeleted, &wrapModStart, &wrapModEnd, &linesInserted, &linesDeleted);
    } else {
        linesInserted = nInserted == 0 ? 0 : Fl_Text_Buffer_count_lines(buf, pos, pos + nInserted);
        linesDeleted = nDeleted == 0 ? 0 : countlines(deletedText);
    }

    if (nInserted != 0 || nDeleted != 0) {
        if (textD->mContinuousWrap) {
            update_line_starts(textD, wrapModStart, wrapModEnd - wrapModStart,
                                nDeleted + pos - wrapModStart + (wrapModEnd - (pos + nInserted)), linesInserted, linesDeleted, &scrolled);
        } else {
            update_line_starts(textD, pos, nInserted, nDeleted, linesInserted, linesDeleted, &scrolled);
        }
    } else {
        scrolled = 0;
    }

    if (maintaining_absolute_top_line_number(textD) && (nInserted != 0 || nDeleted != 0)) {
        if (deletedText && (pos + nDeleted < oldFirstChar))
            textD->mAbsTopLineNum += Fl_Text_Buffer_count_lines(buf, pos, pos + nInserted) - countlines(deletedText);
        else if (pos < oldFirstChar)
            reset_absolute_top_line_number(textD);
    }

    textD->mNBufferLines += linesInserted - linesDeleted;

    if (textD->mCursorToHint != NO_HINT) {
        textD->mCursorPos = textD->mCursorToHint;
        textD->mCursorToHint = NO_HINT;
    } else if (textD->mCursorPos > pos) {
        if (textD->mCursorPos < pos + nDeleted) textD->mCursorPos = pos;
        else textD->mCursorPos += nInserted - nDeleted;
    }

    Fl_Widget_resize(&textD->group.widget, textD->group.widget.x, textD->group.widget.y, textD->group.widget.w, textD->group.widget.h);

    if (!Fl_Widget_visible_r(&textD->group.widget)) return;

    if (scrolled) {
        Fl_Widget_set_damage(&textD->group.widget, FL_DAMAGE_EXPOSE);
        if (textD->mStyleBuffer) Fl_Text_Buffer_primary_selection_mut(textD->mStyleBuffer)->selected = 0;
        return;
    }

    startDispPos = textD->mContinuousWrap ? wrapModStart : pos;
    if (origCursorPos == startDispPos && textD->mCursorPos != startDispPos)
        startDispPos = imin(startDispPos, Fl_Text_Buffer_prev_char_clipped(buf, origCursorPos));

    if (linesInserted == linesDeleted) {
        if (nInserted == 0 && nDeleted == 0) {
            endDispPos = pos + nRestyled;
        } else {
            if (textD->mContinuousWrap) endDispPos = wrapModEnd;
            else endDispPos = Fl_Text_Buffer_next_char(buf, Fl_Text_Buffer_line_end(buf, pos + nInserted));
        }
        if (linesInserted > 1) Fl_Widget_set_damage(&textD->group.widget, FL_DAMAGE_EXPOSE);
    } else {
        endDispPos = Fl_Text_Buffer_next_char(buf, textD->mLastChar);
    }

    if (textD->mStyleBuffer) extend_range_for_styles(textD, &startDispPos, &endDispPos);

    Fl_Text_Display_redisplay_range(textD, startDispPos, endDispPos);
}

/* -------------------------------------------------------------------
 * Absolute (non-wrapped) line numbering
 * ---------------------------------------------------------------- */

static void absolute_top_line_number(Fl_Text_Display *self, int oldFirstChar) {
    if (maintaining_absolute_top_line_number(self)) {
        if (self->mFirstChar < oldFirstChar) self->mAbsTopLineNum -= Fl_Text_Buffer_count_lines(self->mBuffer, self->mFirstChar, oldFirstChar);
        else self->mAbsTopLineNum += Fl_Text_Buffer_count_lines(self->mBuffer, oldFirstChar, self->mFirstChar);
    }
}

static void reset_absolute_top_line_number(Fl_Text_Display *self) {
    self->mAbsTopLineNum = 1;
    absolute_top_line_number(self, 0);
}

static int get_absolute_top_line_number(const Fl_Text_Display *self) {
    if (!self->mContinuousWrap) return self->mTopLineNum;
    if (maintaining_absolute_top_line_number(self)) return self->mAbsTopLineNum;
    return 0;
}

static int position_to_line(const Fl_Text_Display *self, int pos, int *lineNum) {
    int i;
    *lineNum = 0;
    if (pos < self->mFirstChar) return 0;
    if (pos > self->mLastChar) {
        if (empty_vlines(self)) {
            if (self->mLastChar < Fl_Text_Buffer_length(self->mBuffer)) {
                if (!position_to_line(self, self->mLastChar, lineNum)) return 0;
                (*lineNum)++;
                return *lineNum <= self->mNVisibleLines - 1;
            } else {
                position_to_line(self, Fl_Text_Buffer_prev_char_clipped(self->mBuffer, self->mLastChar), lineNum);
                return 1;
            }
        }
        return 0;
    }
    for (i = self->mNVisibleLines - 1; i >= 0; i--) {
        if (self->mLineStarts[i] != -1 && pos >= self->mLineStarts[i]) { *lineNum = i; return 1; }
    }
    return 0;
}

/* -------------------------------------------------------------------
 * The "universal pixel machine": handle_vline()
 * ---------------------------------------------------------------- */

static int handle_vline(const Fl_Text_Display *self, int mode, int lineStartPos, int lineLen, int leftChar, int rightChar,
                         int Y, int bottomClip, int leftClip, int rightClip) {
    int i, X, startX, startIndex, style, charStyle;
    char *lineStr;
    char currChar = 0, prevChar = 0;
    int w;

    (void)leftChar; (void)rightChar; (void)bottomClip; (void)leftClip;

    if (lineStartPos == -1) lineStr = NULL;
    else lineStr = Fl_Text_Buffer_text_range(self->mBuffer, lineStartPos, lineStartPos + lineLen);

    if (mode == H_GET_WIDTH) {
        X = 0;
    } else if (mode == H_FIND_INDEX_FROM_ZERO) {
        X = 0;
        mode = H_FIND_INDEX;
    } else {
        X = self->text_area.x - self->mHorizOffset;
    }

    startX = X;
    startIndex = 0;
    if (!lineStr) {
        if (mode == H_DRAW_LINE) {
            style = Fl_Text_Display_position_style(self, lineStartPos, lineLen, -1);
            draw_string(self, style | BG_ONLY_MASK, self->text_area.x, Y, self->text_area.x + self->text_area.w, lineStr, lineLen);
        }
        if (mode == H_FIND_INDEX) return lineStartPos;
        return 0;
    }

    style = Fl_Text_Display_position_style(self, lineStartPos, lineLen, 0);
    for (i = 0; i < lineLen;) {
        int len;
        currChar = lineStr[i];
        len = fl_utf8len1(currChar);
        if (len <= 0) len = 1;
        charStyle = Fl_Text_Display_position_style(self, lineStartPos, lineLen, i);
        if (charStyle != style || currChar == '\t' || prevChar == '\t') {
            w = 0;
            if (prevChar == '\t') {
                int tab = (int)Fl_Text_Display_col_to_x(self, Fl_Text_Buffer_tab_distance(self->mBuffer));
                int xAbs = (mode == H_GET_WIDTH) ? startX : startX + self->mHorizOffset - self->text_area.x;
                w = (((xAbs / tab) + 1) * tab) - xAbs;
                if (mode == H_DRAW_LINE) draw_string(self, style | BG_ONLY_MASK, startX, Y, startX + w, 0, 0);
                if (mode == H_FIND_INDEX && startX + w > rightClip) { free(lineStr); return lineStartPos + startIndex; }
            } else {
                w = (int)string_width(self, lineStr + startIndex, i - startIndex, style);
                if (mode == H_DRAW_LINE) draw_string(self, style, startX, Y, startX + w, lineStr + startIndex, i - startIndex);
                if (mode == H_FIND_INDEX && startX + w > rightClip) {
                    int di = find_x(self, lineStr + startIndex, i - startIndex, style, rightClip - startX);
                    free(lineStr);
                    return lineStartPos + startIndex + di;
                }
            }
            style = charStyle;
            startX += w;
            startIndex = i;
        }
        i += len;
        prevChar = currChar;
    }

    w = 0;
    if (currChar == '\t') {
        int tab = (int)Fl_Text_Display_col_to_x(self, Fl_Text_Buffer_tab_distance(self->mBuffer));
        int xAbs = (mode == H_GET_WIDTH) ? startX : startX + self->mHorizOffset - self->text_area.x;
        w = (((xAbs / tab) + 1) * tab) - xAbs;
        if (mode == H_DRAW_LINE) draw_string(self, style | BG_ONLY_MASK, startX, Y, startX + w, 0, 0);
        if (mode == H_FIND_INDEX) { free(lineStr); return lineStartPos + startIndex + (rightClip - startX > w ? 1 : 0); }
    } else {
        w = (int)string_width(self, lineStr + startIndex, i - startIndex, style);
        if (mode == H_DRAW_LINE) draw_string(self, style, startX, Y, startX + w, lineStr + startIndex, i - startIndex);
        if (mode == H_FIND_INDEX) {
            int di = find_x(self, lineStr + startIndex, i - startIndex, style, rightClip - startX);
            free(lineStr);
            return lineStartPos + startIndex + di;
        }
    }
    if (mode == H_GET_WIDTH) { free(lineStr); return startX + w; }

    startX += w;
    style = Fl_Text_Display_position_style(self, lineStartPos, lineLen, i);
    if (mode == H_DRAW_LINE) draw_string(self, style | BG_ONLY_MASK, startX, Y, self->text_area.x + self->text_area.w, lineStr, lineLen);

    free(lineStr);
    return lineStartPos + lineLen;
}

static int find_x(const Fl_Text_Display *self, const char *s, int len, int style, int x) {
    int i = 0;
    while (i < len) {
        int cl = fl_utf8len1(s[i]);
        int w = (int)string_width(self, s, i + cl, style);
        if (w > x) return i;
        i += cl;
    }
    return len;
}

static void draw_vline(Fl_Text_Display *self, int visLineNum, int leftClip, int rightClip, int leftCharIndex, int rightCharIndex) {
    int Y, lineStartPos, lineLen, fontHeight;

    if (visLineNum < 0 || visLineNum >= self->mNVisibleLines) return;

    fontHeight = self->mMaxsize;
    Y = self->text_area.y + visLineNum * fontHeight;

    lineStartPos = self->mLineStarts[visLineNum];
    lineLen = (lineStartPos == -1) ? 0 : vline_length(self, visLineNum);

    leftClip = imax(self->text_area.x, leftClip);
    rightClip = imin(rightClip, self->text_area.x + self->text_area.w);

    handle_vline(self, H_DRAW_LINE, lineStartPos, lineLen, leftCharIndex, rightCharIndex, Y, Y + fontHeight, leftClip, rightClip);
}

static void draw_string(const Fl_Text_Display *self, int style, int X, int Y, int toX, const char *string, int nChars) {
    const Fl_Widget *self_w = &self->group.widget;
    const Fl_Text_Display_Style *styleRec;
    Fl_Font font = self->textfont_;
    Fl_Fontsize fsize = self->textsize_;
    Fl_Color foreground, background;

    if (style & FILL_MASK) {
        if (style & TEXT_ONLY_MASK) return;
        clear_rect(self, style, X, Y, toX - X, self->mMaxsize);
        return;
    }

    if (style & STYLE_LOOKUP_MASK) {
        int si = (style & STYLE_LOOKUP_MASK) - 'A';
        if (si < 0) si = 0;
        else if (si >= self->mNStyles) si = self->mNStyles - 1;
        styleRec = self->mStyleTable + si;
        font = styleRec->font;
        fsize = styleRec->size;

        if (style & PRIMARY_MASK) {
            background = (Fl_focus() == self_w) ? Fl_Widget_selection_color(self_w) : fl_color_average(self_w->color, Fl_Widget_selection_color(self_w), 0.4f);
        } else if (style & HIGHLIGHT_MASK) {
            background = (Fl_focus() == self_w) ? fl_color_average(self_w->color, Fl_Widget_selection_color(self_w), 0.5f) : fl_color_average(self_w->color, Fl_Widget_selection_color(self_w), 0.6f);
        } else {
            background = self_w->color;
        }
        foreground = (style & PRIMARY_MASK) ? fl_contrast(styleRec->color, background) : styleRec->color;
    } else if (style & PRIMARY_MASK) {
        background = (Fl_focus() == self_w) ? Fl_Widget_selection_color(self_w) : fl_color_average(self_w->color, Fl_Widget_selection_color(self_w), 0.4f);
        foreground = fl_contrast(self->textcolor_, background);
    } else if (style & HIGHLIGHT_MASK) {
        background = (Fl_focus() == self_w) ? fl_color_average(self_w->color, Fl_Widget_selection_color(self_w), 0.5f) : fl_color_average(self_w->color, Fl_Widget_selection_color(self_w), 0.6f);
        foreground = fl_contrast(self->textcolor_, background);
    } else {
        foreground = self->textcolor_;
        background = self_w->color;
    }

    if (!Fl_Widget_active_r(self_w)) {
        foreground = fl_inactive(foreground);
        background = fl_inactive(background);
    }

    if (!(style & TEXT_ONLY_MASK)) {
        fl_color(background);
        fl_rectf(X, Y, toX - X, self->mMaxsize);
    }
    if (!(style & BG_ONLY_MASK)) {
        fl_color(foreground);
        fl_font(font, fsize);
        fl_draw_text(string, nChars, X, Y + self->mMaxsize - fl_descent());
    }
}

static void clear_rect(const Fl_Text_Display *self, int style, int X, int Y, int width, int height) {
    const Fl_Widget *self_w = &self->group.widget;
    Fl_Color c;
    if (width == 0) return;
    if (style & PRIMARY_MASK) {
        c = (Fl_focus() == self_w) ? Fl_Widget_selection_color(self_w) : fl_color_average(self_w->color, Fl_Widget_selection_color(self_w), 0.4f);
    } else if (style & HIGHLIGHT_MASK) {
        c = (Fl_focus() == self_w) ? fl_color_average(self_w->color, Fl_Widget_selection_color(self_w), 0.5f) : fl_color_average(self_w->color, Fl_Widget_selection_color(self_w), 0.6f);
    } else {
        c = self_w->color;
    }
    fl_color(Fl_Widget_active_r(self_w) ? c : fl_inactive(c));
    fl_rectf(X, Y, width, height);
}

static void draw_cursor(Fl_Text_Display *self, int X, int Y) {
    typedef struct { int x1, y1, x2, y2; } Segment;
    Segment segs[5];
    int left, right, cursorWidth, midY;
    int fontWidth = 6;
    int nSegs = 0;
    int fontHeight = self->mMaxsize;
    int bot = Y + fontHeight - 1;
    int k;

    if (X < self->text_area.x - 1 || X > self->text_area.x + self->text_area.w) return;

    cursorWidth = 4;
    left = X - cursorWidth / 2;
    right = left + cursorWidth;

    if (self->mCursorStyle == FL_TEXT_DISPLAY_CARET_CURSOR) {
        midY = bot - fontHeight / 5;
        segs[0].x1 = left; segs[0].y1 = bot; segs[0].x2 = X; segs[0].y2 = midY;
        segs[1].x1 = X; segs[1].y1 = midY; segs[1].x2 = right; segs[1].y2 = bot;
        segs[2].x1 = left; segs[2].y1 = bot; segs[2].x2 = X; segs[2].y2 = midY - 1;
        segs[3].x1 = X; segs[3].y1 = midY - 1; segs[3].x2 = right; segs[3].y2 = bot;
        nSegs = 4;
    } else if (self->mCursorStyle == FL_TEXT_DISPLAY_NORMAL_CURSOR) {
        segs[0].x1 = left; segs[0].y1 = Y; segs[0].x2 = right; segs[0].y2 = Y;
        segs[1].x1 = X; segs[1].y1 = Y; segs[1].x2 = X; segs[1].y2 = bot;
        segs[2].x1 = left; segs[2].y1 = bot; segs[2].x2 = right; segs[2].y2 = bot;
        nSegs = 3;
    } else if (self->mCursorStyle == FL_TEXT_DISPLAY_HEAVY_CURSOR) {
        segs[0].x1 = X - 1; segs[0].y1 = Y; segs[0].x2 = X - 1; segs[0].y2 = bot;
        segs[1].x1 = X; segs[1].y1 = Y; segs[1].x2 = X; segs[1].y2 = bot;
        segs[2].x1 = X + 1; segs[2].y1 = Y; segs[2].x2 = X + 1; segs[2].y2 = bot;
        segs[3].x1 = left; segs[3].y1 = Y; segs[3].x2 = right; segs[3].y2 = Y;
        segs[4].x1 = left; segs[4].y1 = bot; segs[4].x2 = right; segs[4].y2 = bot;
        nSegs = 5;
    } else if (self->mCursorStyle == FL_TEXT_DISPLAY_DIM_CURSOR) {
        midY = Y + fontHeight / 2;
        segs[0].x1 = X; segs[0].y1 = Y; segs[0].x2 = X; segs[0].y2 = Y;
        segs[1].x1 = X; segs[1].y1 = midY; segs[1].x2 = X; segs[1].y2 = midY;
        segs[2].x1 = X; segs[2].y1 = bot; segs[2].x2 = X; segs[2].y2 = bot;
        nSegs = 3;
    } else if (self->mCursorStyle == FL_TEXT_DISPLAY_BLOCK_CURSOR) {
        right = X + fontWidth;
        segs[0].x1 = X; segs[0].y1 = Y; segs[0].x2 = right; segs[0].y2 = Y;
        segs[1].x1 = right; segs[1].y1 = Y; segs[1].x2 = right; segs[1].y2 = bot;
        segs[2].x1 = right; segs[2].y1 = bot; segs[2].x2 = X; segs[2].y2 = bot;
        segs[3].x1 = X; segs[3].y1 = bot; segs[3].x2 = X; segs[3].y2 = Y;
        nSegs = 4;
    } else if (self->mCursorStyle == FL_TEXT_DISPLAY_SIMPLE_CURSOR) {
        segs[0].x1 = X; segs[0].y1 = Y; segs[0].x2 = X; segs[0].y2 = bot;
        segs[1].x1 = X + 1; segs[1].y1 = Y; segs[1].x2 = X + 1; segs[1].y2 = bot;
        nSegs = 2;
    }

    fl_color(self->mCursor_color);
    for (k = 0; k < nSegs; k++) fl_line(segs[k].x1, segs[k].y1, segs[k].x2, segs[k].y2);
}

int Fl_Text_Display_position_style(const Fl_Text_Display *self, int lineStartPos, int lineLen, int lineIndex) {
    Fl_Text_Buffer *buf = self->mBuffer;
    Fl_Text_Buffer *styleBuf = self->mStyleBuffer;
    int pos, style = 0;

    if (lineStartPos == -1 || buf == NULL) return FILL_MASK;

    pos = lineStartPos + imin(lineIndex, lineLen);

    if (lineIndex >= lineLen) {
        style = FILL_MASK;
    } else if (styleBuf != NULL) {
        style = (unsigned char)Fl_Text_Buffer_byte_at(styleBuf, pos);
        if (style == self->mUnfinishedStyle && self->mUnfinishedHighlightCB) {
            (self->mUnfinishedHighlightCB)(pos, self->mHighlightCBArg);
            style = (unsigned char)Fl_Text_Buffer_byte_at(styleBuf, pos);
        }
    }
    if (Fl_Text_Selection_includes(Fl_Text_Buffer_primary_selection(buf), pos)) style |= PRIMARY_MASK;
    if (Fl_Text_Selection_includes(Fl_Text_Buffer_highlight_selection(buf), pos)) style |= HIGHLIGHT_MASK;
    if (Fl_Text_Selection_includes(Fl_Text_Buffer_secondary_selection(buf), pos)) style |= SECONDARY_MASK;
    return style;
}

static double string_width(const Fl_Text_Display *self, const char *string, int length, int style) {
    Fl_Font font;
    Fl_Fontsize fsize;
    if (self->mNStyles && (style & STYLE_LOOKUP_MASK)) {
        int si = (style & STYLE_LOOKUP_MASK) - 'A';
        if (si < 0) si = 0;
        else if (si >= self->mNStyles) si = self->mNStyles - 1;
        font = self->mStyleTable[si].font;
        fsize = self->mStyleTable[si].size;
    } else {
        font = self->textfont_;
        fsize = self->textsize_;
    }
    fl_font(font, fsize);
    return fl_width(string, length);
}

static int xy_to_position(const Fl_Text_Display *self, int X, int Y, int posType) {
    int lineStart, lineLen, fontHeight, visLineNum;
    (void)posType;

    fontHeight = self->mMaxsize;
    visLineNum = (Y - self->text_area.y) / fontHeight;
    if (visLineNum < 0) return self->mFirstChar;
    if (visLineNum >= self->mNVisibleLines) visLineNum = self->mNVisibleLines - 1;

    lineStart = self->mLineStarts[visLineNum];
    if (lineStart == -1) return Fl_Text_Buffer_length(self->mBuffer);

    lineLen = vline_length(self, visLineNum);
    return handle_vline(self, H_FIND_INDEX, lineStart, lineLen, 0, 0, 0, 0, self->text_area.x, X);
}

int Fl_Text_Display_xy_to_position(const Fl_Text_Display *self, int X, int Y, int posType) {
    return xy_to_position(self, X, Y, posType);
}

/* -------------------------------------------------------------------
 * Line-start bookkeeping
 * ---------------------------------------------------------------- */

static void offset_line_starts(Fl_Text_Display *self, int newTopLineNum) {
    int oldTopLineNum = self->mTopLineNum;
    int oldFirstChar = self->mFirstChar;
    int lineDelta = newTopLineNum - oldTopLineNum;
    int nVisLines = self->mNVisibleLines;
    int *lineStarts = self->mLineStarts;
    int i, lastLineNum;
    Fl_Text_Buffer *buf = self->mBuffer;

    if (lineDelta == 0) return;

    lastLineNum = oldTopLineNum + nVisLines - 1;
    if (newTopLineNum < oldTopLineNum && newTopLineNum < -lineDelta) {
        self->mFirstChar = Fl_Text_Display_skip_lines(self, 0, newTopLineNum - 1, 1);
    } else if (newTopLineNum < oldTopLineNum) {
        self->mFirstChar = Fl_Text_Display_rewind_lines(self, self->mFirstChar, -lineDelta);
    } else if (newTopLineNum < lastLineNum) {
        self->mFirstChar = lineStarts[newTopLineNum - oldTopLineNum];
    } else if (newTopLineNum - lastLineNum < self->mNBufferLines - newTopLineNum) {
        self->mFirstChar = Fl_Text_Display_skip_lines(self, lineStarts[nVisLines - 1], newTopLineNum - lastLineNum, 1);
    } else {
        self->mFirstChar = Fl_Text_Display_rewind_lines(self, Fl_Text_Buffer_length(buf), self->mNBufferLines - newTopLineNum + 1);
    }

    if (lineDelta < 0 && -lineDelta < nVisLines) {
        for (i = nVisLines - 1; i >= -lineDelta; i--) lineStarts[i] = lineStarts[i + lineDelta];
        calc_line_starts(self, 0, -lineDelta);
    } else if (lineDelta > 0 && lineDelta < nVisLines) {
        for (i = 0; i < nVisLines - lineDelta; i++) lineStarts[i] = lineStarts[i + lineDelta];
        calc_line_starts(self, nVisLines - lineDelta, nVisLines - 1);
    } else {
        calc_line_starts(self, 0, nVisLines);
    }

    calc_last_char(self);
    self->mTopLineNum = newTopLineNum;
    absolute_top_line_number(self, oldFirstChar);
}

static void update_line_starts(Fl_Text_Display *self, int pos, int charsInserted, int charsDeleted,
                                int linesInserted, int linesDeleted, int *scrolled) {
    int *lineStarts = self->mLineStarts;
    int i, lineOfPos, lineOfEnd, nVisLines = self->mNVisibleLines;
    int charDelta = charsInserted - charsDeleted;
    int lineDelta = linesInserted - linesDeleted;

    if (pos + charsDeleted < self->mFirstChar) {
        self->mTopLineNum += lineDelta;
        for (i = 0; i < nVisLines && lineStarts[i] != -1; i++) lineStarts[i] += charDelta;
        self->mFirstChar += charDelta;
        self->mLastChar += charDelta;
        *scrolled = 0;
        return;
    }

    if (pos < self->mFirstChar) {
        if (position_to_line(self, pos + charsDeleted, &lineOfEnd) && ++lineOfEnd < nVisLines && lineStarts[lineOfEnd] != -1) {
            self->mTopLineNum = imax(1, self->mTopLineNum + lineDelta);
            self->mFirstChar = Fl_Text_Display_rewind_lines(self, lineStarts[lineOfEnd] + charDelta, lineOfEnd);
        } else {
            if (self->mTopLineNum > self->mNBufferLines + lineDelta) {
                self->mTopLineNum = 1;
                self->mFirstChar = 0;
            } else {
                self->mFirstChar = Fl_Text_Display_skip_lines(self, 0, self->mTopLineNum - 1, 1);
            }
        }
        calc_line_starts(self, 0, nVisLines - 1);
        calc_last_char(self);
        *scrolled = 1;
        return;
    }

    if (pos <= self->mLastChar) {
        position_to_line(self, pos, &lineOfPos);
        if (lineDelta == 0) {
            for (i = lineOfPos + 1; i < nVisLines && lineStarts[i] != -1; i++) lineStarts[i] += charDelta;
        } else if (lineDelta > 0) {
            for (i = nVisLines - 1; i >= lineOfPos + lineDelta + 1; i--)
                lineStarts[i] = lineStarts[i - lineDelta] + (lineStarts[i - lineDelta] == -1 ? 0 : charDelta);
        } else {
            for (i = imax(0, lineOfPos + 1); i < nVisLines + lineDelta; i++)
                lineStarts[i] = lineStarts[i - lineDelta] + (lineStarts[i - lineDelta] == -1 ? 0 : charDelta);
        }
        if (linesInserted >= 0) calc_line_starts(self, lineOfPos + 1, lineOfPos + linesInserted);
        if (lineDelta < 0) calc_line_starts(self, nVisLines + lineDelta, nVisLines);
        calc_last_char(self);
        *scrolled = 0;
        return;
    }

    if (empty_vlines(self)) {
        position_to_line(self, pos, &lineOfPos);
        calc_line_starts(self, lineOfPos, lineOfPos + linesInserted);
        calc_last_char(self);
        *scrolled = 0;
        return;
    }

    *scrolled = 0;
}

static void calc_line_starts(Fl_Text_Display *self, int startLine, int endLine) {
    int startPos, bufLen = Fl_Text_Buffer_length(self->mBuffer);
    int line, lineEnd, nextLineStart, nVis = self->mNVisibleLines;
    int *lineStarts = self->mLineStarts;

    if (endLine < 0) endLine = 0;
    if (endLine >= nVis) endLine = nVis - 1;
    if (startLine < 0) startLine = 0;
    if (startLine >= nVis) startLine = nVis - 1;
    if (startLine > endLine) return;

    if (startLine == 0) {
        lineStarts[0] = self->mFirstChar;
        startLine = 1;
    }
    startPos = lineStarts[startLine - 1];

    if (startPos == -1) {
        for (line = startLine; line <= endLine; line++) lineStarts[line] = -1;
        return;
    }

    for (line = startLine; line <= endLine; line++) {
        find_line_end(self, startPos, 1, &lineEnd, &nextLineStart);
        startPos = nextLineStart;
        if (startPos >= bufLen) {
            if (line == 0 || (lineStarts[line - 1] != bufLen && lineEnd != nextLineStart)) {
                lineStarts[line] = bufLen;
                line++;
            }
            break;
        }
        lineStarts[line] = startPos;
    }

    for (; line <= endLine; line++) lineStarts[line] = -1;
}

static void calc_last_char(Fl_Text_Display *self) {
    int i;
    for (i = self->mNVisibleLines - 1; i >= 0 && self->mLineStarts[i] == -1; i--) { }
    self->mLastChar = i < 0 ? 0 : Fl_Text_Display_line_end(self, self->mLineStarts[i], 1);
}

void Fl_Text_Display_scroll(Fl_Text_Display *self, int topLineNum, int horizOffset) {
    self->mTopLineNumHint = topLineNum;
    self->mHorizOffsetHint = horizOffset;
    Fl_Widget_resize(&self->group.widget, self->group.widget.x, self->group.widget.y, self->group.widget.w, self->group.widget.h);
}

static int scroll_(Fl_Text_Display *self, int topLineNum, int horizOffset) {
    if (topLineNum > self->mNBufferLines + 3 - self->mNVisibleLines) topLineNum = self->mNBufferLines + 3 - self->mNVisibleLines;
    if (topLineNum < 1) topLineNum = 1;

    if (horizOffset > longest_vline(self) - self->text_area.w) horizOffset = longest_vline(self) - self->text_area.w;
    if (horizOffset < 0) horizOffset = 0;

    if (self->mHorizOffset == horizOffset && self->mTopLineNum == topLineNum) return 0;

    offset_line_starts(self, topLineNum);
    self->mHorizOffset = horizOffset;

    Fl_Widget_set_damage(&self->group.widget, FL_DAMAGE_EXPOSE);
    return 1;
}

static void update_v_scrollbar(Fl_Text_Display *self) {
    Fl_Scrollbar_set_value_range(self->mVScrollBar, self->mTopLineNum, self->mNVisibleLines, 1, self->mNBufferLines + 2);
    Fl_Scrollbar_set_linesize(self->mVScrollBar, 3);
}

static void update_h_scrollbar(Fl_Text_Display *self) {
    int sliderMax = imax(longest_vline(self), self->text_area.w + self->mHorizOffset);
    Fl_Scrollbar_set_value_range(self->mHScrollBar, self->mHorizOffset, self->text_area.w, 0, sliderMax);
}

static void v_scrollbar_cb(Fl_Widget *w, void *data) {
    Fl_Text_Display *textD = (Fl_Text_Display *)data;
    int v = (int)Fl_Valuator_value((Fl_Valuator *)w);
    if (v == textD->mTopLineNum) return;
    Fl_Text_Display_scroll(textD, v, textD->mHorizOffset);
}

static void h_scrollbar_cb(Fl_Widget *w, void *data) {
    Fl_Text_Display *textD = (Fl_Text_Display *)data;
    int v = (int)Fl_Valuator_value((Fl_Valuator *)w);
    if (v == textD->mHorizOffset) return;
    Fl_Text_Display_scroll(textD, textD->mTopLineNum, v);
}

static void draw_line_numbers(Fl_Text_Display *self, int clearAll) {
    Fl_Widget *self_w = &self->group.widget;
    int Y, line, visLine, lineStart;
    char lineNumString[32];
    int lineHeight = self->mMaxsize;
    int isactive = Fl_Widget_active_r(self_w) ? 1 : 0;
    int hscroll_h, xoff, yoff;
    Fl_Color fgcolor, bgcolor;
    (void)clearAll;

    if (self->mLineNumWidth <= 0 || !Fl_Widget_visible_r(self_w)) return;

    hscroll_h = Fl_Widget_visible(sb_widget(self->mHScrollBar)) ? sb_widget(self->mHScrollBar)->h : 0;
    xoff = fl_box_dx(self_w->box);
    yoff = fl_box_dy(self_w->box) + ((self->scrollbar_align_ & FL_ALIGN_TOP) ? hscroll_h : 0);

    fgcolor = isactive ? self->linenumber_fgcolor_ : fl_inactive(self->linenumber_fgcolor_);
    bgcolor = isactive ? self->linenumber_bgcolor_ : fl_inactive(self->linenumber_bgcolor_);

    fl_push_clip(self_w->x + xoff, self_w->y + yoff, self->mLineNumWidth, self_w->h - fl_box_dw(self_w->box) - hscroll_h);

    fl_color(bgcolor);
    fl_rectf(self_w->x + xoff, self_w->y, self->mLineNumWidth, self_w->h);

    fl_font(self->linenumber_font_, self->linenumber_size_);

    Y = self_w->y + yoff;
    line = get_absolute_top_line_number(self);

    fl_color(fgcolor);
    for (visLine = 0; visLine < self->mNVisibleLines; visLine++) {
        lineStart = self->mLineStarts[visLine];
        if (lineStart != -1 && (lineStart == 0 || Fl_Text_Buffer_char_at(self->mBuffer, lineStart - 1) == '\n')) {
            int xx = self_w->x + xoff + 3, yy = Y + 3, ww = self->mLineNumWidth - 6, hh = lineHeight;
            Fl_Label lbl;
            snprintf(lineNumString, sizeof(lineNumString), self->linenumber_format_ ? self->linenumber_format_ : "%d", line);
            lbl.value = lineNumString; lbl.image = NULL; lbl.deimage = NULL; lbl.type = FL_NORMAL_LABEL;
            lbl.font = self->linenumber_font_; lbl.size = self->linenumber_size_; lbl.color = fgcolor; lbl.align = self->linenumber_align_;
            fl_label_draw(&lbl, xx, yy, ww, hh, self->linenumber_align_);
            line++;
        } else {
            if (visLine == 0) line++;
        }
        Y += lineHeight;
    }
    fl_pop_clip();
}

static int measure_vline(const Fl_Text_Display *self, int visLineNum) {
    int lineLen = vline_length(self, visLineNum);
    int lineStartPos = self->mLineStarts[visLineNum];
    if (lineStartPos < 0 || lineLen == 0) return 0;
    return handle_vline(self, H_GET_WIDTH, lineStartPos, lineLen, 0, 0, 0, 0, 0, 0);
}

static int empty_vlines(const Fl_Text_Display *self) {
    return (self->mNVisibleLines > 0) && (self->mLineStarts[self->mNVisibleLines - 1] == -1);
}

static int vline_length(const Fl_Text_Display *self, int visLineNum) {
    int nextLineStart, lineStartPos, nextLineStartMinus1;

    if (visLineNum < 0 || visLineNum >= self->mNVisibleLines) return 0;

    lineStartPos = self->mLineStarts[visLineNum];
    if (lineStartPos == -1) return 0;

    if (visLineNum + 1 >= self->mNVisibleLines) return self->mLastChar - lineStartPos;

    nextLineStart = self->mLineStarts[visLineNum + 1];
    if (nextLineStart == -1) return self->mLastChar - lineStartPos;

    nextLineStartMinus1 = Fl_Text_Buffer_prev_char(self->mBuffer, nextLineStart);
    if (wrap_uses_character(self, nextLineStartMinus1)) return nextLineStartMinus1 - lineStartPos;
    return nextLineStart - lineStartPos;
}

static void find_wrap_range(Fl_Text_Display *self, const char *deletedText, int pos, int nInserted, int nDeleted,
                             int *modRangeStart, int *modRangeEnd, int *linesInserted, int *linesDeleted) {
    int length, retPos, retLines, retLineStart, retLineEnd;
    Fl_Text_Buffer *deletedTextBuf;
    Fl_Text_Buffer *buf = self->mBuffer;
    int nVisLines = self->mNVisibleLines;
    int *lineStarts = self->mLineStarts;
    int countFrom, countTo = 0, lineStart, adjLineStart, i;
    int visLineNum = 0, nLines = 0;

    if (pos >= self->mFirstChar && pos <= self->mLastChar) {
        for (i = nVisLines - 1; i > 0; i--) if (lineStarts[i] != -1 && pos >= lineStarts[i]) break;
        if (i > 0) { countFrom = lineStarts[i - 1]; visLineNum = i - 1; }
        else countFrom = Fl_Text_Buffer_line_start(buf, pos);
    } else {
        countFrom = Fl_Text_Buffer_line_start(buf, pos);
    }

    lineStart = countFrom;
    *modRangeStart = countFrom;
    for (;;) {
        wrapped_line_counter(self, buf, lineStart, Fl_Text_Buffer_length(buf), 1, 1, 0, &retPos, &retLines, &retLineStart, &retLineEnd, 1);
        if (retPos >= Fl_Text_Buffer_length(buf)) {
            countTo = Fl_Text_Buffer_length(buf);
            *modRangeEnd = countTo;
            if (retPos != retLineEnd) nLines++;
            break;
        } else {
            lineStart = retPos;
        }
        nLines++;
        if (lineStart > pos + nInserted && Fl_Text_Buffer_char_at(buf, Fl_Text_Buffer_prev_char(buf, lineStart)) == '\n') {
            countTo = lineStart;
            *modRangeEnd = lineStart;
            break;
        }

        if (self->mSuppressResync) continue;

        if (lineStart <= pos) {
            while (visLineNum < nVisLines && lineStarts[visLineNum] < lineStart) visLineNum++;
            if (visLineNum < nVisLines && lineStarts[visLineNum] == lineStart) {
                countFrom = lineStart;
                nLines = 0;
                if (visLineNum + 1 < nVisLines && lineStarts[visLineNum + 1] != -1)
                    *modRangeStart = imin(pos, Fl_Text_Buffer_prev_char(buf, lineStarts[visLineNum + 1]));
                else
                    *modRangeStart = countFrom;
            } else {
                *modRangeStart = imin(*modRangeStart, Fl_Text_Buffer_prev_char(buf, lineStart));
            }
        } else if (lineStart > pos + nInserted) {
            adjLineStart = lineStart - nInserted + nDeleted;
            while (visLineNum < nVisLines && lineStarts[visLineNum] < adjLineStart) visLineNum++;
            if (visLineNum < nVisLines && lineStarts[visLineNum] != -1 && lineStarts[visLineNum] == adjLineStart) {
                countTo = Fl_Text_Display_line_end(self, lineStart, 1);
                *modRangeEnd = lineStart;
                break;
            }
        }
    }
    *linesInserted = nLines;

    if (self->mSuppressResync) {
        *linesDeleted = self->mNLinesDeleted;
        self->mSuppressResync = 0;
        return;
    }

    length = (pos - countFrom) + nDeleted + (countTo - (pos + nInserted));
    deletedTextBuf = Fl_Text_Buffer_new(length, 1024);
    Fl_Text_Buffer_copy(deletedTextBuf, buf, countFrom, pos, 0);
    if (nDeleted != 0) Fl_Text_Buffer_insert(deletedTextBuf, pos - countFrom, deletedText);
    Fl_Text_Buffer_copy(deletedTextBuf, buf, pos + nInserted, countTo, pos - countFrom + nDeleted);
    wrapped_line_counter(self, deletedTextBuf, 0, length, INT_MAX, 1, countFrom, &retPos, &retLines, &retLineStart, &retLineEnd, 0);
    Fl_Text_Buffer_free(deletedTextBuf);
    *linesDeleted = retLines;
    self->mSuppressResync = 0;
}

static void measure_deleted_lines(Fl_Text_Display *self, int pos, int nDeleted) {
    int retPos, retLines, retLineStart, retLineEnd;
    Fl_Text_Buffer *buf = self->mBuffer;
    int nVisLines = self->mNVisibleLines;
    int *lineStarts = self->mLineStarts;
    int countFrom, lineStart;
    int nLines = 0, i;

    if (pos >= self->mFirstChar && pos <= self->mLastChar) {
        for (i = nVisLines - 1; i > 0; i--) if (lineStarts[i] != -1 && pos >= lineStarts[i]) break;
        if (i > 0) countFrom = lineStarts[i - 1];
        else countFrom = Fl_Text_Buffer_line_start(buf, pos);
    } else {
        countFrom = Fl_Text_Buffer_line_start(buf, pos);
    }

    lineStart = countFrom;
    for (;;) {
        wrapped_line_counter(self, buf, lineStart, Fl_Text_Buffer_length(buf), 1, 1, 0, &retPos, &retLines, &retLineStart, &retLineEnd, 1);
        if (retPos >= Fl_Text_Buffer_length(buf)) {
            if (retPos != retLineEnd) nLines++;
            break;
        } else {
            lineStart = retPos;
        }
        nLines++;
        if (lineStart > pos + nDeleted && Fl_Text_Buffer_char_at(buf, lineStart - 1) == '\n') break;
    }
    self->mNLinesDeleted = nLines;
    self->mSuppressResync = 1;
}

static void wrapped_line_counter(const Fl_Text_Display *self, Fl_Text_Buffer *buf, int startPos, int maxPos,
                                  int maxLines, int startPosIsLineStart, int styleBufOffset,
                                  int *retPos, int *retLines, int *retLineStart, int *retLineEnd,
                                  int countLastLineMissingNewLine) {
    int lineStart, newLineStart = 0, b, p, colNum, wrapMarginPix;
    int i, foundBreak;
    double width;
    int nLines = 0;
    unsigned int c;

    wrapMarginPix = self->mWrapMarginPix != 0 ? self->mWrapMarginPix : self->text_area.w;

    lineStart = startPosIsLineStart ? startPos : Fl_Text_Display_line_start(self, startPos);

    colNum = 0;
    width = 0;
    for (p = lineStart; p < Fl_Text_Buffer_length(buf); p = Fl_Text_Buffer_next_char(buf, p)) {
        c = Fl_Text_Buffer_char_at(buf, p);

        if (c == '\n') {
            if (p >= maxPos) {
                *retPos = maxPos; *retLines = nLines; *retLineStart = lineStart; *retLineEnd = maxPos;
                return;
            }
            nLines++;
            {
                int p1 = Fl_Text_Buffer_next_char(buf, p);
                if (nLines >= maxLines) {
                    *retPos = p1; *retLines = nLines; *retLineStart = p1; *retLineEnd = p;
                    return;
                }
                lineStart = p1;
            }
            colNum = 0;
            width = 0;
        } else {
            const char *s = Fl_Text_Buffer_address(buf, p);
            colNum++;
            width += measure_proportional_character(self, s, (int)width, p + styleBufOffset);
        }

        if (width > wrapMarginPix) {
            foundBreak = 0;
            for (b = p; b >= lineStart; b = Fl_Text_Buffer_prev_char(buf, b)) {
                c = Fl_Text_Buffer_char_at(buf, b);
                if (c == '\t' || c == ' ') {
                    int iMax;
                    newLineStart = Fl_Text_Buffer_next_char(buf, b);
                    colNum = 0;
                    width = 0;
                    iMax = Fl_Text_Buffer_next_char(buf, p);
                    for (i = Fl_Text_Buffer_next_char(buf, b); i < iMax; i = Fl_Text_Buffer_next_char(buf, i)) {
                        width += measure_proportional_character(self, Fl_Text_Buffer_address(buf, i), (int)width, i + styleBufOffset);
                        colNum++;
                    }
                    foundBreak = 1;
                    break;
                }
            }
            if (b < lineStart) b = lineStart;
            if (!foundBreak) {
                newLineStart = imax(p, Fl_Text_Buffer_next_char(buf, lineStart));
                colNum++;
                if (b >= Fl_Text_Buffer_length(buf)) {
                    width = 0;
                } else {
                    const char *s = Fl_Text_Buffer_address(buf, b);
                    width = measure_proportional_character(self, s, 0, p + styleBufOffset);
                }
            }
            if (p >= maxPos) {
                *retPos = maxPos;
                *retLines = maxPos < newLineStart ? nLines : nLines + 1;
                *retLineStart = maxPos < newLineStart ? lineStart : newLineStart;
                *retLineEnd = maxPos;
                return;
            }
            nLines++;
            if (nLines >= maxLines) {
                *retPos = foundBreak ? Fl_Text_Buffer_next_char(buf, b) : imax(p, Fl_Text_Buffer_next_char(buf, lineStart));
                *retLines = nLines;
                *retLineStart = lineStart;
                *retLineEnd = foundBreak ? b : p;
                return;
            }
            lineStart = newLineStart;
        }
    }

    *retPos = Fl_Text_Buffer_length(buf);
    *retLines = nLines;
    if (countLastLineMissingNewLine && colNum > 0) *retLines = Fl_Text_Buffer_next_char(buf, *retLines);
    *retLineStart = lineStart;
    *retLineEnd = Fl_Text_Buffer_length(buf);
}

static double measure_proportional_character(const Fl_Text_Display *self, const char *s, int xPix, int pos) {
    int charLen, style = 0;
    if (*s == '\t') {
        int tab = (int)Fl_Text_Display_col_to_x(self, Fl_Text_Buffer_tab_distance(self->mBuffer));
        return (double)((((xPix / tab) + 1) * tab) - xPix);
    }
    charLen = fl_utf8len1(*s);
    if (self->mStyleBuffer) style = Fl_Text_Buffer_byte_at(self->mStyleBuffer, pos);
    return string_width(self, s, charLen, style);
}

static void find_line_end(const Fl_Text_Display *self, int startPos, int startPosIsLineStart, int *lineEnd, int *nextLineStart) {
    int retLines, retLineStart;
    if (!self->mContinuousWrap) {
        int le = Fl_Text_Buffer_line_end(self->mBuffer, startPos);
        int ls = Fl_Text_Buffer_next_char(self->mBuffer, le);
        *lineEnd = le;
        *nextLineStart = imin(Fl_Text_Buffer_length(self->mBuffer), ls);
        return;
    }
    wrapped_line_counter(self, self->mBuffer, startPos, Fl_Text_Buffer_length(self->mBuffer), 1, startPosIsLineStart, 0, nextLineStart, &retLines, &retLineStart, lineEnd, 1);
}

static int wrap_uses_character(const Fl_Text_Display *self, int lineEndPos) {
    unsigned int c;
    if (!self->mContinuousWrap || lineEndPos == Fl_Text_Buffer_length(self->mBuffer)) return 1;
    c = Fl_Text_Buffer_char_at(self->mBuffer, lineEndPos);
    return c == '\n' || ((c == '\t' || c == ' ') && lineEndPos + 1 < Fl_Text_Buffer_length(self->mBuffer));
}

static void extend_range_for_styles(Fl_Text_Display *self, int *startpos, int *endpos) {
    Fl_Text_Selection *sel = Fl_Text_Buffer_primary_selection_mut(self->mStyleBuffer);
    int extended = 0;

    if (sel->selected) {
        if (sel->start < *startpos) {
            *startpos = sel->start;
            *startpos = Fl_Text_Buffer_utf8_align(self->mBuffer, *startpos);
            extended = 1;
        }
        if (sel->end > *endpos) {
            *endpos = sel->end;
            *endpos = Fl_Text_Buffer_utf8_align(self->mBuffer, *endpos);
            extended = 1;
        }
    }

    if (extended) *endpos = Fl_Text_Buffer_line_end(self->mBuffer, *endpos) + 1;
}

/* -------------------------------------------------------------------
 * draw()
 * ---------------------------------------------------------------- */

void Fl_Text_Display_draw(Fl_Widget *self_w) {
    Fl_Text_Display *self = (Fl_Text_Display *)self_w;
    Fl_Color bgcolor;
    int start, end, has_selection;

    if (!self->mBuffer) { fl_draw_box(self_w->box, self_w->x, self_w->y, self_w->w, self_w->h, self_w->color); return; }

    fl_push_clip(self_w->x, self_w->y, self_w->w, self_w->h);

    bgcolor = Fl_Widget_active_r(self_w) ? self_w->color : fl_inactive(self_w->color);

    if (self_w->damage & FL_DAMAGE_ALL) {
        int W = self_w->w, H = self_w->h;
        fl_draw_box(self_w->box, self_w->x, self_w->y, W, H, bgcolor);

        if (Fl_Widget_visible(sb_widget(self->mHScrollBar))) W -= self->scrollbar_width_;
        if (Fl_Widget_visible(sb_widget(self->mVScrollBar))) H -= self->scrollbar_width_;

        fl_color(bgcolor);
        fl_rectf(self->text_area.x - LEFT_MARGIN, self->text_area.y - TOP_MARGIN, LEFT_MARGIN, self->text_area.h + TOP_MARGIN + BOTTOM_MARGIN);
        fl_rectf(self->text_area.x + self->text_area.w, self->text_area.y - TOP_MARGIN, RIGHT_MARGIN, self->text_area.h + TOP_MARGIN + BOTTOM_MARGIN);
        fl_rectf(self->text_area.x, self->text_area.y - TOP_MARGIN, self->text_area.w, TOP_MARGIN);
        fl_rectf(self->text_area.x, self->text_area.y + self->text_area.h, self->text_area.w, BOTTOM_MARGIN);

        if (Fl_Widget_visible(sb_widget(self->mVScrollBar)) && Fl_Widget_visible(sb_widget(self->mHScrollBar))) {
            fl_color(FL_GRAY);
            fl_rectf(sb_widget(self->mVScrollBar)->x, sb_widget(self->mHScrollBar)->y, sb_widget(self->mVScrollBar)->w, sb_widget(self->mHScrollBar)->h);
        }
    } else if (self_w->damage & (FL_DAMAGE_SCROLL | FL_DAMAGE_EXPOSE)) {
        fl_push_clip(self->text_area.x - LEFT_MARGIN, self->text_area.y, self->text_area.w + LEFT_MARGIN + RIGHT_MARGIN, self->text_area.h);
        fl_color(bgcolor);
        fl_rectf(self->text_area.x - LEFT_MARGIN, self->mCursorOldY, LEFT_MARGIN, self->mMaxsize);
        fl_rectf(self->text_area.x + self->text_area.w, self->mCursorOldY, RIGHT_MARGIN, self->mMaxsize);
        fl_pop_clip();
    }

    if (self_w->damage & (FL_DAMAGE_ALL | FL_DAMAGE_CHILD)) {
        Fl_Widget_set_damage(sb_widget(self->mVScrollBar), FL_DAMAGE_ALL);
        Fl_Widget_set_damage(sb_widget(self->mHScrollBar), FL_DAMAGE_ALL);
    }
    update_child(sb_widget(self->mVScrollBar));
    update_child(sb_widget(self->mHScrollBar));

    if (self_w->damage & (FL_DAMAGE_ALL | FL_DAMAGE_EXPOSE)) {
        int X, Y, W, H;
        if (fl_clip_box(self->text_area.x, self->text_area.y, self->text_area.w, self->text_area.h, &X, &Y, &W, &H)) {
            draw_text(self, X, Y, W, H);
        } else {
            draw_text(self, self->text_area.x, self->text_area.y, self->text_area.w, self->text_area.h);
        }
    } else if (self_w->damage & FL_DAMAGE_SCROLL) {
        fl_push_clip(self->text_area.x, self->text_area.y, self->text_area.w, self->text_area.h);
        draw_range(self, self->damage_range1_start, self->damage_range1_end);
        if (self->damage_range2_end != -1) draw_range(self, self->damage_range2_start, self->damage_range2_end);
        self->damage_range1_start = self->damage_range1_end = -1;
        self->damage_range2_start = self->damage_range2_end = -1;
        fl_pop_clip();
    }

    has_selection = Fl_Text_Buffer_selection_position(self->mBuffer, &start, &end);
    if ((self_w->damage & (FL_DAMAGE_ALL | FL_DAMAGE_SCROLL | FL_DAMAGE_EXPOSE)) &&
        (!has_selection || self->mCursorPos < start || self->mCursorPos > end) &&
        self->mCursorOn && Fl_focus() == self_w) {
        int X = 0, Y = 0;
        fl_push_clip(self->text_area.x - LEFT_MARGIN, self->text_area.y, self->text_area.w + LEFT_MARGIN + RIGHT_MARGIN, self->text_area.h);
        if (Fl_Text_Display_position_to_xy(self, self->mCursorPos, &X, &Y)) {
            draw_cursor(self, X, Y);
            self->mCursorOldY = Y;
        }
        fl_pop_clip();
    }

    draw_line_numbers(self, 1);

    fl_pop_clip();
}

/* -------------------------------------------------------------------
 * Drag helper shared with Fl_Text_Editor
 * ---------------------------------------------------------------- */

void Fl_Text_Display_drag_me(Fl_Text_Display *d, int pos) {
    if (d->dragType == FL_TEXT_DISPLAY_DRAG_CHAR) {
        if (pos >= d->dragPos) Fl_Text_Buffer_select(d->mBuffer, d->dragPos, pos);
        else Fl_Text_Buffer_select(d->mBuffer, pos, d->dragPos);
        Fl_Text_Display_set_insert_position(d, pos);
    } else if (d->dragType == FL_TEXT_DISPLAY_DRAG_WORD) {
        if (pos >= d->dragPos) {
            Fl_Text_Display_set_insert_position(d, Fl_Text_Display_word_end(d, pos));
            Fl_Text_Buffer_select(d->mBuffer, Fl_Text_Display_word_start(d, d->dragPos), Fl_Text_Display_word_end(d, pos));
        } else {
            Fl_Text_Display_set_insert_position(d, Fl_Text_Display_word_start(d, pos));
            Fl_Text_Buffer_select(d->mBuffer, Fl_Text_Display_word_start(d, pos), Fl_Text_Display_word_end(d, d->dragPos));
        }
    } else if (d->dragType == FL_TEXT_DISPLAY_DRAG_LINE) {
        if (pos >= d->dragPos) {
            Fl_Text_Display_set_insert_position(d, Fl_Text_Buffer_line_end(d->mBuffer, pos) + 1);
            Fl_Text_Buffer_select(d->mBuffer, Fl_Text_Buffer_line_start(d->mBuffer, d->dragPos), Fl_Text_Buffer_line_end(d->mBuffer, pos) + 1);
        } else {
            Fl_Text_Display_set_insert_position(d, Fl_Text_Buffer_line_start(d->mBuffer, pos));
            Fl_Text_Buffer_select(d->mBuffer, Fl_Text_Buffer_line_start(d->mBuffer, pos), Fl_Text_Buffer_line_end(d->mBuffer, d->dragPos) + 1);
        }
    }
}

static void scroll_timer_cb(void *user_data) {
    Fl_Text_Display *w = (Fl_Text_Display *)user_data;
    int pos;
    switch (scroll_direction) {
        case 1:
            Fl_Text_Display_scroll(w, w->mTopLineNum, w->mHorizOffset + scroll_amount);
            pos = xy_to_position(w, w->text_area.x + w->text_area.w, scroll_y, FL_TEXT_DISPLAY_CURSOR_POS);
            break;
        case 2:
            Fl_Text_Display_scroll(w, w->mTopLineNum, w->mHorizOffset + scroll_amount);
            pos = xy_to_position(w, w->text_area.x, scroll_y, FL_TEXT_DISPLAY_CURSOR_POS);
            break;
        case 3:
            Fl_Text_Display_scroll(w, w->mTopLineNum + scroll_amount, w->mHorizOffset);
            pos = xy_to_position(w, scroll_x, w->text_area.y, FL_TEXT_DISPLAY_CURSOR_POS);
            break;
        case 4:
            Fl_Text_Display_scroll(w, w->mTopLineNum + scroll_amount, w->mHorizOffset);
            pos = xy_to_position(w, scroll_x, w->text_area.y + w->text_area.h, FL_TEXT_DISPLAY_CURSOR_POS);
            break;
        default:
            return;
    }
    Fl_Text_Display_drag_me(w, pos);
    Fl_repeat_timeout(.1, scroll_timer_cb, user_data);
}

/* -------------------------------------------------------------------
 * handle()
 * ---------------------------------------------------------------- */

int Fl_Text_Display_handle(Fl_Widget *self_w, int event) {
    Fl_Text_Display *self = (Fl_Text_Display *)self_w;

    if (!self->mBuffer) return 0;

    if (!Fl_event_inside_rect(self->text_area.x, self->text_area.y, self->text_area.w, self->text_area.h) &&
        !self->dragging && event != FL_LEAVE && event != FL_ENTER && event != FL_MOVE &&
        event != FL_FOCUS && event != FL_UNFOCUS && event != FL_KEYBOARD && event != FL_KEYUP) {
        return Fl_Group_handle(self_w, event);
    }

    switch (event) {
        case FL_ENTER:
        case FL_MOVE:
            return Fl_Widget_active_r(self_w) ? 1 : 0;

        case FL_LEAVE:
        case FL_HIDE:
            return Fl_Widget_active_r(self_w) ? 1 : 0;

        case FL_PUSH: {
            int pos;
            if (Fl_focus() != self_w) { Fl_set_focus(self_w); Fl_Widget_handle(self_w, FL_FOCUS); }
            if (Fl_Group_handle(self_w, event)) return 1;
            if (Fl_event_state() & FL_SHIFT) return Fl_Widget_handle(self_w, FL_DRAG);
            self->dragging = 1;
            pos = xy_to_position(self, Fl_event_x(), Fl_event_y(), FL_TEXT_DISPLAY_CURSOR_POS);
            self->dragPos = pos;
            if (Fl_Text_Selection_includes(Fl_Text_Buffer_primary_selection(self->mBuffer), pos)) {
                self->dragType = FL_TEXT_DISPLAY_DRAG_START_DND;
                return 1;
            }
            self->dragType = Fl_event_clicks();
            if (self->dragType == FL_TEXT_DISPLAY_DRAG_CHAR) {
                Fl_Text_Buffer_unselect(self->mBuffer);
            } else if (self->dragType == FL_TEXT_DISPLAY_DRAG_WORD) {
                Fl_Text_Buffer_select(self->mBuffer, Fl_Text_Display_word_start(self, pos), Fl_Text_Display_word_end(self, pos));
                self->dragPos = Fl_Text_Display_word_start(self, pos);
            }
            if (Fl_Text_Buffer_selected(self->mBuffer)) Fl_Text_Display_set_insert_position(self, Fl_Text_Buffer_primary_selection(self->mBuffer)->end);
            else Fl_Text_Display_set_insert_position(self, pos);
            Fl_Text_Display_show_insert_position(self);
            return 1;
        }

        case FL_DRAG: {
            int X, Y, pos;
            if (self->dragType == FL_TEXT_DISPLAY_DRAG_NONE) return 1;
            if (self->dragType == FL_TEXT_DISPLAY_DRAG_START_DND) return 1; /* no DND support, see header */
            X = Fl_event_x(); Y = Fl_event_y();
            pos = Fl_Text_Display_insert_position(self);
            if (Y < self->text_area.y) {
                scroll_x = X;
                scroll_amount = (Y - self->text_area.y) / 5 - 1;
                if (!scroll_direction) Fl_add_timeout(.01, scroll_timer_cb, self);
                scroll_direction = 3;
            } else if (Y >= self->text_area.y + self->text_area.h) {
                scroll_x = X;
                scroll_amount = (Y - self->text_area.y - self->text_area.h) / 5 + 1;
                if (!scroll_direction) Fl_add_timeout(.01, scroll_timer_cb, self);
                scroll_direction = 4;
            } else if (X < self->text_area.x) {
                scroll_y = Y;
                scroll_amount = (X - self->text_area.x) / 2 - 1;
                if (!scroll_direction) Fl_add_timeout(.01, scroll_timer_cb, self);
                scroll_direction = 2;
            } else if (X >= self->text_area.x + self->text_area.w) {
                scroll_y = Y;
                scroll_amount = (X - self->text_area.x - self->text_area.w) / 2 + 1;
                if (!scroll_direction) Fl_add_timeout(.01, scroll_timer_cb, self);
                scroll_direction = 1;
            } else {
                if (scroll_direction) { Fl_remove_timeout(scroll_timer_cb, self); scroll_direction = 0; }
                pos = xy_to_position(self, X, Y, FL_TEXT_DISPLAY_CURSOR_POS);
                pos = Fl_Text_Buffer_next_char(self->mBuffer, pos);
            }
            Fl_Text_Display_drag_me(self, pos);
            return 1;
        }

        case FL_RELEASE: {
            char *copy;
            if (Fl_event_is_click() && !Fl_event_clicks() &&
                Fl_Text_Selection_includes(Fl_Text_Buffer_primary_selection(self->mBuffer), self->dragPos) &&
                !(Fl_event_state() & FL_SHIFT)) {
                Fl_Text_Buffer_unselect(self->mBuffer);
                Fl_Text_Display_set_insert_position(self, self->dragPos);
                return 1;
            } else if (Fl_event_clicks() == FL_TEXT_DISPLAY_DRAG_LINE && Fl_event_button() == FL_LEFT_MOUSE) {
                Fl_Text_Buffer_select(self->mBuffer, Fl_Text_Buffer_line_start(self->mBuffer, self->dragPos), Fl_Text_Buffer_next_char(self->mBuffer, Fl_Text_Buffer_line_end(self->mBuffer, self->dragPos)));
                self->dragPos = Fl_Text_Display_line_start(self, self->dragPos);
                self->dragType = FL_TEXT_DISPLAY_DRAG_CHAR;
            } else {
                self->dragging = 0;
                if (scroll_direction) { Fl_remove_timeout(scroll_timer_cb, self); scroll_direction = 0; }
                self->dragType = FL_TEXT_DISPLAY_DRAG_CHAR;
            }
            copy = Fl_Text_Buffer_selection_text(self->mBuffer);
            if (*copy) Fl_copy(copy, (int)strlen(copy), 0);
            free(copy);
            return 1;
        }

        case FL_MOUSEWHEEL:
            if (Fl_event_dy()) return Fl_Widget_handle(sb_widget(self->mVScrollBar), event);
            return Fl_Widget_handle(sb_widget(self->mHScrollBar), event);

        case FL_UNFOCUS:
        case FL_FOCUS:
            if (Fl_Text_Buffer_selected(self->mBuffer)) {
                int start, end;
                if (Fl_Text_Buffer_selection_position(self->mBuffer, &start, &end)) Fl_Text_Display_redisplay_range(self, start, end);
            }
            if (Fl_Text_Buffer_secondary_selected(self->mBuffer)) {
                int start, end;
                if (Fl_Text_Buffer_secondary_selection_position(self->mBuffer, &start, &end)) Fl_Text_Display_redisplay_range(self, start, end);
            }
            if (Fl_Text_Buffer_highlighted(self->mBuffer)) {
                int start, end;
                if (Fl_Text_Buffer_highlight_position(self->mBuffer, &start, &end)) Fl_Text_Display_redisplay_range(self, start, end);
            }
            return 1;

        case FL_KEYBOARD:
            if ((Fl_event_state() & FL_CTRL) && Fl_event_key() == 'c') {
                char *copy;
                if (!Fl_Text_Buffer_selected(self->mBuffer)) return 1;
                copy = Fl_Text_Buffer_selection_text(self->mBuffer);
                if (*copy) Fl_copy(copy, (int)strlen(copy), 1);
                free(copy);
                return 1;
            }
            if ((Fl_event_state() & FL_CTRL) && Fl_event_key() == 'a') {
                char *copy;
                Fl_Text_Buffer_select(self->mBuffer, 0, Fl_Text_Buffer_length(self->mBuffer));
                copy = Fl_Text_Buffer_selection_text(self->mBuffer);
                if (*copy) Fl_copy(copy, (int)strlen(copy), 0);
                free(copy);
                return 1;
            }
            if (Fl_Widget_handle(sb_widget(self->mVScrollBar), event)) return 1;
            if (Fl_Widget_handle(sb_widget(self->mHScrollBar), event)) return 1;
            break;

        case FL_SHORTCUT:
            if (!(self->shortcut_ ? Fl_test_shortcut((Fl_Shortcut)self->shortcut_) : Fl_Widget_test_shortcut(self_w))) return 0;
            if (Fl_visible_focus() && Fl_Widget_handle(self_w, FL_FOCUS)) { Fl_set_focus(self_w); return 1; }
            break;

        default:
            break;
    }

    return 0;
}

double Fl_Text_Display_x_to_col(const Fl_Text_Display *self, double x) {
    Fl_Text_Display *mself = (Fl_Text_Display *)self;
    if (!self->mColumnScale) mself->mColumnScale = string_width(self, "Mitg", 4, 'A') / 4.0;
    return (x / self->mColumnScale) + 0.5;
}

double Fl_Text_Display_col_to_x(const Fl_Text_Display *self, double col) {
    if (!self->mColumnScale) Fl_Text_Display_x_to_col(self, 0);
    return col * self->mColumnScale;
}

void Fl_Text_Display_set_textfont(Fl_Text_Display *self, Fl_Font s) { self->textfont_ = s; self->mColumnScale = 0; }
void Fl_Text_Display_set_textsize(Fl_Text_Display *self, Fl_Fontsize s) { self->textsize_ = s; self->mColumnScale = 0; }
