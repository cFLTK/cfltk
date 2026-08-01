/*
 * cfltk - Fl_Browser_.c
 * See include/cfltk/Fl_Browser_.h for the class-conversion notes.
 * Translated from src/Fl_Browser_.cxx.
 */
#include <stdlib.h>
#include <string.h>

#include "cfltk/Fl_Browser_.h"
#include "cfltk/Fl.h"
#include "cfltk/fl_draw.h"

static inline Fl_Widget *sb_widget(Fl_Scrollbar *s) { return &s->slider.valuator.widget; }

/* -------------------------------------------------------------------
 * Item-ops dispatch (with defaults for the optional entries).
 * ---------------------------------------------------------------- */

static void *call_item_first(Fl_Browser_ *self) { return self->item_ops->item_first(self); }
static void *call_item_next(Fl_Browser_ *self, void *item) { return self->item_ops->item_next(self, item); }
static void *call_item_prev(Fl_Browser_ *self, void *item) { return self->item_ops->item_prev(self, item); }
static int call_item_height(Fl_Browser_ *self, void *item) { return self->item_ops->item_height(self, item); }
static int call_item_width(Fl_Browser_ *self, void *item) { return self->item_ops->item_width(self, item); }
static void call_item_draw(Fl_Browser_ *self, void *item, int X, int Y, int W, int H) {
    self->item_ops->item_draw(self, item, X, Y, W, H);
}
static void call_item_swap(Fl_Browser_ *self, void *a, void *b) {
    if (self->item_ops->item_swap) self->item_ops->item_swap(self, a, b);
}
static const char *call_item_text(Fl_Browser_ *self, void *item) {
    return self->item_ops->item_text ? self->item_ops->item_text(self, item) : NULL;
}
static void call_item_select(Fl_Browser_ *self, void *item, int val) {
    if (self->item_ops->item_select) self->item_ops->item_select(self, item, val);
}

int Fl_Browser__default_item_quick_height(Fl_Browser_ *self, void *item) { return call_item_height(self, item); }
static int call_item_quick_height(Fl_Browser_ *self, void *item) {
    return self->item_ops->item_quick_height ? self->item_ops->item_quick_height(self, item)
                                              : Fl_Browser__default_item_quick_height(self, item);
}

int Fl_Browser__default_item_selected(Fl_Browser_ *self, void *item) { return item == self->selection_ ? 1 : 0; }
static int call_item_selected(Fl_Browser_ *self, void *item) {
    return self->item_ops->item_selected ? self->item_ops->item_selected(self, item)
                                          : Fl_Browser__default_item_selected(self, item);
}

int Fl_Browser__default_full_width(Fl_Browser_ *self) { return self->max_width; }
static int call_full_width(Fl_Browser_ *self) {
    return self->item_ops->full_width ? self->item_ops->full_width(self) : Fl_Browser__default_full_width(self);
}

int Fl_Browser__default_full_height(Fl_Browser_ *self) {
    int t = 0;
    void *p;
    for (p = call_item_first(self); p; p = call_item_next(self, p)) t += call_item_quick_height(self, p);
    return t;
}
static int call_full_height(Fl_Browser_ *self) {
    return self->item_ops->full_height ? self->item_ops->full_height(self) : Fl_Browser__default_full_height(self);
}

int Fl_Browser__default_incr_height(Fl_Browser_ *self) { return call_item_quick_height(self, call_item_first(self)); }

/* -------------------------------------------------------------------
 * Construction / shared widget ops.
 * ---------------------------------------------------------------- */

static void scrollbar_callback(Fl_Widget *s, void *data) {
    Fl_Browser_ *self = (Fl_Browser_ *)Fl_Widget_parent(s);
    (void)data;
    Fl_Browser__set_position(self, (int)Fl_Valuator_value((Fl_Valuator *)s));
}

static void hscrollbar_callback(Fl_Widget *s, void *data) {
    Fl_Browser_ *self = (Fl_Browser_ *)Fl_Widget_parent(s);
    (void)data;
    Fl_Browser__set_hposition(self, (int)Fl_Valuator_value((Fl_Valuator *)s));
}

void Fl_Browser__init(Fl_Browser_ *self, const Fl_WidgetOps *widget_ops, const Fl_Browser_ItemOps *item_ops,
                       int x, int y, int w, int h, const char *label) {
    Fl_Group_init(&self->group, x, y, w, h, label);
    self->group.widget.ops = widget_ops;
    self->item_ops = item_ops;

    Fl_Scrollbar_init(&self->scrollbar, 0, 0, 0, 0, NULL);
    Fl_Scrollbar_init(&self->hscrollbar, 0, 0, 0, 0, NULL);

    self->group.widget.box = FL_NO_BOX;
    Fl_Widget_set_align(&self->group.widget, FL_ALIGN_BOTTOM);
    self->position_ = self->real_position_ = 0;
    self->hposition_ = self->real_hposition_ = 0;
    self->offset_ = 0;
    self->top_ = NULL;
    Fl_Widget_set_when(&self->group.widget, FL_WHEN_RELEASE_ALWAYS);
    self->selection_ = NULL;
    Fl_Widget_set_colors(&self->group.widget, FL_BACKGROUND2_COLOR, FL_SELECTION_COLOR);
    Fl_Widget_set_callback(sb_widget(&self->scrollbar), scrollbar_callback, NULL);
    Fl_Widget_set_callback(sb_widget(&self->hscrollbar), hscrollbar_callback, NULL);
    Fl_Widget_set_type(sb_widget(&self->hscrollbar), FL_HORIZONTAL);
    self->textfont_ = FL_HELVETICA;
    self->textsize_ = FL_NORMAL_SIZE;
    self->textcolor_ = FL_FOREGROUND_COLOR;
    self->has_scrollbar_ = FL_BROWSER_BOTH;
    self->max_width = 0;
    self->max_width_item = NULL;
    self->scrollbar_size_ = 0;
    self->redraw1 = self->redraw2 = NULL;
    Fl_Group_end(&self->group);
}

void Fl_Browser__destroy(Fl_Widget *self_w) {
    Fl_Browser_ *self = (Fl_Browser_ *)self_w;
    Fl_Scrollbar_destroy(sb_widget(&self->hscrollbar));
    Fl_Scrollbar_destroy(sb_widget(&self->scrollbar));
    Fl_Group_destroy(self_w);
}

Fl_Group *Fl_Browser__as_group(Fl_Widget *self) { return (Fl_Group *)self; }

/* -------------------------------------------------------------------
 * bbox / resize
 * ---------------------------------------------------------------- */

void Fl_Browser__bbox(const Fl_Browser_ *self, int *X, int *Y, int *W, int *H) {
    const Fl_Widget *w = &self->group.widget;
    int scrollsize = self->scrollbar_size_ ? self->scrollbar_size_ : Fl_scrollbar_size();
    uchar b = w->box ? w->box : FL_DOWN_BOX;

    *X = w->x + fl_box_dx(b);
    *Y = w->y + fl_box_dy(b);
    *W = w->w - fl_box_dw(b);
    *H = w->h - fl_box_dh(b);
    if (Fl_Widget_visible(sb_widget((Fl_Scrollbar *)&self->scrollbar))) {
        *W -= scrollsize;
        if (Fl_Widget_align(sb_widget((Fl_Scrollbar *)&self->scrollbar)) & FL_ALIGN_LEFT) *X += scrollsize;
    }
    if (*W < 0) *W = 0;
    if (Fl_Widget_visible(sb_widget((Fl_Scrollbar *)&self->hscrollbar))) {
        *H -= scrollsize;
        if (Fl_Widget_align(sb_widget((Fl_Scrollbar *)&self->scrollbar)) & FL_ALIGN_TOP) *Y += scrollsize;
    }
    if (*H < 0) *H = 0;
}

int Fl_Browser__leftedge(const Fl_Browser_ *self) {
    int X, Y, W, H;
    Fl_Browser__bbox(self, &X, &Y, &W, &H);
    return X;
}

void Fl_Browser__resize(Fl_Widget *self_w, int X, int Y, int W, int H) {
    Fl_Browser_ *self = (Fl_Browser_ *)self_w;
    int scrollsize = self->scrollbar_size_ ? self->scrollbar_size_ : Fl_scrollbar_size();
    Fl_Widget *vs = sb_widget(&self->scrollbar);
    Fl_Widget *hs = sb_widget(&self->hscrollbar);
    int bx, by, bw, bh;

    /* Skips Fl_Group_resize(): it would move the scrollbars uselessly
     * (they get repositioned below, and again on the next draw()). */
    Fl_Widget_default_resize(self_w, X, Y, W, H);
    Fl_Browser__bbox(self, &bx, &by, &bw, &bh);
    Fl_Widget_resize(vs, (Fl_Widget_align(vs) & FL_ALIGN_LEFT) ? bx - scrollsize : bx + bw, by, scrollsize, bh);
    Fl_Widget_resize(hs, bx, (Fl_Widget_align(vs) & FL_ALIGN_TOP) ? by - scrollsize : by + bh, bw, scrollsize);
    self->max_width = 0;
}

/* -------------------------------------------------------------------
 * Redraw bookkeeping
 * ---------------------------------------------------------------- */

void Fl_Browser__redraw_line(Fl_Browser_ *self, void *item) {
    Fl_Widget *w = &self->group.widget;
    if (!self->redraw1 || self->redraw1 == item) { self->redraw1 = item; Fl_Widget_set_damage(w, FL_DAMAGE_EXPOSE); }
    else if (!self->redraw2 || self->redraw2 == item) { self->redraw2 = item; Fl_Widget_set_damage(w, FL_DAMAGE_EXPOSE); }
    else Fl_Widget_set_damage(w, FL_DAMAGE_SCROLL);
}

/* -------------------------------------------------------------------
 * Scroll position
 * ---------------------------------------------------------------- */

static void update_top(Fl_Browser_ *self) {
    if (!self->top_) self->top_ = call_item_first(self);
    if (self->position_ != self->real_position_) {
        void *l;
        int ly;
        int yy = self->position_;
        if (!self->top_ || yy <= (self->real_position_ / 2)) {
            l = call_item_first(self);
            ly = 0;
        } else {
            l = self->top_;
            ly = self->real_position_ - self->offset_;
        }
        if (!l) {
            self->top_ = NULL;
            self->offset_ = 0;
            self->real_position_ = 0;
        } else {
            int hh = call_item_quick_height(self, l);
            while (ly > yy) {
                void *l1 = call_item_prev(self, l);
                if (!l1) { ly = 0; break; }
                l = l1;
                hh = call_item_quick_height(self, l);
                ly -= hh;
            }
            while ((ly + hh) <= yy) {
                void *l1 = call_item_next(self, l);
                if (!l1) { yy = ly + hh - 1; break; }
                l = l1;
                ly += hh;
                hh = call_item_quick_height(self, l);
            }
            for (;;) {
                hh = call_item_height(self, l);
                if ((ly + hh) > yy) break;
                {
                    void *l1 = call_item_prev(self, l);
                    if (!l1) { ly = yy = 0; break; }
                    l = l1;
                    yy = self->position_ = ly = ly - call_item_quick_height(self, l);
                }
            }
            self->top_ = l;
            self->offset_ = yy - ly;
            self->real_position_ = yy;
        }
        Fl_Widget_set_damage(&self->group.widget, FL_DAMAGE_SCROLL);
    }
}

void Fl_Browser__set_position(Fl_Browser_ *self, int pos) {
    if (pos < 0) pos = 0;
    if (pos == self->position_) return;
    self->position_ = pos;
    if (pos != self->real_position_) Fl_Browser__redraw_lines(self);
}

void Fl_Browser__set_hposition(Fl_Browser_ *self, int pos) {
    if (pos < 0) pos = 0;
    if (pos == self->hposition_) return;
    self->hposition_ = pos;
    if (pos != self->real_hposition_) Fl_Browser__redraw_lines(self);
}

int Fl_Browser__displayed(const Fl_Browser_ *self, void *item) {
    int X, Y, W, H;
    void *l;
    int yy;
    Fl_Browser__bbox(self, &X, &Y, &W, &H);
    yy = H + self->offset_;
    for (l = self->top_; l && yy > 0; l = call_item_next((Fl_Browser_ *)self, l)) {
        if (l == item) return 1;
        yy -= call_item_height((Fl_Browser_ *)self, l);
    }
    return 0;
}

void Fl_Browser__display(Fl_Browser_ *self, void *item) {
    int X, Y, W, H, Yp;
    void *l, *lp;
    int h1;

    update_top(self);
    if (item == call_item_first(self)) { Fl_Browser__set_position(self, 0); return; }

    Fl_Browser__bbox(self, &X, &Y, &W, &H);
    l = self->top_;
    Y = Yp = -self->offset_;

    if (l == item) { Fl_Browser__set_position(self, self->real_position_ + Y); return; }

    lp = call_item_prev(self, l);
    if (lp == item) {
        Fl_Browser__set_position(self, self->real_position_ + Y - call_item_quick_height(self, lp));
        return;
    }

    while (l || lp) {
        if (l) {
            h1 = call_item_quick_height(self, l);
            if (l == item) {
                if (Y <= H) {
                    Y = Y + h1 - H;
                    if (Y > 0) Fl_Browser__set_position(self, self->real_position_ + Y);
                } else {
                    Fl_Browser__set_position(self, self->real_position_ + Y - (H - h1) / 2);
                }
                return;
            }
            Y += h1;
            l = call_item_next(self, l);
        }
        if (lp) {
            h1 = call_item_quick_height(self, lp);
            Yp -= h1;
            if (lp == item) {
                if ((Yp + h1) >= 0) Fl_Browser__set_position(self, self->real_position_ + Yp);
                else Fl_Browser__set_position(self, self->real_position_ + Yp - (H - h1) / 2);
                return;
            }
            lp = call_item_prev(self, lp);
        }
    }
}

/* -------------------------------------------------------------------
 * Draw
 * ---------------------------------------------------------------- */

void Fl_Browser__draw(Fl_Widget *self_w) {
    Fl_Browser_ *self = (Fl_Browser_ *)self_w;
    Fl_Widget *vs = sb_widget(&self->scrollbar);
    Fl_Widget *hs = sb_widget(&self->hscrollbar);
    int drawsquare = 0;
    int full_width_, full_height_;
    int X, Y, W, H;
    int dont_repeat = 0;

    update_top(self);
    full_width_ = call_full_width(self);
    full_height_ = call_full_height(self);
    Fl_Browser__bbox(self, &X, &Y, &W, &H);

J1:
    if (self_w->damage & FL_DAMAGE_ALL) {
        uchar b = self_w->box ? self_w->box : FL_DOWN_BOX;
        fl_draw_box(b, self_w->x, self_w->y, self_w->w, self_w->h, self_w->color);
        drawsquare = 1;
    }

    if ((self->has_scrollbar_ & FL_BROWSER_VERTICAL) &&
        ((self->has_scrollbar_ & FL_BROWSER_ALWAYS_ON) || self->position_ || full_height_ > H)) {
        if (!Fl_Widget_visible(vs)) {
            Fl_Widget_set_visible(vs);
            drawsquare = 1;
            Fl_Browser__bbox(self, &X, &Y, &W, &H);
        }
    } else {
        self->top_ = call_item_first(self);
        self->real_position_ = self->offset_ = 0;
        if (Fl_Widget_visible(vs)) {
            Fl_Widget_clear_visible(vs);
            Fl_Widget_clear_damage(self_w, (uchar)(self_w->damage | FL_DAMAGE_SCROLL));
        }
    }

    if ((self->has_scrollbar_ & FL_BROWSER_HORIZONTAL) &&
        ((self->has_scrollbar_ & FL_BROWSER_ALWAYS_ON) || self->hposition_ || full_width_ > W)) {
        if (!Fl_Widget_visible(hs)) {
            Fl_Widget_set_visible(hs);
            drawsquare = 1;
            Fl_Browser__bbox(self, &X, &Y, &W, &H);
        }
    } else {
        self->real_hposition_ = 0;
        if (Fl_Widget_visible(hs)) {
            Fl_Widget_clear_visible(hs);
            Fl_Widget_clear_damage(self_w, (uchar)(self_w->damage | FL_DAMAGE_SCROLL));
        }
    }

    if ((self->has_scrollbar_ & FL_BROWSER_VERTICAL) &&
        ((self->has_scrollbar_ & FL_BROWSER_ALWAYS_ON) || self->position_ || full_height_ > H)) {
        if (!Fl_Widget_visible(vs)) {
            Fl_Widget_set_visible(vs);
            drawsquare = 1;
            Fl_Browser__bbox(self, &X, &Y, &W, &H);
        }
    } else {
        self->top_ = call_item_first(self);
        self->real_position_ = self->offset_ = 0;
        if (Fl_Widget_visible(vs)) {
            Fl_Widget_clear_visible(vs);
            Fl_Widget_clear_damage(self_w, (uchar)(self_w->damage | FL_DAMAGE_SCROLL));
        }
    }

    Fl_Browser__bbox(self, &X, &Y, &W, &H);

    fl_push_clip(X, Y, W, H);
    {
        void *l = Fl_Browser__top(self);
        int yy = -self->offset_;
        for (; l && yy < H; l = call_item_next(self, l)) {
            int hh = call_item_height(self, l);
            if (hh <= 0) continue;
            if ((self_w->damage & (FL_DAMAGE_SCROLL | FL_DAMAGE_ALL)) || l == self->redraw1 || l == self->redraw2) {
                if (call_item_selected(self, l)) {
                    fl_color(Fl_Widget_active_r(self_w) ? Fl_Widget_selection_color(self_w) : fl_inactive(Fl_Widget_selection_color(self_w)));
                    fl_rectf(X, yy + Y, W, hh);
                } else if (!(self_w->damage & FL_DAMAGE_ALL)) {
                    fl_push_clip(X, yy + Y, W, hh);
                    fl_draw_box(self_w->box ? self_w->box : FL_DOWN_BOX, self_w->x, self_w->y, self_w->w, self_w->h, self_w->color);
                    fl_pop_clip();
                }
                call_item_draw(self, l, X - self->hposition_, yy + Y, W + self->hposition_, hh);
                if (l == self->selection_ && Fl_focus() == self_w) {
                    fl_draw_box(FL_BORDER_FRAME, X, yy + Y, W, hh, self_w->color);
                    Fl_Widget_draw_focus(self_w, FL_NO_BOX, X, yy + Y, W + 1, hh + 1);
                }
                {
                    int ww = call_item_width(self, l);
                    if (ww > self->max_width) { self->max_width = ww; self->max_width_item = l; }
                }
            }
            yy += hh;
        }
        if (!(self_w->damage & FL_DAMAGE_ALL) && yy < H) {
            fl_push_clip(X, yy + Y, W, H - yy);
            fl_draw_box(self_w->box ? self_w->box : FL_DOWN_BOX, self_w->x, self_w->y, self_w->w, self_w->h, self_w->color);
            fl_pop_clip();
        }
    }
    fl_pop_clip();

    fl_push_clip(self_w->x, self_w->y, self_w->w, self_w->h);
    self->redraw1 = self->redraw2 = NULL;
    if (!dont_repeat) {
        dont_repeat = 1;
        full_height_ = call_full_height(self);
        full_width_ = call_full_width(self);
        if ((self->has_scrollbar_ & FL_BROWSER_VERTICAL) &&
            ((self->has_scrollbar_ & FL_BROWSER_ALWAYS_ON) || self->position_ || full_height_ > H)) {
            if (!Fl_Widget_visible(vs)) { Fl_Widget_set_damage(self_w, FL_DAMAGE_ALL); fl_pop_clip(); goto J1; }
        } else {
            if (Fl_Widget_visible(vs)) { Fl_Widget_set_damage(self_w, FL_DAMAGE_ALL); fl_pop_clip(); goto J1; }
        }
        if ((self->has_scrollbar_ & FL_BROWSER_HORIZONTAL) &&
            ((self->has_scrollbar_ & FL_BROWSER_ALWAYS_ON) || self->hposition_ || full_width_ > W)) {
            if (!Fl_Widget_visible(hs)) { Fl_Widget_set_damage(self_w, FL_DAMAGE_ALL); fl_pop_clip(); goto J1; }
        } else {
            if (Fl_Widget_visible(hs)) { Fl_Widget_set_damage(self_w, FL_DAMAGE_ALL); fl_pop_clip(); goto J1; }
        }
    }

    {
        int scrollsize = self->scrollbar_size_ ? self->scrollbar_size_ : Fl_scrollbar_size();
        int dy = self->top_ ? call_item_quick_height(self, self->top_) : 0;
        if (dy < 10) dy = 10;
        if (Fl_Widget_visible(vs)) {
            Fl_Widget_damage_resize(vs, (Fl_Widget_align(vs) & FL_ALIGN_LEFT) ? X - scrollsize : X + W, Y, scrollsize, H);
            Fl_Scrollbar_set_value_range((Fl_Scrollbar *)vs, self->position_, H, 0, full_height_);
            Fl_Scrollbar_set_linesize((Fl_Scrollbar *)vs, dy);
            if (drawsquare) Fl_Group_draw_child(&self->group, vs);
            else if (Fl_Widget_damage(vs)) { Fl_Widget_draw(vs); Fl_Widget_clear_damage(vs, 0); }
        }
        if (Fl_Widget_visible(hs)) {
            Fl_Widget_damage_resize(hs, X, (Fl_Widget_align(vs) & FL_ALIGN_TOP) ? Y - scrollsize : Y + H, W, scrollsize);
            Fl_Scrollbar_set_value_range((Fl_Scrollbar *)hs, self->hposition_, W, 0, full_width_);
            Fl_Scrollbar_set_linesize((Fl_Scrollbar *)hs, dy);
            if (drawsquare) Fl_Group_draw_child(&self->group, hs);
            else if (Fl_Widget_damage(hs)) { Fl_Widget_draw(hs); Fl_Widget_clear_damage(hs, 0); }
        }

        if (drawsquare && Fl_Widget_visible(vs) && Fl_Widget_visible(hs)) {
            fl_color(Fl_Widget_parent(self_w) ? Fl_Widget_color(&Fl_Widget_parent(self_w)->widget) : self_w->color);
            fl_rectf(vs->x, hs->y, scrollsize, scrollsize);
        }
    }

    self->real_hposition_ = self->hposition_;
    fl_pop_clip();
}

/* -------------------------------------------------------------------
 * List bookkeeping helpers subclasses call
 * ---------------------------------------------------------------- */

void Fl_Browser__new_list(Fl_Browser_ *self) {
    self->top_ = NULL;
    self->position_ = self->real_position_ = 0;
    self->hposition_ = self->real_hposition_ = 0;
    self->selection_ = NULL;
    self->offset_ = 0;
    self->max_width = 0;
    self->max_width_item = NULL;
    Fl_Browser__redraw_lines(self);
}

void Fl_Browser__deleting(Fl_Browser_ *self, void *item) {
    if (Fl_Browser__displayed(self, item)) {
        Fl_Browser__redraw_lines(self);
        if (item == self->top_) {
            self->real_position_ -= self->offset_;
            self->offset_ = 0;
            self->top_ = call_item_next(self, item);
            if (!self->top_) self->top_ = call_item_prev(self, item);
        }
    } else {
        self->real_position_ = 0;
        self->offset_ = 0;
        self->top_ = NULL;
    }
    if (item == self->selection_) self->selection_ = NULL;
    if (item == self->max_width_item) { self->max_width_item = NULL; self->max_width = 0; }
}

void Fl_Browser__replacing(Fl_Browser_ *self, void *a, void *b) {
    Fl_Browser__redraw_line(self, a);
    if (a == self->selection_) self->selection_ = b;
    if (a == self->top_) self->top_ = b;
    if (a == self->max_width_item) { self->max_width_item = NULL; self->max_width = 0; }
}

void Fl_Browser__swapping(Fl_Browser_ *self, void *a, void *b) {
    Fl_Browser__redraw_line(self, a);
    Fl_Browser__redraw_line(self, b);
    if (a == self->selection_) self->selection_ = b;
    else if (b == self->selection_) self->selection_ = a;
    if (a == self->top_) self->top_ = b;
    else if (b == self->top_) self->top_ = a;
}

void Fl_Browser__inserting(Fl_Browser_ *self, void *a, void *b) {
    if (Fl_Browser__displayed(self, a)) Fl_Browser__redraw_lines(self);
    if (a == self->top_) self->top_ = b;
}

void *Fl_Browser__find_item(Fl_Browser_ *self, int ypos) {
    int X, Y, W, H;
    void *l;
    int yy;
    update_top(self);
    Fl_Browser__bbox(self, &X, &Y, &W, &H);
    yy = Y - self->offset_;
    for (l = self->top_; l; l = call_item_next(self, l)) {
        int hh = call_item_height(self, l);
        if (hh <= 0) continue;
        yy += hh;
        if (ypos <= yy || yy >= (Y + H)) return l;
    }
    return NULL;
}

/* -------------------------------------------------------------------
 * Selection
 * ---------------------------------------------------------------- */

int Fl_Browser__select(Fl_Browser_ *self, void *item, int val, int docallbacks) {
    Fl_Widget *self_w = &self->group.widget;
    if (self_w->type == FL_MULTI_BROWSER) {
        if (self->selection_ != item) {
            if (self->selection_) Fl_Browser__redraw_line(self, self->selection_);
            self->selection_ = item;
            Fl_Browser__redraw_line(self, item);
        }
        if ((!val) == (!call_item_selected(self, item))) return 0;
        call_item_select(self, item, val);
        Fl_Browser__redraw_line(self, item);
    } else {
        if (val && self->selection_ == item) return 0;
        if (!val && self->selection_ != item) return 0;
        if (self->selection_) {
            call_item_select(self, self->selection_, 0);
            Fl_Browser__redraw_line(self, self->selection_);
            self->selection_ = NULL;
        }
        if (val) {
            call_item_select(self, item, 1);
            self->selection_ = item;
            Fl_Browser__redraw_line(self, item);
            Fl_Browser__display(self, item);
        }
    }
    if (docallbacks) {
        Fl_Widget_set_changed(self_w);
        Fl_Widget_do_callback(self_w);
    }
    return 1;
}

int Fl_Browser__deselect(Fl_Browser_ *self, int docallbacks) {
    Fl_Widget *self_w = &self->group.widget;
    if (self_w->type == FL_MULTI_BROWSER) {
        int change = 0;
        void *p;
        for (p = call_item_first(self); p; p = call_item_next(self, p)) change |= Fl_Browser__select(self, p, 0, docallbacks);
        return change;
    }
    if (!self->selection_) return 0;
    call_item_select(self, self->selection_, 0);
    Fl_Browser__redraw_line(self, self->selection_);
    self->selection_ = NULL;
    return 1;
}

int Fl_Browser__select_only(Fl_Browser_ *self, void *item, int docallbacks) {
    Fl_Widget *self_w = &self->group.widget;
    int change = 0;
    Fl_Widget_Tracker wp;

    if (!item) return Fl_Browser__deselect(self, docallbacks);

    Fl_Widget_Tracker_watch(&wp, self_w);
    if (self_w->type == FL_MULTI_BROWSER) {
        void *p;
        for (p = call_item_first(self); p; p = call_item_next(self, p)) {
            if (p != item) change |= Fl_Browser__select(self, p, 0, docallbacks);
            if (!Fl_Widget_Tracker_exists(&wp)) { Fl_Widget_Tracker_release(&wp); return change; }
        }
    }
    change |= Fl_Browser__select(self, item, 1, docallbacks);
    if (!Fl_Widget_Tracker_exists(&wp)) { Fl_Widget_Tracker_release(&wp); return change; }
    Fl_Browser__display(self, item);
    Fl_Widget_Tracker_release(&wp);
    return change;
}

/* -------------------------------------------------------------------
 * Event handling
 * ---------------------------------------------------------------- */

int Fl_Browser__handle(Fl_Widget *self_w, int event) {
    Fl_Browser_ *self = (Fl_Browser_ *)self_w;
    Fl_Widget_Tracker wp;
    int X, Y, W, H, my;
    static char change;
    static char whichway;
    static int py;

    Fl_Widget_Tracker_watch(&wp, self_w);

    if (event == FL_ENTER || event == FL_LEAVE) { Fl_Widget_Tracker_release(&wp); return 1; }

    if (event == FL_KEYBOARD && self_w->type >= FL_HOLD_BROWSER) {
        void *l1 = self->selection_;
        void *l = l1;
        if (!l) l = self->top_;
        if (!l) l = call_item_first(self);
        if (l) {
            if (self_w->type == FL_HOLD_BROWSER) {
                switch (Fl_event_key()) {
                    case FL_Down:
                        while ((l = call_item_next(self, l))) {
                            if (call_item_height(self, l) > 0) { Fl_Browser__select_only(self, l, Fl_Widget_when(self_w)); break; }
                        }
                        Fl_Widget_Tracker_release(&wp);
                        return 1;
                    case FL_Up:
                        while ((l = call_item_prev(self, l))) {
                            if (call_item_height(self, l) > 0) { Fl_Browser__select_only(self, l, Fl_Widget_when(self_w)); break; }
                        }
                        Fl_Widget_Tracker_release(&wp);
                        return 1;
                    default:
                        break;
                }
            } else {
                switch (Fl_event_key()) {
                    case FL_Enter:
                    case FL_KP_Enter:
                        Fl_Browser__select_only(self, l, Fl_Widget_when(self_w) & ~(uchar)FL_WHEN_ENTER_KEY);
                        if (!Fl_Widget_Tracker_exists(&wp)) { Fl_Widget_Tracker_release(&wp); return 1; }
                        if (Fl_Widget_when(self_w) & FL_WHEN_ENTER_KEY) {
                            Fl_Widget_set_changed(self_w);
                            Fl_Widget_do_callback(self_w);
                        }
                        Fl_Widget_Tracker_release(&wp);
                        return 1;
                    case ' ':
                        self->selection_ = l;
                        Fl_Browser__select(self, l, !call_item_selected(self, l), Fl_Widget_when(self_w) & ~(uchar)FL_WHEN_ENTER_KEY);
                        Fl_Widget_Tracker_release(&wp);
                        return 1;
                    case FL_Down:
                        while ((l = call_item_next(self, l))) {
                            if (Fl_event_state() & (FL_SHIFT | FL_CTRL))
                                Fl_Browser__select(self, l, l1 ? call_item_selected(self, l1) : 1, Fl_Widget_when(self_w));
                            if (!Fl_Widget_Tracker_exists(&wp)) { Fl_Widget_Tracker_release(&wp); return 1; }
                            if (call_item_height(self, l) > 0) goto J1;
                        }
                        Fl_Widget_Tracker_release(&wp);
                        return 1;
                    case FL_Up:
                        while ((l = call_item_prev(self, l))) {
                            if (Fl_event_state() & (FL_SHIFT | FL_CTRL))
                                Fl_Browser__select(self, l, l1 ? call_item_selected(self, l1) : 1, Fl_Widget_when(self_w));
                            if (!Fl_Widget_Tracker_exists(&wp)) { Fl_Widget_Tracker_release(&wp); return 1; }
                            if (call_item_height(self, l) > 0) goto J1;
                        }
                        Fl_Widget_Tracker_release(&wp);
                        return 1;
                    J1:
                        if (self->selection_) Fl_Browser__redraw_line(self, self->selection_);
                        self->selection_ = l;
                        Fl_Browser__redraw_line(self, l);
                        Fl_Browser__display(self, l);
                        Fl_Widget_Tracker_release(&wp);
                        return 1;
                    default:
                        break;
                }
            }
        }
    }

    if (Fl_Group_handle(self_w, event)) { Fl_Widget_Tracker_release(&wp); return 1; }
    if (!Fl_Widget_Tracker_exists(&wp)) { Fl_Widget_Tracker_release(&wp); return 1; }

    Fl_Browser__bbox(self, &X, &Y, &W, &H);

    switch (event) {
        case FL_PUSH:
            if (!Fl_event_inside_rect(X, Y, W, H)) { Fl_Widget_Tracker_release(&wp); return 0; }
            if (Fl_visible_focus()) { Fl_set_focus(self_w); Fl_Widget_redraw(self_w); }
            my = py = Fl_event_y();
            change = 0;
            if (self_w->type == FL_NORMAL_BROWSER || !self->top_) {
                /* nothing */
            } else if (self_w->type != FL_MULTI_BROWSER) {
                change = (char)Fl_Browser__select_only(self, Fl_Browser__find_item(self, my), 0);
                if (!Fl_Widget_Tracker_exists(&wp)) { Fl_Widget_Tracker_release(&wp); return 1; }
                if (change && (Fl_Widget_when(self_w) & FL_WHEN_CHANGED)) {
                    Fl_Widget_set_changed(self_w);
                    Fl_Widget_do_callback(self_w);
                    if (!Fl_Widget_Tracker_exists(&wp)) { Fl_Widget_Tracker_release(&wp); return 1; }
                }
            } else {
                void *l = Fl_Browser__find_item(self, my);
                whichway = 1;
                if (Fl_event_state() & FL_COMMAND) {
                TOGGLE:
                    if (l) {
                        whichway = (char)!call_item_selected(self, l);
                        change = (char)Fl_Browser__select(self, l, whichway, 0);
                        if (!Fl_Widget_Tracker_exists(&wp)) { Fl_Widget_Tracker_release(&wp); return 1; }
                        if (change && (Fl_Widget_when(self_w) & FL_WHEN_CHANGED)) {
                            Fl_Widget_set_changed(self_w);
                            Fl_Widget_do_callback(self_w);
                            if (!Fl_Widget_Tracker_exists(&wp)) { Fl_Widget_Tracker_release(&wp); return 1; }
                        }
                    }
                } else if (Fl_event_state() & FL_SHIFT) {
                    int down;
                    void *m;
                    if (l == self->selection_) goto TOGGLE;
                    whichway = l ? (char)!call_item_selected(self, l) : 1;
                    if (!l) down = 1;
                    else {
                        for (m = self->selection_;; m = call_item_next(self, m)) {
                            if (m == l) { down = 1; break; }
                            if (!m) { down = 0; break; }
                        }
                    }
                    if (down) {
                        for (m = self->selection_; m != l; m = call_item_next(self, m)) {
                            Fl_Browser__select(self, m, whichway, Fl_Widget_when(self_w) & FL_WHEN_CHANGED);
                            if (!Fl_Widget_Tracker_exists(&wp)) { Fl_Widget_Tracker_release(&wp); return 1; }
                        }
                    } else {
                        void *e = self->selection_;
                        for (m = call_item_next(self, l); m; m = call_item_next(self, m)) {
                            Fl_Browser__select(self, m, whichway, Fl_Widget_when(self_w) & FL_WHEN_CHANGED);
                            if (!Fl_Widget_Tracker_exists(&wp)) { Fl_Widget_Tracker_release(&wp); return 1; }
                            if (m == e) break;
                        }
                    }
                    change = 1;
                    if (l) Fl_Browser__select(self, l, whichway, Fl_Widget_when(self_w) & FL_WHEN_CHANGED);
                    if (!Fl_Widget_Tracker_exists(&wp)) { Fl_Widget_Tracker_release(&wp); return 1; }
                } else {
                    change = (char)Fl_Browser__select_only(self, l, 0);
                    if (!Fl_Widget_Tracker_exists(&wp)) { Fl_Widget_Tracker_release(&wp); return 1; }
                    if (change && (Fl_Widget_when(self_w) & FL_WHEN_CHANGED)) {
                        Fl_Widget_set_changed(self_w);
                        Fl_Widget_do_callback(self_w);
                        if (!Fl_Widget_Tracker_exists(&wp)) { Fl_Widget_Tracker_release(&wp); return 1; }
                    }
                }
            }
            Fl_Widget_Tracker_release(&wp);
            return 1;
        case FL_DRAG:
            my = Fl_event_y();
            if (my < Y && my < py) {
                int p = self->real_position_ + my - Y;
                if (p < 0) p = 0;
                Fl_Browser__set_position(self, p);
            } else if (my > (Y + H) && my > py) {
                int p = self->real_position_ + my - (Y + H);
                int hh = call_full_height(self) - H;
                if (p > hh) p = hh;
                if (p < 0) p = 0;
                Fl_Browser__set_position(self, p);
            }
            if (self_w->type == FL_NORMAL_BROWSER || !self->top_) {
                /* nothing */
            } else if (self_w->type == FL_MULTI_BROWSER) {
                void *l = Fl_Browser__find_item(self, my);
                void *t, *b;
                if (my > py) {
                    t = self->selection_ ? call_item_next(self, self->selection_) : NULL;
                    b = l ? call_item_next(self, l) : NULL;
                } else {
                    t = l;
                    b = self->selection_;
                }
                for (; t && t != b; t = call_item_next(self, t)) {
                    char change_t = (char)Fl_Browser__select(self, t, whichway, 0);
                    if (!Fl_Widget_Tracker_exists(&wp)) { Fl_Widget_Tracker_release(&wp); return 1; }
                    change = (char)(change | change_t);
                    if (change_t && (Fl_Widget_when(self_w) & FL_WHEN_CHANGED)) {
                        Fl_Widget_set_changed(self_w);
                        Fl_Widget_do_callback(self_w);
                        if (!Fl_Widget_Tracker_exists(&wp)) { Fl_Widget_Tracker_release(&wp); return 1; }
                    }
                }
                if (l) self->selection_ = l;
            } else {
                void *l1 = self->selection_;
                void *l = (Fl_event_x() < self_w->x || Fl_event_x() > self_w->x + self_w->w) ? self->selection_ : Fl_Browser__find_item(self, my);
                change = (char)(l != l1);
                Fl_Browser__select_only(self, l, Fl_Widget_when(self_w) & FL_WHEN_CHANGED);
                if (!Fl_Widget_Tracker_exists(&wp)) { Fl_Widget_Tracker_release(&wp); return 1; }
            }
            py = my;
            Fl_Widget_Tracker_release(&wp);
            return 1;
        case FL_RELEASE:
            if (self_w->type == FL_SELECT_BROWSER) {
                void *t = self->selection_;
                Fl_Browser__deselect(self, 0);
                if (!Fl_Widget_Tracker_exists(&wp)) { Fl_Widget_Tracker_release(&wp); return 1; }
                self->selection_ = t;
            }
            if (change) {
                Fl_Widget_set_changed(self_w);
                if (Fl_Widget_when(self_w) & FL_WHEN_RELEASE) Fl_Widget_do_callback(self_w);
            } else {
                if (Fl_Widget_when(self_w) & FL_WHEN_NOT_CHANGED) Fl_Widget_do_callback(self_w);
            }
            if (!Fl_Widget_Tracker_exists(&wp)) { Fl_Widget_Tracker_release(&wp); return 1; }
            if (Fl_event_clicks() && (Fl_Widget_when(self_w) & FL_WHEN_ENTER_KEY)) {
                Fl_Widget_set_changed(self_w);
                Fl_Widget_do_callback(self_w);
            }
            Fl_Widget_Tracker_release(&wp);
            return 1;
        case FL_FOCUS:
        case FL_UNFOCUS:
            if (self_w->type >= FL_HOLD_BROWSER && Fl_visible_focus()) { Fl_Widget_redraw(self_w); Fl_Widget_Tracker_release(&wp); return 1; }
            Fl_Widget_Tracker_release(&wp);
            return 0;
        default:
            break;
    }

    Fl_Widget_Tracker_release(&wp);
    return 0;
}

/* -------------------------------------------------------------------
 * Sort
 * ---------------------------------------------------------------- */

void Fl_Browser__sort(Fl_Browser_ *self, int flags) {
    int i, j, n = -1;
    int desc = (flags & FL_SORT_DESCENDING) == FL_SORT_DESCENDING;
    void *a = call_item_first(self);
    void *b, *c;

    if (!a) return;
    while (a) { a = call_item_next(self, a); n++; }

    for (i = n; i > 0; i--) {
        char swapped = 0;
        a = call_item_first(self);
        b = call_item_next(self, a);
        for (j = 0; j < i; j++) {
            const char *ta = call_item_text(self, a);
            const char *tb = call_item_text(self, b);
            c = call_item_next(self, b);
            if (desc) {
                if (strcmp(ta, tb) < 0) { call_item_swap(self, a, b); swapped = 1; }
            } else {
                if (strcmp(ta, tb) > 0) { call_item_swap(self, a, b); swapped = 1; }
            }
            if (!c) break;
            b = c;
            a = call_item_prev(self, b);
        }
        if (!swapped) break;
    }
}
