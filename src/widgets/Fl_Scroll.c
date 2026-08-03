/*
 * cfltk - Fl_Scroll.c
 * See include/cfltk/Fl_Scroll.h for the class-conversion notes.
 * Translated from src/Fl_Scroll.cxx.
 */
#include <stdlib.h>

#include "cfltk/Fl_Scroll.h"
#include "cfltk/Fl.h"
#include "cfltk/fl_draw.h"

static Fl_Group *Fl_Scroll_as_group(Fl_Widget *self) { return (Fl_Group *)self; }

const Fl_WidgetOps fl_scroll_ops = {
    Fl_Scroll_draw,
    Fl_Scroll_handle,
    Fl_Scroll_resize,
    Fl_Widget_default_show,
    Fl_Widget_default_hide,
    Fl_Scroll_destroy,
    Fl_Scroll_as_group,
    NULL
};

static inline Fl_Widget *sb_widget(Fl_Scrollbar *s) { return &s->slider.valuator.widget; }

/* Ensures the two scrollbars are always the last two children (hscrollbar
 * then scrollbar), so every other loop can safely iterate
 * `children()-2` to visit only the real content children. */
static void fix_scrollbar_order(Fl_Scroll *self) {
    Fl_Group *g = &self->group;
    Fl_Widget **a = g->children_array;
    int nc = g->children_count;
    Fl_Widget *hs = sb_widget(&self->hscrollbar);
    Fl_Widget *vs = sb_widget(&self->scrollbar);
    int i, j;

    if (nc == 0 || a[nc - 1] == vs) return;
    for (i = 0, j = 0; j < nc; j++)
        if (a[j] != hs && a[j] != vs) a[i++] = a[j];
    a[i++] = hs;
    a[i++] = vs;
}

static void hscrollbar_cb(Fl_Widget *o, void *data) {
    Fl_Scroll *self = (Fl_Scroll *)Fl_Widget_parent(o);
    (void)data;
    Fl_Scroll_scroll_to(self, (int)Fl_Valuator_value((Fl_Valuator *)o), self->yposition_);
}

static void scrollbar_cb(Fl_Widget *o, void *data) {
    Fl_Scroll *self = (Fl_Scroll *)Fl_Widget_parent(o);
    (void)data;
    Fl_Scroll_scroll_to(self, self->xposition_, (int)Fl_Valuator_value((Fl_Valuator *)o));
}

void Fl_Scroll_init(Fl_Scroll *self, int x, int y, int w, int h, const char *label) {
    int sbs = Fl_scrollbar_size();

    Fl_Group_init(&self->group, x, y, w, h, label);
    self->group.widget.ops = &fl_scroll_ops;
    self->group.widget.box = FL_NO_BOX;

    /* Declaration order matches upstream's member order (scrollbar then
     * hscrollbar), which is also construction order there -- both auto-
     * add themselves as children of `self` while it's still the current
     * group (Fl_Group_init() called Fl_Group_begin() and never ends it,
     * per the standard cfltk widget-tree-building convention). */
    Fl_Scrollbar_init(&self->scrollbar, x + w - sbs, y, sbs, h - sbs, NULL);
    Fl_Scrollbar_init(&self->hscrollbar, x, y + h - sbs, w - sbs, sbs, NULL);

    Fl_Widget_set_type(&self->group.widget, FL_SCROLL_BOTH);
    self->xposition_ = self->oldx = 0;
    self->yposition_ = self->oldy = 0;
    self->scrollbar_size_ = 0;

    Fl_Widget_set_type(sb_widget(&self->hscrollbar), FL_HORIZONTAL);
    Fl_Widget_set_callback(sb_widget(&self->hscrollbar), hscrollbar_cb, NULL);
    Fl_Widget_set_callback(sb_widget(&self->scrollbar), scrollbar_cb, NULL);
}

Fl_Scroll *Fl_Scroll_new(int x, int y, int w, int h, const char *label) {
    Fl_Scroll *self = (Fl_Scroll *)malloc(sizeof(Fl_Scroll));
    Fl_Scroll_init(self, x, y, w, h, label);
    return self;
}

void Fl_Scroll_destroy(Fl_Widget *self_w) {
    Fl_Scroll *self = (Fl_Scroll *)self_w;
    Fl_Scrollbar_destroy(sb_widget(&self->hscrollbar));
    Fl_Scrollbar_destroy(sb_widget(&self->scrollbar));
    Fl_Group_destroy(self_w);
}

void Fl_Scroll_clear(Fl_Scroll *self) {
    Fl_Group *g = &self->group;
    Fl_Widget *vs = sb_widget(&self->scrollbar);
    Fl_Widget *hs = sb_widget(&self->hscrollbar);
    Fl_Group_remove(g, vs);
    Fl_Group_remove(g, hs);
    Fl_Group_clear(g);
    Fl_Group_add(g, hs);
    Fl_Group_add(g, vs);
}

static void bbox(Fl_Scroll *self, int *X, int *Y, int *W, int *H) {
    Fl_Widget *w = &self->group.widget;
    Fl_Widget *vs = sb_widget(&self->scrollbar);
    Fl_Widget *hs = sb_widget(&self->hscrollbar);

    *X = w->x + fl_box_dx(w->box);
    *Y = w->y + fl_box_dy(w->box);
    *W = w->w - fl_box_dw(w->box);
    *H = w->h - fl_box_dh(w->box);
    if (Fl_Widget_visible(vs)) {
        *W -= vs->w;
        if (Fl_Widget_align(vs) & FL_ALIGN_LEFT) *X += vs->w;
    }
    if (Fl_Widget_visible(hs)) {
        *H -= hs->h;
        if (Fl_Widget_align(vs) & FL_ALIGN_TOP) *Y += hs->h;
    }
}

typedef struct { int x, y, w, h; } RegionXYWH;
typedef struct { int l, r, t, b; } RegionLRTB;
typedef struct { int x, y, w, h, pos, size, first, total; } ScrollbarGeom;

typedef struct {
    int scrollsize;
    RegionXYWH innerbox;
    RegionXYWH innerchild;
    RegionLRTB child;
    int hneeded, vneeded;
    ScrollbarGeom hscroll, vscroll;
} ScrollInfo;

static void recalc_scrollbars(Fl_Scroll *self, ScrollInfo *si) {
    Fl_Widget *w = &self->group.widget;
    Fl_Group *g = &self->group;
    Fl_Widget *vs = sb_widget(&self->scrollbar);
    Fl_Widget *hs = sb_widget(&self->hscrollbar);
    int nc = Fl_Group_children(g);
    int first, i;
    int X, Y, W, H;

    si->innerbox.x = w->x + fl_box_dx(w->box);
    si->innerbox.y = w->y + fl_box_dy(w->box);
    si->innerbox.w = w->w - fl_box_dw(w->box);
    si->innerbox.h = w->h - fl_box_dh(w->box);

    si->child.l = si->innerbox.x;
    si->child.r = si->innerbox.x;
    si->child.b = si->innerbox.y;
    si->child.t = si->innerbox.y;
    first = 1;
    for (i = 0; i < nc; i++) {
        Fl_Widget *o = Fl_Group_child(g, i);
        if (o == vs || o == hs) continue;
        if (first) {
            first = 0;
            si->child.l = o->x;
            si->child.r = o->x + o->w;
            si->child.b = o->y + o->h;
            si->child.t = o->y;
        } else {
            if (o->x < si->child.l) si->child.l = o->x;
            if (o->y < si->child.t) si->child.t = o->y;
            if (o->x + o->w > si->child.r) si->child.r = o->x + o->w;
            if (o->y + o->h > si->child.b) si->child.b = o->y + o->h;
        }
    }

    X = si->innerbox.x; Y = si->innerbox.y; W = si->innerbox.w; H = si->innerbox.h;
    si->scrollsize = self->scrollbar_size_ ? self->scrollbar_size_ : Fl_scrollbar_size();
    si->vneeded = 0;
    si->hneeded = 0;
    if (w->type & FL_SCROLL_VERTICAL) {
        if ((w->type & FL_SCROLL_ALWAYS_ON) || si->child.t < Y || si->child.b > Y + H) {
            si->vneeded = 1;
            W -= si->scrollsize;
            if (Fl_Widget_align(vs) & FL_ALIGN_LEFT) X += si->scrollsize;
        }
    }
    if (w->type & FL_SCROLL_HORIZONTAL) {
        if ((w->type & FL_SCROLL_ALWAYS_ON) || si->child.l < X || si->child.r > X + W) {
            si->hneeded = 1;
            H -= si->scrollsize;
            if (Fl_Widget_align(vs) & FL_ALIGN_TOP) Y += si->scrollsize;
            if (!si->vneeded && (w->type & FL_SCROLL_VERTICAL)) {
                if ((w->type & FL_SCROLL_ALWAYS_ON) || si->child.t < Y || si->child.b > Y + H) {
                    si->vneeded = 1;
                    W -= si->scrollsize;
                    if (Fl_Widget_align(vs) & FL_ALIGN_LEFT) X += si->scrollsize;
                }
            }
        }
    }
    si->innerchild.x = X; si->innerchild.y = Y; si->innerchild.w = W; si->innerchild.h = H;

    si->hscroll.x = si->innerchild.x;
    si->hscroll.y = (Fl_Widget_align(vs) & FL_ALIGN_TOP) ? si->innerbox.y : si->innerbox.y + si->innerbox.h - si->scrollsize;
    si->hscroll.w = si->innerchild.w;
    si->hscroll.h = si->scrollsize;

    si->vscroll.x = (Fl_Widget_align(vs) & FL_ALIGN_LEFT) ? si->innerbox.x : si->innerbox.x + si->innerbox.w - si->scrollsize;
    si->vscroll.y = si->innerchild.y;
    si->vscroll.w = si->scrollsize;
    si->vscroll.h = si->innerchild.h;

    si->hscroll.pos = si->innerchild.x - si->child.l;
    si->hscroll.size = si->innerchild.w;
    si->hscroll.first = 0;
    si->hscroll.total = si->child.r - si->child.l;
    if (si->hscroll.pos < 0) { si->hscroll.total += (-si->hscroll.pos); si->hscroll.first = si->hscroll.pos; }

    si->vscroll.pos = si->innerchild.y - si->child.t;
    si->vscroll.size = si->innerchild.h;
    si->vscroll.first = 0;
    si->vscroll.total = si->child.b - si->child.t;
    if (si->vscroll.pos < 0) { si->vscroll.total += (-si->vscroll.pos); si->vscroll.first = si->vscroll.pos; }
}

/* Fills [X,Y,W,H] with color() and draws every real content child (and
 * its outside label) inside it. Upstream also tiles a color-scheme
 * background for frame/no-box types; cfltk has no color schemes (see
 * header), so this always takes upstream's own "no scheme" fallback. */
static void draw_clip(Fl_Scroll *self, int X, int Y, int W, int H) {
    Fl_Group *g = &self->group;
    int nc = Fl_Group_children(g);
    int i;

    fl_push_clip(X, Y, W, H);
    fl_color(self->group.widget.color);
    fl_rectf(X, Y, W, H);

    for (i = 0; i < nc - 2; i++) {
        Fl_Widget *o = Fl_Group_child(g, i);
        Fl_Group_draw_child(g, o);
        Fl_Group_draw_outside_label(o);
    }
    fl_pop_clip();
}

static void scroll_draw_area_cb(void *data, int x, int y, int w, int h) {
    draw_clip((Fl_Scroll *)data, x, y, w, h);
}

void Fl_Scroll_draw(Fl_Widget *self_w) {
    Fl_Scroll *self = (Fl_Scroll *)self_w;
    Fl_Group *g = &self->group;
    Fl_Widget *vs = sb_widget(&self->scrollbar);
    Fl_Widget *hs = sb_widget(&self->hscrollbar);
    int X, Y, W, H;
    uchar d;
    ScrollInfo si;

    fix_scrollbar_order(self);
    bbox(self, &X, &Y, &W, &H);

    d = self_w->damage;

    if (d & FL_DAMAGE_ALL) {
        fl_draw_box(self_w->box, self_w->x, self_w->y, self_w->w, self_w->h, self_w->color);
        draw_clip(self, X, Y, W, H);
    } else {
        if (d & FL_DAMAGE_SCROLL) {
            /* [FIXED - see header] Blits the already-correct pixels by
             * the amount scrolled and redraws only the newly-exposed
             * strip, via cfltk's new fl_scroll() (commit 166e90e).
             * self->oldx/oldy hold the position last drawn (frozen
             * until updated below); self->xposition_/yposition_ hold
             * the current target (children were already repositioned
             * by scroll_to()/Fl_Scroll_resize() before this draw). */
            fl_scroll(X, Y, W, H, self->oldx - self->xposition_,
                      self->oldy - self->yposition_, scroll_draw_area_cb, self);
        }
        if (d & FL_DAMAGE_CHILD) {
            int nc = Fl_Group_children(g);
            int i;
            fl_push_clip(X, Y, W, H);
            for (i = 0; i < nc - 2; i++) {
                Fl_Widget *o = Fl_Group_child(g, i);
                if (Fl_Widget_visible(o) && Fl_Widget_damage(o)) {
                    Fl_Widget_draw(o);
                    Fl_Widget_clear_damage(o, 0);
                }
            }
            fl_pop_clip();
        }
    }

    recalc_scrollbars(self, &si);

    if (si.vneeded && !Fl_Widget_visible(vs)) {
        Fl_Widget_set_visible(vs);
        d = FL_DAMAGE_ALL;
    } else if (!si.vneeded && Fl_Widget_visible(vs)) {
        Fl_Widget_clear_visible(vs);
        draw_clip(self, si.vscroll.x, si.vscroll.y, si.vscroll.w, si.vscroll.h);
        d = FL_DAMAGE_ALL;
    }
    if (si.hneeded && !Fl_Widget_visible(hs)) {
        Fl_Widget_set_visible(hs);
        d = FL_DAMAGE_ALL;
    } else if (!si.hneeded && Fl_Widget_visible(hs)) {
        Fl_Widget_clear_visible(hs);
        draw_clip(self, si.hscroll.x, si.hscroll.y, si.hscroll.w, si.hscroll.h);
        d = FL_DAMAGE_ALL;
    } else if (hs->h != si.scrollsize || vs->w != si.scrollsize) {
        d = FL_DAMAGE_ALL;
    }

    Fl_Widget_resize(vs, si.vscroll.x, si.vscroll.y, si.vscroll.w, si.vscroll.h);
    self->oldy = self->yposition_ = si.vscroll.pos;
    Fl_Scrollbar_set_value_range((Fl_Scrollbar *)vs, si.vscroll.pos, si.vscroll.size, si.vscroll.first, si.vscroll.total);

    Fl_Widget_resize(hs, si.hscroll.x, si.hscroll.y, si.hscroll.w, si.hscroll.h);
    self->oldx = self->xposition_ = si.hscroll.pos;
    Fl_Scrollbar_set_value_range((Fl_Scrollbar *)hs, si.hscroll.pos, si.hscroll.size, si.hscroll.first, si.hscroll.total);

    if (d & FL_DAMAGE_ALL) {
        Fl_Group_draw_child(g, vs);
        Fl_Group_draw_child(g, hs);
        if (Fl_Widget_visible(vs) && Fl_Widget_visible(hs)) {
            fl_color(self_w->color);
            fl_rectf(vs->x, hs->y, vs->w, hs->h);
        }
    } else {
        if (Fl_Widget_visible(vs) && Fl_Widget_damage(vs)) { Fl_Widget_draw(vs); Fl_Widget_clear_damage(vs, 0); }
        if (Fl_Widget_visible(hs) && Fl_Widget_damage(hs)) { Fl_Widget_draw(hs); Fl_Widget_clear_damage(hs, 0); }
    }
}

void Fl_Scroll_resize(Fl_Widget *self_w, int X, int Y, int W, int H) {
    Fl_Scroll *self = (Fl_Scroll *)self_w;
    Fl_Group *g = &self->group;
    Fl_Widget *vs = sb_widget(&self->scrollbar);
    Fl_Widget *hs = sb_widget(&self->hscrollbar);
    int dx = X - self_w->x, dy = Y - self_w->y;
    int dw = W - self_w->w, dh = H - self_w->h;
    int nc, i;

    /* Deliberately NOT Fl_Group_resize(): children keep their size,
     * only shift position by the amount Fl_Scroll itself moved. */
    Fl_Widget_default_resize(self_w, X, Y, W, H);
    fix_scrollbar_order(self);

    nc = Fl_Group_children(g);
    for (i = 0; i < nc - 2; i++) {
        Fl_Widget *o = Fl_Group_child(g, i);
        Fl_Widget_position(o, o->x + dx, o->y + dy);
    }

    if (dw == 0 && dh == 0) {
        int pad = Fl_Widget_visible(vs) && Fl_Widget_visible(hs);
        int al = (Fl_Widget_align(vs) & FL_ALIGN_LEFT) != 0;
        int at = (Fl_Widget_align(vs) & FL_ALIGN_TOP) != 0;
        Fl_Widget_position(vs, al ? X : X + W - vs->w, (at && pad) ? Y + hs->h : Y);
        Fl_Widget_position(hs, (al && pad) ? X + vs->w : X, at ? Y : Y + H - hs->h);
    } else {
        Fl_Widget_redraw(self_w); /* full scrollbar recalculation happens in draw() */
    }
}

void Fl_Scroll_scroll_to(Fl_Scroll *self, int X, int Y) {
    Fl_Group *g = &self->group;
    Fl_Widget *self_w = &g->widget;
    Fl_Widget *vs = sb_widget(&self->scrollbar);
    Fl_Widget *hs = sb_widget(&self->hscrollbar);
    int dx = self->xposition_ - X;
    int dy = self->yposition_ - Y;
    int nc, i;

    if (!dx && !dy) return;
    self->xposition_ = X;
    self->yposition_ = Y;

    nc = Fl_Group_children(g);
    for (i = 0; i < nc; i++) {
        Fl_Widget *o = Fl_Group_child(g, i);
        if (o == hs || o == vs) continue;
        Fl_Widget_position(o, o->x + dx, o->y + dy);
    }
    /* Upstream also checks a color-scheme background special case here
     * (forces FL_DAMAGE_ALL); doesn't apply, cfltk has no color schemes. */
    Fl_Widget_set_damage(self_w, FL_DAMAGE_SCROLL);
}

int Fl_Scroll_handle(Fl_Widget *self_w, int event) {
    Fl_Scroll *self = (Fl_Scroll *)self_w;
    fix_scrollbar_order(self);
    return Fl_Group_handle(self_w, event);
}
