/*
 * cfltk - Fl_Tabs.c
 * See include/cfltk/Fl_Tabs.h for the class-conversion notes.
 * Translated from src/Fl_Tabs.cxx.
 */
#include <stdlib.h>

#include "cfltk/Fl_Tabs.h"
#include "cfltk/Fl.h"
#include "cfltk/fl_draw.h"

#define BORDER 2
#define EXTRASPACE 10
#define SELECTION_BORDER 5

enum { TAB_LEFT, TAB_RIGHT, TAB_SELECTED };

static Fl_Group *Fl_Tabs_as_group(Fl_Widget *self) { return (Fl_Group *)self; }

const Fl_WidgetOps fl_tabs_ops = {
    Fl_Tabs_draw,
    Fl_Tabs_handle,
    Fl_Group_resize,
    Fl_Widget_default_show,
    Fl_Widget_default_hide,
    Fl_Tabs_destroy,
    Fl_Tabs_as_group,
    NULL
};

static void clear_tab_positions(Fl_Tabs *self) {
    if (self->tab_pos) { free(self->tab_pos); self->tab_pos = NULL; }
    if (self->tab_width) { free(self->tab_width); self->tab_width = NULL; }
}

void Fl_Tabs_init(Fl_Tabs *self, int x, int y, int w, int h, const char *label) {
    Fl_Group_init(&self->group, x, y, w, h, label);
    self->group.widget.ops = &fl_tabs_ops;
    self->group.widget.box = FL_THIN_UP_BOX;
    self->push_ = NULL;
    self->tab_pos = NULL;
    self->tab_width = NULL;
    self->tab_count = 0;
}

Fl_Tabs *Fl_Tabs_new(int x, int y, int w, int h, const char *label) {
    Fl_Tabs *self = (Fl_Tabs *)malloc(sizeof(Fl_Tabs));
    Fl_Tabs_init(self, x, y, w, h, label);
    return self;
}

void Fl_Tabs_destroy(Fl_Widget *self_w) {
    Fl_Tabs *self = (Fl_Tabs *)self_w;
    clear_tab_positions(self);
    Fl_Group_destroy(self_w);
}

/* Computes/caches tab_pos[]/tab_width[] (nc+1 / nc entries); returns the
 * index of the currently-visible child. */
static int tab_positions(Fl_Tabs *self) {
    Fl_Group *g = &self->group;
    Fl_Widget *w = &g->widget;
    int nc = Fl_Group_children(g);
    int selected = 0;
    int i;
    int prev_shortcut;
    int r;

    if (nc != self->tab_count) {
        clear_tab_positions(self);
        if (nc) {
            self->tab_pos = (int *)malloc((size_t)(nc + 1) * sizeof(int));
            self->tab_width = (int *)malloc((size_t)(nc + 1) * sizeof(int));
        }
        self->tab_count = nc;
    }
    if (nc == 0) return 0;

    prev_shortcut = fl_draw_shortcut;
    fl_draw_shortcut = 1;

    self->tab_pos[0] = fl_box_dx(w->box);
    for (i = 0; i < nc; i++) {
        Fl_Widget *o = Fl_Group_child(g, i);
        int wt = 0, ht = 0;
        if (Fl_Widget_visible(o)) selected = i;
        fl_label_measure(&o->label, &wt, &ht);
        self->tab_width[i] = wt + EXTRASPACE;
        self->tab_pos[i + 1] = self->tab_pos[i] + self->tab_width[i] + BORDER;
    }
    fl_draw_shortcut = prev_shortcut;

    r = w->w;
    if (self->tab_pos[i] <= r) return selected;
    /* too big: pack against the right edge */
    self->tab_pos[i] = r;
    for (i = nc; i--;) {
        int l = r - self->tab_width[i];
        if (self->tab_pos[i + 1] < l) l = self->tab_pos[i + 1];
        if (self->tab_pos[i] <= l) break;
        self->tab_pos[i] = l;
        r -= EXTRASPACE;
    }
    /* pack against the left edge and truncate width if still too big */
    for (i = 0; i < nc; i++) {
        int W;
        if (self->tab_pos[i] >= i * EXTRASPACE) break;
        self->tab_pos[i] = i * EXTRASPACE;
        W = w->w - 1 - EXTRASPACE * (nc - i) - self->tab_pos[i];
        if (self->tab_width[i] > W) self->tab_width[i] = W;
    }
    /* adjust edges according to visibility */
    for (i = nc; i > selected; i--)
        self->tab_pos[i] = self->tab_pos[i - 1] + self->tab_width[i - 1];
    return selected;
}

/* Space (height) needed for the tab strip; negative puts it at the
 * bottom. Full height if there are no children. */
static int tab_height(Fl_Tabs *self) {
    Fl_Group *g = &self->group;
    Fl_Widget *w = &g->widget;
    int nc = Fl_Group_children(g);
    int H, H2, i;

    if (nc == 0) return w->h;
    H = w->h;
    H2 = w->y;
    for (i = 0; i < nc; i++) {
        Fl_Widget *o = Fl_Group_child(g, i);
        if (o->y < w->y + H) H = o->y - w->y;
        if (o->y + o->h > H2) H2 = o->y + o->h;
    }
    H2 = w->y + w->h - H2;
    if (H2 > H) return (H2 <= 0) ? 0 : -H2;
    return (H <= 0) ? 0 : H;
}

Fl_Widget *Fl_Tabs_which(Fl_Tabs *self, int event_x, int event_y) {
    Fl_Group *g = &self->group;
    Fl_Widget *w = &g->widget;
    int nc = Fl_Group_children(g);
    int H, i;
    Fl_Widget *ret = NULL;

    if (nc == 0) return NULL;
    H = tab_height(self);
    if (H < 0) {
        if (event_y > w->y + w->h || event_y < w->y + w->h + H) return NULL;
    } else {
        if (event_y > w->y + H || event_y < w->y) return NULL;
    }
    if (event_x < w->x) return NULL;

    tab_positions(self);
    for (i = 0; i < nc; i++) {
        if (event_x < w->x + self->tab_pos[i + 1]) {
            ret = Fl_Group_child(g, i);
            break;
        }
    }
    return ret;
}

static void redraw_tabs(Fl_Tabs *self) {
    Fl_Widget *w = &self->group.widget;
    int H = tab_height(self);
    if (H >= 0) {
        H += fl_box_dy(w->box);
        Fl_Widget_set_damage_area(w, FL_DAMAGE_SCROLL, w->x, w->y, w->w, H);
    } else {
        H = fl_box_dy(w->box) - H;
        Fl_Widget_set_damage_area(w, FL_DAMAGE_SCROLL, w->x, w->y + w->h - H, w->w, H);
    }
}

Fl_Widget *Fl_Tabs_value(Fl_Tabs *self) {
    Fl_Group *g = &self->group;
    int nc = Fl_Group_children(g);
    Fl_Widget *v = NULL;
    int i;
    for (i = 0; i < nc; i++) {
        Fl_Widget *o = Fl_Group_child(g, i);
        if (v) Fl_Widget_hide(o);
        else if (Fl_Widget_visible(o)) v = o;
        else if (i == nc - 1) { Fl_Widget_show(o); v = o; }
    }
    return v;
}

int Fl_Tabs_set_value(Fl_Tabs *self, Fl_Widget *newvalue) {
    Fl_Group *g = &self->group;
    int nc = Fl_Group_children(g);
    int ret = 0;
    int i;
    for (i = 0; i < nc; i++) {
        Fl_Widget *o = Fl_Group_child(g, i);
        if (o == newvalue) {
            if (!Fl_Widget_visible(o)) ret = 1;
            Fl_Widget_show(o);
        } else {
            Fl_Widget_hide(o);
        }
    }
    return ret;
}

int Fl_Tabs_set_push(Fl_Tabs *self, Fl_Widget *o) {
    if (self->push_ == o) return 0;
    if ((self->push_ && !Fl_Widget_visible(self->push_)) || (o && !Fl_Widget_visible(o)))
        redraw_tabs(self);
    self->push_ = o;
    return 1;
}

void Fl_Tabs_client_area(Fl_Tabs *self, int *rx, int *ry, int *rw, int *rh, int tabh) {
    Fl_Group *g = &self->group;
    Fl_Widget *w = &g->widget;

    if (Fl_Group_children(g)) {
        Fl_Widget *c0 = Fl_Group_child(g, 0);
        *rx = c0->x; *ry = c0->y; *rw = c0->w; *rh = c0->h;
    } else {
        int y_offset, label_height;
        fl_font(Fl_Widget_labelfont(w), Fl_Widget_labelsize(w));
        label_height = fl_height() + BORDER * 2;

        if (tabh == 0) y_offset = label_height;
        else if (tabh == -1) y_offset = -label_height;
        else y_offset = tabh;

        *rx = w->x;
        *rw = w->w;
        if (y_offset >= 0) { *ry = w->y + y_offset; *rh = w->h - y_offset; }
        else { *ry = w->y; *rh = w->h + y_offset; }
    }
}

static void draw_tab(Fl_Tabs *self, int x1, int x2, int W, int H, Fl_Widget *o, int what) {
    Fl_Widget *tw = &self->group.widget;
    int sel = (what == TAB_SELECTED);
    int dh = fl_box_dh(tw->box);
    int dy = fl_box_dy(tw->box);
    int prev_shortcut = fl_draw_shortcut;
    uchar bt;
    int yofs;
    Fl_Color c, oc;

    fl_draw_shortcut = 1;
    bt = (o == self->push_ && !sel) ? fl_down(tw->box) : tw->box;
    yofs = sel ? 0 : BORDER;

    if ((x2 < x1 + W) && what == TAB_RIGHT) x1 = x2 - W;

    if (H >= 0) {
        if (sel) fl_push_clip(x1, tw->y, x2 - x1, H + dh - dy);
        else fl_push_clip(x1, tw->y, x2 - x1, H);

        H += dh;
        c = sel ? Fl_Widget_selection_color(tw) : Fl_Widget_selection_color(o);
        fl_draw_box(bt, x1, tw->y + yofs, W, H + 10 - yofs, c);

        oc = Fl_Widget_labelcolor(o);
        Fl_Widget_set_labelcolor(o, sel ? Fl_Widget_labelcolor(tw) : Fl_Widget_labelcolor(o));
        Fl_Widget_draw_label_at(o, x1, tw->y + yofs, W, H - yofs, FL_ALIGN_CENTER);
        Fl_Widget_set_labelcolor(o, oc);

        if (Fl_focus() == tw && Fl_Widget_visible(o))
            Fl_Widget_draw_focus(tw, tw->box, x1, tw->y, W, H);

        fl_pop_clip();
    } else {
        H = -H;
        if (sel) fl_push_clip(x1, tw->y + tw->h - H - dy, x2 - x1, H + dy);
        else fl_push_clip(x1, tw->y + tw->h - H, x2 - x1, H);

        H += dh;
        c = sel ? Fl_Widget_selection_color(tw) : Fl_Widget_selection_color(o);
        fl_draw_box(bt, x1, tw->y + tw->h - H - 10, W, H + 10 - yofs, c);

        oc = Fl_Widget_labelcolor(o);
        Fl_Widget_set_labelcolor(o, sel ? Fl_Widget_labelcolor(tw) : Fl_Widget_labelcolor(o));
        Fl_Widget_draw_label_at(o, x1, tw->y + tw->h - H, W, H - yofs, FL_ALIGN_CENTER);
        Fl_Widget_set_labelcolor(o, oc);

        if (Fl_focus() == tw && Fl_Widget_visible(o))
            Fl_Widget_draw_focus(tw, tw->box, x1, tw->y + tw->h - H, W, H);

        fl_pop_clip();
    }
    fl_draw_shortcut = prev_shortcut;
}

void Fl_Tabs_draw(Fl_Widget *self_w) {
    Fl_Tabs *self = (Fl_Tabs *)self_w;
    Fl_Group *g = &self->group;
    Fl_Widget *v = Fl_Tabs_value(self);
    int H = tab_height(self);

    if (self_w->damage & FL_DAMAGE_ALL) {
        Fl_Color c = v ? v->color : self_w->color;
        fl_draw_box(self_w->box, self_w->x, self_w->y + (H >= 0 ? H : 0), self_w->w, self_w->h - (H >= 0 ? H : -H), c);

        if (Fl_Widget_selection_color(self_w) != c) {
            int clip_y = (H >= 0) ? self_w->y + H : self_w->y + self_w->h + H - SELECTION_BORDER;
            fl_push_clip(self_w->x, clip_y, self_w->w, SELECTION_BORDER);
            fl_draw_box(self_w->box, self_w->x, clip_y, self_w->w, SELECTION_BORDER, Fl_Widget_selection_color(self_w));
            fl_pop_clip();
        }
        if (v) Fl_Group_draw_child(g, v);
    } else {
        if (v && Fl_Widget_damage(v)) { Fl_Widget_draw(v); Fl_Widget_clear_damage(v, 0); }
    }

    if (self_w->damage & (FL_DAMAGE_SCROLL | FL_DAMAGE_ALL)) {
        int nc = Fl_Group_children(g);
        int selected = tab_positions(self);
        int i;
        for (i = 0; i < selected; i++)
            draw_tab(self, self_w->x + self->tab_pos[i], self_w->x + self->tab_pos[i + 1],
                     self->tab_width[i], H, Fl_Group_child(g, i), TAB_LEFT);
        for (i = nc - 1; i > selected; i--)
            draw_tab(self, self_w->x + self->tab_pos[i], self_w->x + self->tab_pos[i + 1],
                     self->tab_width[i], H, Fl_Group_child(g, i), TAB_RIGHT);
        if (v) {
            i = selected;
            draw_tab(self, self_w->x + self->tab_pos[i], self_w->x + self->tab_pos[i + 1],
                     self->tab_width[i], H, Fl_Group_child(g, i), TAB_SELECTED);
        }
    }
}

int Fl_Tabs_handle(Fl_Widget *self_w, int event) {
    Fl_Tabs *self = (Fl_Tabs *)self_w;
    Fl_Group *g = &self->group;
    Fl_Widget *o;
    int i;

    switch (event) {
        case FL_PUSH: {
            int H = tab_height(self);
            if (H >= 0) {
                if (Fl_event_y() > self_w->y + H) return Fl_Group_handle(self_w, event);
            } else {
                if (Fl_event_y() < self_w->y + self_w->h + H) return Fl_Group_handle(self_w, event);
            }
        }
        /* fallthrough */
        case FL_DRAG:
        case FL_RELEASE:
            o = Fl_Tabs_which(self, Fl_event_x(), Fl_event_y());
            if (event == FL_RELEASE) {
                Fl_Tabs_set_push(self, NULL);
                if (o && Fl_visible_focus() && Fl_focus() != self_w) {
                    Fl_set_focus(self_w);
                    redraw_tabs(self);
                }
                if (o && (Fl_Tabs_set_value(self, o) || (Fl_Widget_when(self_w) & FL_WHEN_NOT_CHANGED))) {
                    Fl_Widget_Tracker wp;
                    Fl_Widget_Tracker_watch(&wp, o);
                    Fl_Widget_set_changed(self_w);
                    Fl_Widget_do_callback(self_w);
                    if (!Fl_Widget_Tracker_exists(&wp)) { Fl_Widget_Tracker_release(&wp); return 1; }
                    Fl_Widget_Tracker_release(&wp);
                }
            } else {
                Fl_Tabs_set_push(self, o);
            }
            return 1;
        case FL_FOCUS:
        case FL_UNFOCUS:
            if (!Fl_visible_focus()) return Fl_Group_handle(self_w, event);
            if (event == FL_RELEASE || event == FL_SHORTCUT || event == FL_KEYBOARD ||
                event == FL_FOCUS || event == FL_UNFOCUS) {
                redraw_tabs(self);
                if (event == FL_FOCUS) return Fl_Group_handle(self_w, event);
                if (event == FL_UNFOCUS) return 0;
                return 1;
            }
            return Fl_Group_handle(self_w, event);
        case FL_KEYBOARD:
            switch (Fl_event_key()) {
                case FL_Left:
                    if (!Fl_Group_children(g)) return 0;
                    if (Fl_Widget_visible(Fl_Group_child(g, 0))) return 0;
                    for (i = 1; i < Fl_Group_children(g); i++)
                        if (Fl_Widget_visible(Fl_Group_child(g, i))) break;
                    Fl_Tabs_set_value(self, Fl_Group_child(g, i - 1));
                    Fl_Widget_set_changed(self_w);
                    Fl_Widget_do_callback(self_w);
                    return 1;
                case FL_Right:
                    if (!Fl_Group_children(g)) return 0;
                    if (Fl_Widget_visible(Fl_Group_child(g, Fl_Group_children(g) - 1))) return 0;
                    for (i = 0; i < Fl_Group_children(g); i++)
                        if (Fl_Widget_visible(Fl_Group_child(g, i))) break;
                    Fl_Tabs_set_value(self, Fl_Group_child(g, i + 1));
                    Fl_Widget_set_changed(self_w);
                    Fl_Widget_do_callback(self_w);
                    return 1;
                case FL_Down:
                    Fl_Widget_redraw(self_w);
                    return Fl_Group_handle(self_w, FL_FOCUS);
                default:
                    break;
            }
            return Fl_Group_handle(self_w, event);
        case FL_SHORTCUT:
            for (i = 0; i < Fl_Group_children(g); i++) {
                Fl_Widget *c = Fl_Group_child(g, i);
                if (Fl_Widget_test_shortcut(c)) {
                    int sc = !Fl_Widget_visible(c);
                    Fl_Tabs_set_value(self, c);
                    if (sc) Fl_Widget_set_changed(self_w);
                    Fl_Widget_do_callback(self_w);
                    return 1;
                }
            }
            return Fl_Group_handle(self_w, event);
        case FL_SHOW:
            Fl_Tabs_value(self);
            return Fl_Group_handle(self_w, event);
        default:
            return Fl_Group_handle(self_w, event);
    }
}
