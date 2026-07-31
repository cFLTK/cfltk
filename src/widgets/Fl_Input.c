/*
 * cfltk - Fl_Input.c
 * See include/cfltk/Fl_Input.h for the class-conversion notes.
 * Translated from src/Fl_Input_.cxx and src/Fl_Input.cxx.
 */
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "cfltk/Fl_Input.h"
#include "cfltk/Fl.h"
#include "cfltk/fl_draw.h"

#define DISPLAY_BUF_CAP 2048

const Fl_WidgetOps fl_input_ops = {
    Fl_Input_draw,
    Fl_Input_handle,
    Fl_Input_resize,
    NULL, NULL,
    Fl_Input_destroy,
    NULL, NULL
};

/* Horizontal pixel column preserved across consecutive Up/Down presses,
 * exactly like upstream's file-static up_down_pos/was_up_down (shared
 * across all inputs since only one can have focus at a time). */
static double g_up_down_pos = 0.0;
static int g_was_up_down = 0;

/* ---------------------------------------------------------------------
 * Construction / destruction
 * ------------------------------------------------------------------ */

void Fl_Input_init(Fl_Input *self, int x, int y, int w, int h, const char *label) {
    Fl_Widget_init(&self->widget, &fl_input_ops, x, y, w, h, label);
    self->widget.box = FL_DOWN_BOX;
    self->widget.color = FL_BACKGROUND2_COLOR;
    self->widget.color2 = FL_SELECTION_COLOR;
    Fl_Widget_set_align(&self->widget, FL_ALIGN_LEFT);

    self->buffer = (char *)malloc(1);
    self->buffer[0] = '\0';
    self->size_ = 0;
    self->bufsize = 1;
    self->position_ = 0;
    self->mark_ = 0;
    self->xscroll_ = 0;
    self->yscroll_ = 0;
    self->maximum_size_ = 32767;
    self->shortcut_ = 0;
    self->textfont_ = FL_HELVETICA;
    self->textsize_ = FL_NORMAL_SIZE;
    self->textcolor_ = FL_FOREGROUND_COLOR;
    self->cursor_color_ = FL_FOREGROUND_COLOR;
    self->tab_nav_ = 1;

    self->widget.flags |= FL_WIDGET_SHORTCUT_LABEL;
}

Fl_Input *Fl_Input_new(int x, int y, int w, int h, const char *label) {
    Fl_Input *self = (Fl_Input *)malloc(sizeof(Fl_Input));
    Fl_Input_init(self, x, y, w, h, label);
    return self;
}

void Fl_Input_destroy(Fl_Widget *self_w) {
    Fl_Input *self = (Fl_Input *)self_w;
    free(self->buffer);
    self->buffer = NULL;
    Fl_Widget_base_destroy(self_w);
}

void Fl_Input_resize(Fl_Widget *self_w, int x, int y, int w, int h) {
    Fl_Input *self = (Fl_Input *)self_w;
    if (w != self_w->w) self->xscroll_ = 0;
    if (h != self_w->h) self->yscroll_ = 0;
    Fl_Widget_default_resize(self_w, x, y, w, h);
}

/* ---------------------------------------------------------------------
 * Value
 * ------------------------------------------------------------------ */

static void ensure_capacity(Fl_Input *self, int needed_len) {
    if (needed_len + 1 <= self->bufsize) return;
    self->bufsize = self->bufsize ? self->bufsize * 2 : 16;
    while (self->bufsize < needed_len + 1) self->bufsize *= 2;
    self->buffer = (char *)realloc(self->buffer, (size_t)self->bufsize);
}

int Fl_Input_set_value(Fl_Input *self, const char *str, int len) {
    Fl_Widget_clear_changed(&self->widget);
    if (len < 0) len = 0;
    if (len == self->size_ && (len == 0 || memcmp(str, self->buffer, (size_t)len) == 0)) return 0;

    ensure_capacity(self, len);
    if (len) memcpy(self->buffer, str, (size_t)len);
    self->buffer[len] = '\0';
    self->size_ = len;
    self->xscroll_ = self->yscroll_ = 0;

    Fl_Input_set_position(self, Fl_Input_readonly(self) ? 0 : self->size_);
    Fl_Widget_redraw(&self->widget);
    return 1;
}

/* ---------------------------------------------------------------------
 * position()/mark()
 * ------------------------------------------------------------------ */

int Fl_Input_set_position_mark(Fl_Input *self, int p, int m) {
    g_was_up_down = 0;
    if (p < 0) p = 0;
    if (p > self->size_) p = self->size_;
    if (m < 0) m = 0;
    if (m > self->size_) m = self->size_;
    if (p == self->position_ && m == self->mark_) return 0;
    self->position_ = p;
    self->mark_ = m;
    Fl_Widget_redraw(&self->widget);
    return 1;
}

/* ---------------------------------------------------------------------
 * replace() -- all buffer mutation goes through here.
 * ------------------------------------------------------------------ */

int Fl_Input_replace(Fl_Input *self, int b, int e, const char *text, int ilen) {
    int nchars;
    g_was_up_down = 0;

    if (b < 0) b = 0;
    if (e < 0) e = 0;
    if (b > self->size_) b = self->size_;
    if (e > self->size_) e = self->size_;
    if (e < b) { int t = b; b = e; e = t; }

    if (text && !ilen) ilen = (int)strlen(text);
    if (e <= b && !ilen) return 0;

    nchars = self->size_ - (e - b);
    if (nchars + ilen > self->maximum_size_) {
        ilen = self->maximum_size_ - nchars;
        if (ilen < 0) ilen = 0;
    }

    ensure_capacity(self, self->size_ - (e - b) + ilen);

    if (e > b) {
        memmove(self->buffer + b, self->buffer + e, (size_t)(self->size_ - e + 1));
        self->size_ -= (e - b);
    }
    if (ilen) {
        memmove(self->buffer + b + ilen, self->buffer + b, (size_t)(self->size_ - b + 1));
        memcpy(self->buffer + b, text, (size_t)ilen);
        self->size_ += ilen;
    }

    self->mark_ = self->position_ = b + ilen;

    Fl_Widget_set_changed(&self->widget);
    Fl_Widget_redraw(&self->widget);
    if (Fl_Widget_when(&self->widget) & FL_WHEN_CHANGED) Fl_Widget_do_callback(&self->widget);
    return 1;
}

int Fl_Input_copy(Fl_Input *self, int clipboard) {
    int b = self->position_, e = self->mark_;
    if (b == e) return 0;
    if (b > e) { int t = b; b = e; e = t; }
    if (Fl_Input_input_type(self) == FL_SECRET_INPUT) return 0;
    Fl_copy(self->buffer + b, e - b, clipboard);
    return 1;
}

/* ---------------------------------------------------------------------
 * Word / line navigation
 * ------------------------------------------------------------------ */

static int is_word_char(char c) { return (c & 0x80) || isalnum((unsigned char)c) || strchr("#%-@_~", c) != NULL; }

int Fl_Input_word_start(const Fl_Input *self, int i) {
    if (Fl_Input_input_type(self) == FL_SECRET_INPUT) return 0;
    while (i > 0 && !is_word_char(self->buffer[i - 1])) i--;
    while (i > 0 && is_word_char(self->buffer[i - 1])) i--;
    return i;
}

int Fl_Input_word_end(const Fl_Input *self, int i) {
    if (Fl_Input_input_type(self) == FL_SECRET_INPUT) return self->size_;
    while (i < self->size_ && !is_word_char(self->buffer[i])) i++;
    while (i < self->size_ && is_word_char(self->buffer[i])) i++;
    return i;
}

int Fl_Input_line_start(const Fl_Input *self, int i) {
    if (Fl_Input_input_type(self) != FL_MULTILINE_INPUT) return 0;
    while (i > 0 && self->buffer[i - 1] != '\n') i--;
    return i;
}

int Fl_Input_line_end(const Fl_Input *self, int i) {
    if (Fl_Input_input_type(self) != FL_MULTILINE_INPUT) return self->size_;
    while (i < self->size_ && self->buffer[i] != '\n') i++;
    return i;
}

/* ---------------------------------------------------------------------
 * Rendering helpers
 * ------------------------------------------------------------------ */

/* Byte length of the UTF-8 sequence starting at c, treating each
 * multi-byte character as one navigable unit for click positioning and
 * line layout. Does not validate continuation bytes (see Fl_Input.h's
 * "cursor motion does not snap to UTF-8 boundaries" note): malformed
 * input degrades to per-byte stepping, never a crash. */
static int utf8_seq_len(unsigned char c) {
    if ((c & 0x80) == 0) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1;
}

/* Fills out[0..n) with the text that should actually be rendered for
 * buffer[b..b+n) -- identity, except FL_SECRET_INPUT masks every byte
 * with '*' (one byte in, one '*' out: see Fl_Input.h's secret-input
 * masking note -- multi-byte characters become multiple stars). */
static int display_bytes(const Fl_Input *self, int b, int n, char *out, int outcap) {
    if (n > outcap) n = outcap;
    if (Fl_Input_input_type(self) == FL_SECRET_INPUT) {
        memset(out, '*', (size_t)n);
    } else {
        memcpy(out, self->buffer + b, (size_t)n);
    }
    return n;
}

static double line_width(const Fl_Input *self, int b, int n) {
    char buf[DISPLAY_BUF_CAP];
    int dn = display_bytes(self, b, n, buf, sizeof(buf));
    return fl_width(buf, dn);
}

/* Finds the byte offset in [line_b,line_e) whose glyph boundary is
 * closest to target_x pixels from the start of the line. */
static int offset_for_x(const Fl_Input *self, int line_b, int line_e, double target_x) {
    int i = line_b;
    double prev_w = 0.0;
    if (target_x <= 0.0) return line_b;
    while (i < line_e) {
        int clen = utf8_seq_len((unsigned char)self->buffer[i]);
        double w, mid;
        if (i + clen > line_e) clen = line_e - i;
        w = line_width(self, line_b, i + clen - line_b);
        mid = (prev_w + w) / 2.0;
        if (target_x < mid) return i;
        prev_w = w;
        i += clen;
    }
    return line_e;
}

/* ---------------------------------------------------------------------
 * Drawing
 * ------------------------------------------------------------------ */

static void drawtext(Fl_Input *self, int X, int Y, int W, int H) {
    Fl_Widget *self_w = &self->widget;
    int height, desc;
    int selstart, selend;
    int is_focused = (Fl_focus() == self_w || Fl_pushed() == self_w);
    int line_b, cur_line_b = -1, cur_line_e = -1;
    int cury = 0;

    fl_font(self->textfont_, self->textsize_);
    height = fl_height();
    desc = height - fl_descent();

    if (is_focused) {
        selstart = self->position_ < self->mark_ ? self->position_ : self->mark_;
        selend = self->position_ < self->mark_ ? self->mark_ : self->position_;
    } else {
        selstart = selend = 0;
    }

    /* Locate the cursor's line first, to (re)compute scroll offsets
     * before anything is actually drawn -- mirrors upstream's two-pass
     * "measure, then draw" structure without its minimal-update path. */
    for (line_b = 0; ; ) {
        int line_e = Fl_Input_line_end(self, line_b);
        if (self->position_ >= line_b && self->position_ <= line_e) {
            cur_line_b = line_b;
            cur_line_e = line_e;
            break;
        }
        if (line_e >= self->size_) { cur_line_b = line_b; cur_line_e = line_e; break; }
        line_b = line_e + 1;
    }
    {
        double curx = line_width(self, cur_line_b, self->position_ - cur_line_b);
        int line_index = 0, b;
        for (b = 0; b < cur_line_b; ) {
            int e = Fl_Input_line_end(self, b);
            line_index++;
            if (e >= self->size_) break;
            b = e + 1;
        }
        cury = line_index * height;

        if (!g_was_up_down) g_up_down_pos = curx;

        if ((int)curx > self->xscroll_ + W - 2) self->xscroll_ = (int)curx - W + 2;
        else if ((int)curx < self->xscroll_) self->xscroll_ = (int)curx;
        if (self->xscroll_ < 0) self->xscroll_ = 0;

        if (Fl_Input_input_type(self) == FL_MULTILINE_INPUT) {
            if (cury < self->yscroll_) self->yscroll_ = cury;
            else if (cury > self->yscroll_ + H - height) self->yscroll_ = cury - H + height;
            if (self->yscroll_ < 0) self->yscroll_ = 0;
        } else {
            self->yscroll_ = -(H - height) / 2;
        }
    }

    fl_draw_box(self_w->box, self_w->x, self_w->y, self_w->w, self_w->h, self_w->color);
    fl_push_clip(X, Y, W, H);

    {
        Fl_Color tc = Fl_Widget_active_r(self_w) ? self->textcolor_ : fl_inactive(self->textcolor_);
        int ypos = -self->yscroll_;
        int b2 = 0;

        for (; ypos < H; ) {
            int e = Fl_Input_line_end(self, b2);
            int n = e - b2;
            char dbuf[DISPLAY_BUF_CAP];
            int dn = display_bytes(self, b2, n, dbuf, sizeof(dbuf));
            float xpos = (float)(X - self->xscroll_ + 1);

            if (ypos > -height) {
                if (selstart < selend && selstart <= e && selend >= b2) {
                    int s0 = selstart < b2 ? b2 : selstart;
                    int s1 = selend > e ? e : selend;
                    /* Clamp into [0,dn]: dbuf may be shorter than the
                     * real line if it hit DISPLAY_BUF_CAP. */
                    int r0 = s0 - b2, r1 = s1 - b2;
                    double x0, x1;
                    if (r0 < 0) r0 = 0;
                    if (r0 > dn) r0 = dn;
                    if (r1 < 0) r1 = 0;
                    if (r1 > dn) r1 = dn;
                    x0 = xpos + line_width(self, b2, r0);
                    x1 = xpos + line_width(self, b2, r1);

                    fl_color(tc);
                    fl_draw_text(dbuf, r0, (int)xpos, Y + ypos + desc);

                    fl_color(self_w->color2);
                    fl_rectf((int)(x0 + 0.5), Y + ypos, (int)(x1 - x0 + 0.5), height);
                    fl_color(fl_contrast(self->textcolor_, self_w->color2));
                    fl_draw_text(dbuf + r0, r1 - r0, (int)x0, Y + ypos + desc);

                    fl_color(tc);
                    fl_draw_text(dbuf + r1, dn - r1, (int)x1, Y + ypos + desc);
                } else {
                    fl_color(tc);
                    fl_draw_text(dbuf, dn, (int)xpos, Y + ypos + desc);
                }

                if (Fl_focus() == self_w && selstart == selend && self->position_ >= b2 && self->position_ <= e) {
                    int curx = (int)line_width(self, b2, self->position_ - b2);
                    fl_color(self->cursor_color_);
                    fl_rectf((int)(xpos + curx + 0.5), Y + ypos, 2, height);
                }
            }

            ypos += height;
            if (e >= self->size_) break;
            b2 = e + 1;
        }
    }

    fl_pop_clip();
    (void)cur_line_e;
}

void Fl_Input_draw(Fl_Widget *self_w) {
    Fl_Input *self = (Fl_Input *)self_w;
    uchar b = self_w->box;

    if (Fl_Input_input_type(self) == FL_HIDDEN_INPUT) return;
    drawtext(self, self_w->x + fl_box_dx(b), self_w->y + fl_box_dy(b),
             self_w->w - fl_box_dw(b), self_w->h - fl_box_dh(b));
}

/* ---------------------------------------------------------------------
 * Mouse
 * ------------------------------------------------------------------ */

static void handle_mouse(Fl_Input *self, int X, int Y, int drag) {
    int line_b, line_e, newpos, newmark;
    int clicks = Fl_event_clicks();

    if (Fl_Input_input_type(self) == FL_MULTILINE_INPUT) {
        int target_line = (Fl_event_y() - Y + self->yscroll_) / fl_height();
        int line_index = 0;
        line_b = 0;
        for (;;) {
            line_e = Fl_Input_line_end(self, line_b);
            if (line_index >= target_line || line_e >= self->size_) break;
            line_b = line_e + 1;
            line_index++;
        }
    } else {
        line_b = 0;
        line_e = self->size_;
    }

    newpos = offset_for_x(self, line_b, line_e, (double)(Fl_event_x() - X + self->xscroll_));
    newmark = drag ? self->mark_ : newpos;

    if (clicks >= 2) {
        newpos = Fl_Input_line_end(self, newpos);
        newmark = Fl_Input_line_start(self, newmark);
    } else if (clicks == 1) {
        newpos = Fl_Input_word_end(self, newpos);
        newmark = Fl_Input_word_start(self, newmark);
    }

    Fl_Input_set_position_mark(self, newpos, newmark);
}

/* ---------------------------------------------------------------------
 * Keyboard
 * ------------------------------------------------------------------ */

static void shift_position(Fl_Input *self, int p, int shift) {
    Fl_Input_set_position_mark(self, p, shift ? self->mark_ : p);
}

static void move_up_down(Fl_Input *self, int dir, int shift) {
    int ls = Fl_Input_line_start(self, self->position_);
    int le = Fl_Input_line_end(self, self->position_);
    int target_b, target_e, newpos;
    double curx;

    fl_font(self->textfont_, self->textsize_);
    curx = line_width(self, ls, self->position_ - ls);
    if (!g_was_up_down) g_up_down_pos = curx;

    if (dir < 0) {
        if (ls == 0) return;
        target_e = ls - 1;
        target_b = Fl_Input_line_start(self, target_e);
    } else {
        if (le >= self->size_) return;
        target_b = le + 1;
        target_e = Fl_Input_line_end(self, target_b);
    }
    newpos = offset_for_x(self, target_b, target_e, g_up_down_pos);
    Fl_Input_set_position_mark(self, newpos, shift ? self->mark_ : newpos);
    g_was_up_down = 1;
}

/* Context-sensitive, translated from the FL_INT_INPUT/FL_FLOAT_INPUT
 * branch of Fl_Input::handle_key(): hex letters are only legal once the
 * buffer already starts with "0x"/"0X" (so plain "abc" can't be typed
 * into an int field just because a-f look like hex digits), and +/- are
 * only legal at the very start of the field. */
static int legal_numeric_char(const Fl_Input *self, char c) {
    int type = Fl_Input_input_type(self);
    int ip = self->position_ < self->mark_ ? self->position_ : self->mark_;

    if (type == FL_INT_INPUT) {
        if (!ip && (c == '+' || c == '-')) return 1;
        if (c >= '0' && c <= '9') return 1;
        if (ip == 1 && self->size_ >= 1 && self->buffer[0] == '0' && (c == 'x' || c == 'X')) return 1;
        if (ip > 1 && self->size_ >= 2 && self->buffer[0] == '0' && (self->buffer[1] == 'x' || self->buffer[1] == 'X') &&
            ((c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) return 1;
        return 0;
    }
    if (type == FL_FLOAT_INPUT) {
        if (!ip && (c == '+' || c == '-')) return 1;
        if (c >= '0' && c <= '9') return 1;
        return strchr(".eE+-", c) != NULL;
    }
    return 1;
}

static int handle_key(Fl_Input *self) {
    Fl_Widget *self_w = &self->widget;
    int key = Fl_event_key();
    int state = Fl_event_state();
    int shift = state & FL_SHIFT;
    int ctrl = state & (FL_CTRL | FL_META);
    const char *text = Fl_event_text();
    int len = Fl_event_length();
    int multiline = Fl_Input_input_type(self) == FL_MULTILINE_INPUT;
    int readonly = Fl_Input_readonly(self);

    switch (key) {
        case FL_Left:
            shift_position(self, ctrl ? Fl_Input_word_start(self, self->position_) : self->position_ - 1, shift);
            return 1;
        case FL_Right:
            shift_position(self, ctrl ? Fl_Input_word_end(self, self->position_) : self->position_ + 1, shift);
            return 1;
        case FL_Up:
            if (multiline) { move_up_down(self, -1, shift); return 1; }
            return 0;
        case FL_Down:
            if (multiline) { move_up_down(self, 1, shift); return 1; }
            return 0;
        case FL_Home:
            shift_position(self, ctrl ? 0 : Fl_Input_line_start(self, self->position_), shift);
            return 1;
        case FL_End:
            shift_position(self, ctrl ? self->size_ : Fl_Input_line_end(self, self->position_), shift);
            return 1;
        case FL_Page_Up:
            if (multiline) { move_up_down(self, -1, shift); return 1; }
            return 0;
        case FL_Page_Down:
            if (multiline) { move_up_down(self, 1, shift); return 1; }
            return 0;
        case FL_BackSpace:
            if (readonly) return 1;
            if (self->position_ != self->mark_) Fl_Input_replace(self, self->position_, self->mark_, NULL, 0);
            else if (self->position_ > 0) Fl_Input_replace(self, self->position_ - 1, self->position_, NULL, 0);
            return 1;
        case FL_Delete:
            if (readonly) return 1;
            if (self->position_ != self->mark_) Fl_Input_replace(self, self->position_, self->mark_, NULL, 0);
            else if (self->position_ < self->size_) Fl_Input_replace(self, self->position_, self->position_ + 1, NULL, 0);
            return 1;
        case FL_Enter:
        case FL_KP_Enter:
            if (multiline && !readonly) { Fl_Input_replace(self, self->position_, self->mark_, "\n", 1); return 1; }
            return 0;
        case FL_Tab:
            if (multiline && !self->tab_nav_ && !readonly) {
                Fl_Input_replace(self, self->position_, self->mark_, "\t", 1);
                return 1;
            }
            return 0;
        default:
            break;
    }

    if (ctrl) {
        switch (key) {
            case 'a': Fl_Input_set_position_mark(self, 0, self->size_); return 1;
            case 'c': Fl_Input_copy(self, 1); return 1;
            case 'x':
                if (!readonly) { Fl_Input_copy(self, 1); Fl_Input_replace(self, self->position_, self->mark_, NULL, 0); }
                return 1;
            case 'v':
                if (!readonly) Fl_paste(self_w, 1);
                return 1;
            default:
                return 0;
        }
    }

    if (!readonly && len > 0 && (unsigned char)text[0] >= ' ' && (unsigned char)text[0] != 127) {
        int i;
        if (Fl_Input_input_type(self) == FL_INT_INPUT || Fl_Input_input_type(self) == FL_FLOAT_INPUT) {
            for (i = 0; i < len; i++) if (!legal_numeric_char(self, text[i])) return 1;
        }
        Fl_Input_replace(self, self->position_, self->mark_, text, len);
        return 1;
    }
    return 0;
}

/* ---------------------------------------------------------------------
 * Event dispatch
 * ------------------------------------------------------------------ */

int Fl_Input_handle(Fl_Widget *self_w, int event) {
    Fl_Input *self = (Fl_Input *)self_w;
    /* Matches drawtext()'s box-inset text origin, so click/drag
     * coordinates line up with what's actually drawn. */
    int X = self_w->x + fl_box_dx(self_w->box);
    int Y = self_w->y + fl_box_dy(self_w->box);

    switch (event) {
        case FL_ENTER:
        case FL_MOVE:
        case FL_LEAVE:
            return 1;

        case FL_FOCUS:
        case FL_UNFOCUS:
            Fl_Widget_redraw(self_w);
            return 1;

        case FL_PUSH:
            handle_mouse(self, X, Y, Fl_event_state_of(FL_SHIFT));
            if (Fl_focus() != self_w) {
                Fl_set_focus(self_w);
                Fl_Widget_handle(self_w, FL_FOCUS);
            }
            return 1;

        case FL_DRAG:
            handle_mouse(self, X, Y, 1);
            return 1;

        case FL_RELEASE:
            Fl_Input_copy(self, 0);
            return 1;

        case FL_PASTE: {
            const char *t;
            int len;
            if (Fl_Input_readonly(self)) return 1;
            t = Fl_event_text();
            len = Fl_event_length();
            if (!t || !len) return 1;
            if (Fl_Input_input_type(self) != FL_MULTILINE_INPUT)
                while (len > 0 && isspace((unsigned char)t[len - 1])) len--;
            if (len <= 0) return 1;
            Fl_Input_replace(self, self->position_, self->mark_, t, len);
            return 1;
        }

        case FL_SHORTCUT:
            if (!(self->shortcut_ ? Fl_test_shortcut((Fl_Shortcut)self->shortcut_) : Fl_Widget_test_shortcut(self_w)))
                return 0;
            if (Fl_visible_focus()) {
                Fl_set_focus(self_w);
                Fl_Widget_handle(self_w, FL_FOCUS);
                return 1;
            }
            return 0;

        case FL_KEYDOWN:
            return handle_key(self);

        default:
            return 0;
    }
}
