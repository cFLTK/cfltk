/*
 * cfltk - Fl_Tile.c
 * See include/cfltk/Fl_Tile.h for the class-conversion notes.
 * Translated from src/Fl_Tile.cxx.
 */
#include <stdlib.h>

#include "cfltk/Fl_Tile.h"
#include "cfltk/Fl.h"

/* Only one Fl_Tile can be mid-drag at a time process-wide -- matches
 * upstream's own function-static drag-state variables in
 * Fl_Tile::handle(). `s_sizes` is the pre-drag layout snapshot (see
 * header): [0..3] = the tile's own bounds, [4..7] = the resizable
 * child's clipped bounds, [8..] = one {x,right,y,bottom} block per
 * child in array order -- exactly upstream's Fl_Group::sizes() shape. */
static int *s_sizes = NULL;

#define DRAGH 1
#define DRAGV 2

static int s_sdrag = 0;
static int s_sdx = 0, s_sdy = 0;
static int s_sx = 0, s_sy = 0;

static int *build_sizes(Fl_Tile *self) {
    Fl_Group *g = &self->group;
    Fl_Widget *self_w = &g->widget;
    int *p;
    int i;
    Fl_Widget *r;

    free(s_sizes);
    p = s_sizes = (int *)malloc(sizeof(int) * 4 * (size_t)(Fl_Group_children(g) + 2));

    p[0] = self_w->x; p[2] = self_w->y;
    p[1] = p[0] + self_w->w; p[3] = p[2] + self_w->h;

    p[4] = p[0]; p[5] = p[1]; p[6] = p[2]; p[7] = p[3];
    r = Fl_Group_resizable(g);
    if (r && r != self_w) {
        int t;
        t = r->x; if (t > p[0]) p[4] = t;
        t += r->w; if (t < p[1]) p[5] = t;
        t = r->y; if (t > p[2]) p[6] = t;
        t += r->h; if (t < p[3]) p[7] = t;
    }

    p += 8;
    for (i = 0; i < Fl_Group_children(g); i++) {
        Fl_Widget *o = Fl_Group_child(g, i);
        *p++ = o->x;
        *p++ = o->x + o->w;
        *p++ = o->y;
        *p++ = o->y + o->h;
    }
    return s_sizes;
}

void Fl_Tile_position(Fl_Tile *self, int oldx, int oldy, int newx, int newy) {
    Fl_Group *g = &self->group;
    int *p = s_sizes;
    int i;

    if (!p) return;
    p += 8;
    for (i = 0; i < Fl_Group_children(g); i++, p += 4) {
        Fl_Widget *o = Fl_Group_child(g, i);
        int X, R, Y, B;
        if (o == Fl_Group_resizable(g)) continue;

        X = o->x;
        R = X + o->w;
        if (oldx) {
            int t = p[0];
            if (t == oldx || (t > oldx && X < newx) || (t < oldx && X > newx)) X = newx;
            t = p[1];
            if (t == oldx || (t > oldx && R < newx) || (t < oldx && R > newx)) R = newx;
        }
        Y = o->y;
        B = Y + o->h;
        if (oldy) {
            int t = p[2];
            if (t == oldy || (t > oldy && Y < newy) || (t < oldy && Y > newy)) Y = newy;
            t = p[3];
            if (t == oldy || (t > oldy && B < newy) || (t < oldy && B > newy)) B = newy;
        }
        Fl_Widget_damage_resize(o, X, Y, R - X, B - Y);
    }
}

static void tile_resize(Fl_Widget *self_w, int X, int Y, int W, int H) {
    Fl_Tile *self = (Fl_Tile *)self_w;
    Fl_Group *g = &self->group;
    int dx = X - self_w->x, dy = Y - self_w->y;
    int dw = W - self_w->w, dh = H - self_w->h;
    int *p = build_sizes(self); /* snapshot the pre-resize layout */
    int OR, NR, OB, NB;
    int i;

    Fl_Widget_default_resize(self_w, X, Y, W, H);

    OR = p[5];
    NR = X + W - (p[1] - OR);
    OB = p[7];
    NB = Y + H - (p[3] - OB);

    p += 8;
    for (i = 0; i < Fl_Group_children(g); i++) {
        Fl_Widget *o = Fl_Group_child(g, i);
        int xx = o->x + dx, R, yy = o->y + dy, B;
        R = xx + o->w;
        B = yy + o->h;

        if (*p++ >= OR) xx += dw; else if (xx > NR) xx = NR;
        if (*p++ >= OR) R += dw; else if (R > NR) R = NR;
        if (*p++ >= OB) yy += dh; else if (yy > NB) yy = NB;
        if (*p++ >= OB) B += dh; else if (B > NB) B = NB;

        Fl_Widget_resize(o, xx, yy, R - xx, B - yy);
    }
}

static int tile_handle(Fl_Widget *self_w, int event) {
    Fl_Tile *self = (Fl_Tile *)self_w;
    Fl_Group *g = &self->group;
    int mx = Fl_event_x(), my = Fl_event_y();

    switch (event) {
        case FL_MOVE:
        case FL_ENTER:
        case FL_PUSH: {
            int mindx = 100, mindy = 100, oldx = 0, oldy = 0;
            int *q, *p;
            int i;

            if (!Fl_Widget_active_r(self_w)) break;

            q = build_sizes(self);
            p = q + 8;
            for (i = 0; i < Fl_Group_children(g); i++, p += 4) {
                Fl_Widget *o = Fl_Group_child(g, i);
                if (o == Fl_Group_resizable(g)) continue;
                if (p[1] < q[1] && o->y <= my + 4 && o->y + o->h >= my - 4) {
                    int t = mx - (o->x + o->w);
                    if (abs(t) < mindx) { s_sdx = t; mindx = abs(t); oldx = p[1]; }
                }
                if (p[3] < q[3] && o->x <= mx + 4 && o->x + o->w >= mx - 4) {
                    int t = my - (o->y + o->h);
                    if (abs(t) < mindy) { s_sdy = t; mindy = abs(t); oldy = p[3]; }
                }
            }
            s_sdrag = 0; s_sx = s_sy = 0;
            if (mindx <= 4) { s_sdrag = DRAGH; s_sx = oldx; }
            if (mindy <= 4) { s_sdrag |= DRAGV; s_sy = oldy; }
            if (s_sdrag) return 1;
            return Fl_Group_handle(self_w, event);
        }

        case FL_LEAVE:
            break;

        case FL_DRAG:
        case FL_RELEASE: {
            Fl_Widget *r;
            int newx, newy;
            if (!s_sdrag) return 0;
            r = Fl_Group_resizable(g);
            if (!r) r = self_w;

            if (s_sdrag & DRAGH) {
                newx = Fl_event_x() - s_sdx;
                if (newx < r->x) newx = r->x;
                else if (newx > r->x + r->w) newx = r->x + r->w;
            } else {
                newx = s_sx;
            }
            if (s_sdrag & DRAGV) {
                newy = Fl_event_y() - s_sdy;
                if (newy < r->y) newy = r->y;
                else if (newy > r->y + r->h) newy = r->y + r->h;
            } else {
                newy = s_sy;
            }
            Fl_Tile_position(self, s_sx, s_sy, newx, newy);
            if (event == FL_DRAG) Fl_Widget_set_changed(self_w);
            Fl_Widget_do_callback(self_w);
            return 1;
        }

        default:
            break;
    }

    return Fl_Group_handle(self_w, event);
}

const Fl_WidgetOps fl_tile_ops = {
    Fl_Group_draw,
    tile_handle,
    tile_resize,
    NULL, NULL,
    Fl_Group_destroy,
    Fl_Group_as_group,
    NULL
};

void Fl_Tile_init(Fl_Tile *self, int x, int y, int w, int h, const char *label) {
    Fl_Group_init(&self->group, x, y, w, h, label);
    self->group.widget.ops = &fl_tile_ops;
}

Fl_Tile *Fl_Tile_new(int x, int y, int w, int h, const char *label) {
    Fl_Tile *self = (Fl_Tile *)malloc(sizeof(Fl_Tile));
    Fl_Tile_init(self, x, y, w, h, label);
    return self;
}
