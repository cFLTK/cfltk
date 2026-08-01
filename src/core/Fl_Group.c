/*
 * cfltk - Fl_Group.c
 * See include/cfltk/Fl_Group.h for the class-conversion notes.
 * Translated from src/Fl_Group.cxx.
 */
#include <stdlib.h>
#include <string.h>

#include "cfltk/Fl_Group.h"
#include "cfltk/Fl_Window.h"
#include "cfltk/Fl.h"
#include "cfltk/fl_draw.h"

static Fl_Group *g_current_group = NULL;

/* -------------------------------------------------------------------
 * Vtable
 * ---------------------------------------------------------------- */

const Fl_WidgetOps fl_group_ops = {
    Fl_Group_draw,
    Fl_Group_handle,
    Fl_Group_resize,
    Fl_Widget_default_show,
    Fl_Widget_default_hide,
    Fl_Group_destroy,
    Fl_Group_as_group,
    NULL /* as_window */
};

Fl_Group *Fl_Group_as_group(Fl_Widget *self) { return (Fl_Group *)self; }

/* -------------------------------------------------------------------
 * Construction / destruction
 * ---------------------------------------------------------------- */

void Fl_Group_init(Fl_Group *self, int x, int y, int w, int h, const char *label) {
    Fl_Widget_init(&self->widget, &fl_group_ops, x, y, w, h, label);
    self->widget.label.align = FL_ALIGN_TOP;
    self->widget.box = FL_NO_BOX;

    self->children_array = NULL;
    self->children_count = 0;
    self->children_capacity = 0;
    self->saved_focus = NULL;
    self->resizable_widget = &self->widget;
    self->sizes = NULL;

    /* Fl_Widget_init() (called above) already added this group to
     * whatever group was current, exactly as it does for any other
     * widget. Only begin() is this class's own responsibility. */
    Fl_Group_begin(self);
}

Fl_Group *Fl_Group_new(int x, int y, int w, int h, const char *label) {
    Fl_Group *self = (Fl_Group *)malloc(sizeof(Fl_Group));
    Fl_Group_init(self, x, y, w, h, label);
    return self;
}

void Fl_Group_clear(Fl_Group *self) {
    int i;
    Fl_Widget *pushed = Fl_pushed();

    self->saved_focus = NULL;
    self->resizable_widget = &self->widget;
    free(self->sizes);
    self->sizes = NULL;

    if (Fl_Widget_contains(&self->widget, pushed)) Fl_set_pushed(&self->widget);

    /* Delete children back-to-front so widgets that remove siblings
     * from within their own destroy() don't invalidate the index we're
     * iterating on -- matches upstream's REVERSE_CHILDREN deletion order. */
    for (i = self->children_count - 1; i >= 0; i--) {
        Fl_Widget *child = self->children_array[i];
        child->parent = NULL; /* base_destroy would otherwise try to
                                  remove(child) from an array we're busy
                                  tearing down */
        Fl_Widget_delete(child);
    }

    free(self->children_array);
    self->children_array = NULL;
    self->children_count = 0;
    self->children_capacity = 0;
}

void Fl_Group_destroy(Fl_Widget *self_w) {
    Fl_Group *self = (Fl_Group *)self_w;
    Fl_Group_clear(self);
    Fl_Widget_base_destroy(self_w);
}

/* -------------------------------------------------------------------
 * current()/begin()/end()
 * ---------------------------------------------------------------- */

void Fl_Group_begin(Fl_Group *self) { g_current_group = self; }
void Fl_Group_end(Fl_Group *self) { g_current_group = self->widget.parent; }
Fl_Group *Fl_Group_current(void) { return g_current_group; }
void Fl_Group_set_current(Fl_Group *g) { g_current_group = g; }

/* -------------------------------------------------------------------
 * Children array
 * ---------------------------------------------------------------- */

int Fl_Group_find(const Fl_Group *self, const Fl_Widget *w) {
    int i;
    for (i = 0; i < self->children_count; i++)
        if (self->children_array[i] == w) return i;
    return self->children_count;
}

static void ensure_capacity(Fl_Group *self, int needed) {
    if (needed <= self->children_capacity) return;
    self->children_capacity = self->children_capacity ? self->children_capacity * 2 : 8;
    if (self->children_capacity < needed) self->children_capacity = needed;
    self->children_array = (Fl_Widget **)realloc(
        self->children_array, (size_t)self->children_capacity * sizeof(Fl_Widget *));
}

void Fl_Group_insert(Fl_Group *self, Fl_Widget *w, int index) {
    if (w->parent == self) {
        int old = Fl_Group_find(self, w);
        if (old < index) index--;
        if (old == index) return;
    }
    if (w->parent) Fl_Group_remove(w->parent, w);

    ensure_capacity(self, self->children_count + 1);
    memmove(&self->children_array[index + 1], &self->children_array[index],
            (size_t)(self->children_count - index) * sizeof(Fl_Widget *));
    self->children_array[index] = w;
    self->children_count++;
    w->parent = self;
}

void Fl_Group_insert_before(Fl_Group *self, Fl_Widget *w, Fl_Widget *before) {
    Fl_Group_insert(self, w, Fl_Group_find(self, before));
}

void Fl_Group_add(Fl_Group *self, Fl_Widget *w) {
    Fl_Group_insert(self, w, self->children_count);
}

void Fl_Group_add_resizable(Fl_Group *self, Fl_Widget *w) {
    self->resizable_widget = w;
    Fl_Group_add(self, w);
}

void Fl_Group_remove_at(Fl_Group *self, int index) {
    Fl_Widget *w;
    if (index < 0 || index >= self->children_count) return;
    w = self->children_array[index];
    w->parent = NULL;
    self->children_count--;
    memmove(&self->children_array[index], &self->children_array[index + 1],
            (size_t)(self->children_count - index) * sizeof(Fl_Widget *));
}

void Fl_Group_remove(Fl_Group *self, Fl_Widget *w) {
    if (w->parent != self) return;
    Fl_Group_remove_at(self, Fl_Group_find(self, w));
}

/* -------------------------------------------------------------------
 * Resize (proportional child resize, src/Fl_Group.cxx Fl_Group::resize)
 * ---------------------------------------------------------------- */

void Fl_Group_init_sizes(Fl_Group *self) {
    int i;
    int *p;
    free(self->sizes);
    self->sizes = (int *)malloc((size_t)(self->children_count + 1) * 4 * sizeof(int));
    p = self->sizes;

    p[0] = self->widget.x; p[1] = self->widget.x + self->widget.w;
    p[2] = self->widget.y; p[3] = self->widget.y + self->widget.h;
    p += 4;

    for (i = 0; i < self->children_count; i++) {
        Fl_Widget *w = self->children_array[i];
        p[0] = w->x; p[1] = w->x + w->w;
        p[2] = w->y; p[3] = w->y + w->h;
        p += 4;
    }
}

void Fl_Group_resize(Fl_Widget *self_w, int X, int Y, int W, int H) {
    Fl_Group *self = (Fl_Group *)self_w;
    int dx, dy, dw, dh;
    int *p;
    int i;
    /* A top-level (or, if ever supported, native subwindow) widget's
     * children already store window-local coordinates that don't
     * change when the WINDOW itself is repositioned on screen -- only
     * self_w's own x()/y() (used to place the native X window) should
     * move. Matches upstream's "if (type() >= FL_WINDOW) dx = dy = 0"
     * in both branches below (src/Fl_Group.cxx Fl_Group::resize()).
     * Missing this made Fl_Window_hotspot()/position() on any window
     * with a resizable_widget shift every child by the window's own
     * move delta on top of their already-correct position -- found
     * while implementing fl_ask.c's dialog, whose hotspot()-positioned
     * window has 1-3 buttons plus a resizable_widget. */
    int is_window = Fl_Widget_as_window(self_w) != NULL;

    dx = X - self_w->x;
    dy = Y - self_w->y;
    dw = W - self_w->w;
    dh = H - self_w->h;

    if (!self->children_count || !self->resizable_widget || (dw == 0 && dh == 0)) {
        Fl_Widget_default_resize(self_w, X, Y, W, H);
        if (self->children_count && !is_window) {
            for (i = 0; i < self->children_count; i++)
                Fl_Widget_position(self->children_array[i],
                                    self->children_array[i]->x + dx,
                                    self->children_array[i]->y + dy);
        }
        return;
    }

    if (!self->sizes) Fl_Group_init_sizes(self);
    p = self->sizes;

    Fl_Widget_default_resize(self_w, X, Y, W, H);

    if (is_window) { dx = 0; dy = 0; }

    {
        /* Defaults to the group's own original bounds; overridden below
         * if resizable_widget is one of the children. */
        int rx = p[0], rr = p[1], ry = p[2], rb = p[3];
        int resizable_index = Fl_Group_find(self, self->resizable_widget);
        if (resizable_index < self->children_count) {
            const int *rp = self->sizes + 4 * (resizable_index + 1);
            rx = rp[0]; rr = rp[1]; ry = rp[2]; rb = rp[3];
        }

        for (i = 0; i < self->children_count; i++) {
            Fl_Widget *w = self->children_array[i];
            const int *cp = self->sizes + 4 * (i + 1);
            int cx = cp[0], cr = cp[1], cy = cp[2], cb = cp[3];
            int nx, nr, ny, nb;

            if (cx >= rr) nx = cx + dw; else if (cx > rx) nx = rx + (cx - rx) * (rr + dw - rx) / (rr - rx); else nx = cx;
            if (cr >= rr) nr = cr + dw; else if (cr > rx) nr = rx + (cr - rx) * (rr + dw - rx) / (rr - rx); else nr = cr;
            if (cy >= rb) ny = cy + dh; else if (cy > ry) ny = ry + (cy - ry) * (rb + dh - ry) / (rb - ry); else ny = cy;
            if (cb >= rb) nb = cb + dh; else if (cb > ry) nb = ry + (cb - ry) * (rb + dh - ry) / (rb - ry); else nb = cb;

            Fl_Widget_resize(w, nx + dx, ny + dy, nr - nx, nb - ny);
        }
    }
}

/* -------------------------------------------------------------------
 * Active/visible propagation
 * ---------------------------------------------------------------- */

void Fl_Group_activate_children(Fl_Group *self) {
    int i;
    for (i = 0; i < self->children_count; i++) {
        Fl_Widget *w = self->children_array[i];
        if (Fl_Widget_active(w)) Fl_Widget_handle(w, FL_ACTIVATE);
    }
}

void Fl_Group_deactivate_children(Fl_Group *self) {
    int i;
    for (i = 0; i < self->children_count; i++) {
        Fl_Widget *w = self->children_array[i];
        if (Fl_Widget_active(w)) Fl_Widget_handle(w, FL_DEACTIVATE);
    }
}

/* -------------------------------------------------------------------
 * Drawing
 * ---------------------------------------------------------------- */

void Fl_Group_draw_child(Fl_Group *self, Fl_Widget *w) {
    (void)self;
    if (!Fl_Widget_visible(w)) return;
    Fl_Widget_clear_damage(w, FL_DAMAGE_ALL);
    Fl_Widget_draw(w);
    Fl_Widget_clear_damage(w, 0);
}

void Fl_Group_draw_outside_label(const Fl_Widget *w) {
    Fl_Align a = Fl_Widget_align(w);
    int lx = w->x, ly = w->y, lw = w->w, lh = w->h;

    if (!(a & FL_ALIGN_INSIDE) && Fl_Widget_label(w) && Fl_Widget_label(w)[0]) {
        int mw, mh;
        fl_label_measure(&w->label, &mw, &mh);

        if (a & FL_ALIGN_TOP) { ly -= mh; lh = mh; a = (a & ~(FL_ALIGN_TOP | FL_ALIGN_BOTTOM)) | FL_ALIGN_BOTTOM; }
        else if (a & FL_ALIGN_BOTTOM) { ly += lh; lh = mh; a = (a & ~(FL_ALIGN_TOP | FL_ALIGN_BOTTOM)) | FL_ALIGN_TOP; }
        else if (a & FL_ALIGN_LEFT) { lw = mw; lx -= mw; }
        else if (a & FL_ALIGN_RIGHT) { lw = mw; lx += w->w; }
        else return;

        fl_label_draw(&w->label, lx, ly, lw, lh, a);
    }
}

void Fl_Group_draw_children(Fl_Group *self) {
    int i;
    Fl_Widget *self_w = &self->widget;
    /* A window's own x()/y() are its screen position (used to place its
     * native X window), not an offset within its own drawable -- unlike
     * every other widget, whose x()/y() are already local to the
     * drawable they're painted into. Drawing this group's own box at
     * (self_w->x, self_w->y) is only correct when self_w is a
     * non-window child of some other group; for a widget that is
     * itself a window (top-level or, if ever supported, a native
     * subwindow), the box must be drawn at its own local origin (0,0)
     * instead. Missing this shifted+clipped every non-(0,0)-positioned
     * window's own background (found while implementing Fl_Tooltip's
     * popup window, which is never at (0,0)); see docs/DESIGN.md. */
    int is_window = Fl_Widget_as_window(self_w) != NULL;
    int bx = is_window ? 0 : self_w->x;
    int by = is_window ? 0 : self_w->y;

    if (Fl_Widget_damage(self_w) & FL_DAMAGE_ALL) {
        if (Fl_Widget_box(self_w) != FL_NO_BOX || self_w->parent == NULL)
            fl_draw_box(Fl_Widget_box(self_w), bx, by, self_w->w, self_w->h, Fl_Widget_color(self_w));
        for (i = 0; i < self->children_count; i++) {
            Fl_Widget *w = self->children_array[i];
            Fl_Group_draw_outside_label(w);
            Fl_Group_draw_child(self, w);
        }
    } else {
        for (i = 0; i < self->children_count; i++) {
            Fl_Widget *w = self->children_array[i];
            if (Fl_Widget_damage(w)) Fl_Group_draw_child(self, w);
        }
    }
}

void Fl_Group_draw(Fl_Widget *self_w) {
    Fl_Group_draw_children((Fl_Group *)self_w);
}

/* -------------------------------------------------------------------
 * Event handling (src/Fl_Group.cxx Fl_Group::handle)
 * ---------------------------------------------------------------- */

static int send(Fl_Widget *o, int event) {
    int ret;
    if (!Fl_Widget_as_window(o)) {
        /* Non-window children get events unmodified. */
        return Fl_Widget_handle(o, event);
    }
    {
        int sx = Fl_event_x(), sy = Fl_event_y();
        Fl_context_set_event_xy(sx - o->x, sy - o->y);
        ret = Fl_Widget_handle(o, event);
        Fl_context_set_event_xy(sx, sy);
    }
    return ret;
}

static int navkey(void) {
    if (Fl_event_state_of(FL_CTRL | FL_ALT | FL_META)) return 0;
    switch (Fl_event_key()) {
        case 0: break;
        case FL_Tab: return Fl_event_state_of(FL_SHIFT) ? FL_Left : FL_Right;
        case FL_Right: return FL_Right;
        case FL_Left: return FL_Left;
        case FL_Up: return FL_Up;
        case FL_Down: return FL_Down;
        default: break;
    }
    return 0;
}

static int navigation(Fl_Group *self, int key) {
    int i;
    Fl_Widget *previous, *o;

    if (self->children_count <= 1) return 0;
    for (i = 0; ; i++) {
        if (i >= self->children_count) return 0;
        if (Fl_Widget_contains(self->children_array[i], Fl_focus())) break;
    }
    previous = self->children_array[i];

    for (;;) {
        switch (key) {
            case FL_Right:
            case FL_Down:
                i++;
                if (i >= self->children_count) {
                    if (self->widget.parent) return 0;
                    i = 0;
                }
                break;
            case FL_Left:
            case FL_Up:
                if (i) i--;
                else {
                    if (self->widget.parent) return 0;
                    i = self->children_count - 1;
                }
                break;
            default:
                return 0;
        }
        o = self->children_array[i];
        if (o == previous) return 0;
        if (key == FL_Down || key == FL_Up) {
            if (o->x >= previous->x + previous->w || o->x + o->w <= previous->x) continue;
        }
        if (Fl_Widget_take_focus(o)) return 1;
    }
}

int Fl_Group_handle(Fl_Widget *self_w, int event) {
    Fl_Group *self = (Fl_Group *)self_w;
    Fl_Widget **a = self->children_array;
    int i;
    Fl_Widget *o;

    switch (event) {
        case FL_FOCUS:
            switch (navkey()) {
                default:
                    if (self->saved_focus && Fl_Widget_take_focus(self->saved_focus)) return 1;
                    /* fallthrough */
                case FL_Right:
                case FL_Down:
                    for (i = self->children_count - 1; i >= 0; i--) if (Fl_Widget_take_focus(a[i])) return 1;
                    break;
                case FL_Left:
                case FL_Up:
                    for (i = self->children_count - 1; i >= 0; i--) if (Fl_Widget_take_focus(a[i])) return 1;
                    break;
            }
            return 0;

        case FL_UNFOCUS:
            self->saved_focus = Fl_focus();
            return 0;

        case FL_KEYBOARD:
            return navigation(self, navkey());

        case FL_SHORTCUT:
            for (i = self->children_count - 1; i >= 0; i--) {
                o = a[i];
                if (Fl_Widget_takesevents(o) && Fl_event_inside(o) && send(o, FL_SHORTCUT)) return 1;
            }
            for (i = self->children_count - 1; i >= 0; i--) {
                o = a[i];
                if (Fl_Widget_takesevents(o) && !Fl_event_inside(o) && send(o, FL_SHORTCUT)) return 1;
            }
            if (Fl_event_key() == FL_Enter) return navigation(self, FL_Down);
            return 0;

        case FL_ENTER:
        case FL_MOVE:
            for (i = self->children_count - 1; i >= 0; i--) {
                o = a[i];
                if (Fl_Widget_visible(o) && Fl_event_inside(o)) {
                    if (Fl_Widget_contains(o, Fl_belowmouse())) return send(o, FL_MOVE);
                    Fl_set_belowmouse(o);
                    if (send(o, FL_ENTER)) return 1;
                }
            }
            Fl_set_belowmouse(self_w);
            return 1;

        case FL_PUSH: {
            for (i = self->children_count - 1; i >= 0; i--) {
                Fl_Widget_Tracker wp;
                o = a[i];
                if (!(Fl_Widget_takesevents(o) && Fl_event_inside(o))) continue;
                Fl_Widget_Tracker_watch(&wp, o);
                if (send(o, FL_PUSH)) {
                    if (Fl_pushed() && Fl_Widget_Tracker_exists(&wp) && !Fl_Widget_contains(o, Fl_pushed()))
                        Fl_set_pushed(o);
                    Fl_Widget_Tracker_release(&wp);
                    return 1;
                }
                Fl_Widget_Tracker_release(&wp);
            }
            return 0;
        }

        case FL_RELEASE:
        case FL_DRAG:
            o = Fl_pushed();
            if (o == self_w) return 0;
            else if (o) send(o, event);
            else {
                for (i = self->children_count - 1; i >= 0; i--) {
                    o = a[i];
                    if (Fl_Widget_takesevents(o) && Fl_event_inside(o) && send(o, event)) return 1;
                }
            }
            return 0;

        case FL_MOUSEWHEEL:
            for (i = self->children_count - 1; i >= 0; i--) {
                o = a[i];
                if (Fl_Widget_takesevents(o) && Fl_event_inside(o) && send(o, FL_MOUSEWHEEL)) return 1;
            }
            for (i = self->children_count - 1; i >= 0; i--) {
                o = a[i];
                if (Fl_Widget_takesevents(o) && !Fl_event_inside(o) && send(o, FL_MOUSEWHEEL)) return 1;
            }
            return 0;

        case FL_DEACTIVATE:
        case FL_ACTIVATE:
            for (i = self->children_count - 1; i >= 0; i--) {
                o = a[i];
                if (Fl_Widget_active(o)) Fl_Widget_handle(o, event);
            }
            return 1;

        case FL_SHOW:
        case FL_HIDE:
            for (i = self->children_count - 1; i >= 0; i--) {
                o = a[i];
                if (event == FL_HIDE && o == Fl_focus()) {
                    Fl_Widget_handle(o, FL_UNFOCUS);
                    Fl_set_focus(NULL);
                }
                if (Fl_Widget_visible(o)) Fl_Widget_handle(o, event);
            }
            return 1;

        default:
            if (self->children_count == 0) return 0;
            for (i = 0; i < self->children_count; i++) if (Fl_focus() == a[i]) break;
            if (i >= self->children_count) i = 0;
            {
                int j = i;
                for (;;) {
                    if (Fl_Widget_takesevents(a[j])) { if (send(a[j], event)) return 1; }
                    j++;
                    if (j >= self->children_count) j = 0;
                    if (j == i) break;
                }
            }
            return 0;
    }
}
