/*
 * cfltk - fl_menu_popup.c
 *
 * The menu popup engine backing Fl_Menu_Item_popup()/_pulldown() (see
 * Fl_Menu_Item.h) and, through them, Fl_Menu_Button/Fl_Choice/
 * Fl_Menu_Bar. Translated in *spirit*, not verbatim, from
 * src/Fl_Menu.cxx -- see docs/DESIGN.md for why.
 *
 * Upstream creates one native window per open submenu level (a
 * "menuwindow" stack) and relies on FLTK's own grab()/modal() plumbing
 * to route events across them. cfltk doesn't have that plumbing yet
 * (see docs/DESIGN.md's "No grab()/modal() stack" note), so this uses a
 * different, self-contained design: a single borderless popup Fl_Window
 * whose bounds are the union of every currently-open level, with a real
 * X11 pointer+keyboard grab (fl_backend_grab(), owner_events=False) so
 * every event -- even ones physically over some other window -- is
 * delivered to it, reported in that window's coordinate space. All
 * layout/hit-testing/drawing across levels happens by hand inside this
 * one window instead of relying on window-manager stacking. This trades
 * away upstream's per-platform "native popup" polish (e.g. real
 * transparency, WM interaction) for something that works identically
 * across any backend that implements fl_backend_grab().
 *
 * Only one popup can be open at a time (menu items opening another
 * popup from inside a callback is not supported), matching upstream's
 * own use of file-static globals for the currently-open menu chain.
 */
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "cfltk/Fl_Menu_.h"
#include "cfltk/Fl_Window.h"
#include "cfltk/Fl.h"
#include "cfltk/fl_draw.h"
#include "../backend/fl_backend.h"

#define FL_MENU_MAX_DEPTH 8
#define FL_MENU_ROW_PAD 4
#define FL_MENU_COL_PAD 16

typedef struct MenuLevel {
    const Fl_Menu_Item *items; /* first item of the array for this level */
    int x, y, w, h;              /* absolute (root) screen coordinates */
    int horizontal;               /* level 0 of a menubar pulldown */
    int selected;                  /* visible-item index, -1 = none */
} MenuLevel;

typedef struct MenuPopupState {
    Fl_Window *win;
    const Fl_Menu_ *owner;
    MenuLevel levels[FL_MENU_MAX_DEPTH];
    int nlevels;
    const Fl_Menu_Item *result;
    int active;
    int seen_push;
    /* Union of every level rectangle shown so far this session, in
     * absolute (root) coordinates. The window is only ever grown to
     * this, never shrunk -- see recompute_window_bounds(). */
    int union_x, union_y, union_x2, union_y2;
    /* Optional bold header line above level 0 (matches upstream's
     * popup(x,y,title,...) parameter). Reserved as extra window-local
     * space above levels[0]'s own y - see run()'s union_y adjustment
     * and popup_draw()'s title strip - rather than touching levels[0]
     * itself, so none of the row layout/hit-testing/drawing code above
     * needs to know about it at all. */
    const char *title;
    int title_h;
} MenuPopupState;

static MenuPopupState g_menu;

/* -------------------------------------------------------------------
 * Style (falls back to sensible defaults when popped up without an
 * owning Fl_Menu_, e.g. a bare context-menu Fl_Menu_Item_popup() call).
 * ---------------------------------------------------------------- */

static Fl_Color style_bgcolor(void) { return g_menu.owner ? Fl_Widget_color(&g_menu.owner->widget) : FL_BACKGROUND_COLOR; }

static int item_width(const Fl_Menu_Item *it) {
    return Fl_Menu_Item_measure(it, NULL, g_menu.owner);
}

static int item_height(const Fl_Menu_Item *it) {
    int h;
    Fl_Menu_Item_measure(it, &h, g_menu.owner);
    return h + FL_MENU_ROW_PAD;
}

/* Pixel span (row height for a vertical level, column width for a
 * horizontal one) a single item occupies, including divider spacing.
 * Shared by layout and hit-testing so they can never disagree. */
static int row_extent(const MenuLevel *lv, const Fl_Menu_Item *m) {
    int e = lv->horizontal ? (item_width(m) + FL_MENU_COL_PAD) : item_height(m);
    if (!lv->horizontal && (m->flags & FL_MENU_DIVIDER)) e += 3;
    return e;
}

/* -------------------------------------------------------------------
 * Layout
 * ---------------------------------------------------------------- */

static void clamp_to_screen(MenuLevel *lv, int anchor_x, int anchor_y, int anchor_w, int anchor_h) {
    int sw, sh;
    fl_backend_screen_size(&sw, &sh);
    if (lv->x + lv->w > sw) lv->x = (anchor_x - lv->w >= 0) ? anchor_x - lv->w : sw - lv->w;
    if (lv->x < 0) lv->x = 0;
    if (lv->y + lv->h > sh) lv->y = sh - lv->h;
    if (lv->y < 0) lv->y = 0;
    (void)anchor_w; (void)anchor_h;
}

static void layout_level(MenuLevel *lv, const Fl_Menu_Item *items, int horizontal, int x, int y, int preselect) {
    const Fl_Menu_Item *m;
    int maxw = 10, sumw = 0, maxh = 10, sumh = 0;

    lv->items = items;
    lv->horizontal = horizontal;
    lv->selected = preselect;

    for (m = Fl_Menu_Item_first(items); m->text; m = Fl_Menu_Item_next(m, 1)) {
        int iw = item_width(m);
        if (iw + 10 > maxw) maxw = iw + 10;
        if (horizontal) sumw += row_extent(lv, m);
        else sumh += row_extent(lv, m);
        { int ih = item_height(m); if (ih > maxh) maxh = ih; }
    }

    lv->x = x;
    lv->y = y;
    if (horizontal) {
        lv->w = sumw;
        lv->h = maxh;
    } else {
        lv->w = maxw;
        lv->h = sumh + 4;
    }
    clamp_to_screen(lv, x, y, lv->w, lv->h);
}

static const Fl_Menu_Item *level_item_at(const MenuLevel *lv, int visible_index) {
    const Fl_Menu_Item *m = Fl_Menu_Item_first(lv->items);
    int i;
    if (visible_index < 0) return NULL;
    for (i = 0; i < visible_index && m->text; i++) m = Fl_Menu_Item_next(m, 1);
    return (m && m->text) ? m : NULL;
}

static int level_hit_test(const MenuLevel *lv, int ax, int ay) {
    const Fl_Menu_Item *m;
    int i, pos;
    if (ax < lv->x || ax >= lv->x + lv->w || ay < lv->y || ay >= lv->y + lv->h) return -1;
    pos = lv->horizontal ? lv->x : lv->y;
    for (m = Fl_Menu_Item_first(lv->items), i = 0; m->text; m = Fl_Menu_Item_next(m, 1), i++) {
        int e = row_extent(lv, m);
        if (lv->horizontal ? (ax >= pos && ax < pos + e) : (ay >= pos && ay < pos + e)) return i;
        pos += e;
    }
    return -1;
}

/* Skips inactive/invisible items (Fl_Menu_Item_first/_next already
 * filter invisible; this additionally steps over inactive ones during
 * keyboard navigation). Returns -1 if none available in that direction. */
static int level_step(const MenuLevel *lv, int from, int dir) {
    int n = 0, i = from;
    const Fl_Menu_Item *m;
    for (m = Fl_Menu_Item_first(lv->items); m->text; m = Fl_Menu_Item_next(m, 1)) n++;
    if (n == 0) return -1;
    for (;;) {
        i += dir;
        if (i < 0 || i >= n) return from;
        if (Fl_Menu_Item_active(level_item_at(lv, i))) return i;
    }
}

/* -------------------------------------------------------------------
 * Level stack management
 * ---------------------------------------------------------------- */

static void recompute_window_bounds(void) {
    int i, minx = INT_MAX, miny = INT_MAX, maxx = INT_MIN, maxy = INT_MIN;
    for (i = 0; i < g_menu.nlevels; i++) {
        MenuLevel *lv = &g_menu.levels[i];
        if (lv->x < minx) minx = lv->x;
        if (lv->y < miny) miny = lv->y;
        if (lv->x + lv->w > maxx) maxx = lv->x + lv->w;
        if (lv->y + lv->h > maxy) maxy = lv->y + lv->h;
    }

    /* Grow the running union to cover the current levels; never shrink
     * it. If the window instead tracked only the *current* levels'
     * bbox, moving from one top-level menu to another (File -> Edit)
     * would shrink/move the window away from screen area it had
     * already drawn the old dropdown into, and nothing is guaranteed
     * to repaint that area afterwards (this popup is an
     * override-redirect window with no window manager involved, and
     * Expose delivery to whatever's behind it isn't reliable enough to
     * depend on) -- the old dropdown would visibly linger. Staying at
     * the session's max-ever footprint and always fully repainting the
     * window's background (popup_draw()) turns that into a plain
     * redraw instead of a cross-window exposure problem. */
    if (minx < g_menu.union_x) g_menu.union_x = minx;
    if (miny < g_menu.union_y) g_menu.union_y = miny;
    if (maxx > g_menu.union_x2) g_menu.union_x2 = maxx;
    if (maxy > g_menu.union_y2) g_menu.union_y2 = maxy;

    Fl_Widget_resize(FL_WIDGET(g_menu.win), g_menu.union_x, g_menu.union_y,
                      g_menu.union_x2 - g_menu.union_x, g_menu.union_y2 - g_menu.union_y);
    Fl_Widget_redraw(FL_WIDGET(g_menu.win));
}

static void truncate_to(int nlevels) {
    if (nlevels < 1) nlevels = 1;
    if (nlevels < g_menu.nlevels) {
        g_menu.nlevels = nlevels;
        recompute_window_bounds();
    }
}

/* If the item at (level, visible_index) is a submenu and isn't already
 * expanded as the next level, opens it (truncating any deeper,
 * unrelated levels first). No-op for a leaf item. */
static void expand_if_submenu(int level, int visible_index) {
    const Fl_Menu_Item *m = level_item_at(&g_menu.levels[level], visible_index);
    const Fl_Menu_Item *child_items;
    MenuLevel *parent;
    int x, y;

    if (!m || !Fl_Menu_Item_submenu(m) || !Fl_Menu_Item_active(m)) {
        truncate_to(level + 1);
        return;
    }
    if (g_menu.nlevels > level + 1 && g_menu.levels[level + 1].items ==
        ((m->flags & FL_SUBMENU_POINTER) ? (const Fl_Menu_Item *)m->user_data_ : m + 1)) {
        return; /* already expanded */
    }
    truncate_to(level + 1);
    if (g_menu.nlevels >= FL_MENU_MAX_DEPTH) return;

    child_items = (m->flags & FL_SUBMENU_POINTER) ? (const Fl_Menu_Item *)m->user_data_ : m + 1;
    parent = &g_menu.levels[level];
    if (parent->horizontal) {
        x = parent->x;
        {
            const Fl_Menu_Item *it;
            int i, pos = parent->x;
            for (it = Fl_Menu_Item_first(parent->items), i = 0; it->text && i < visible_index; it = Fl_Menu_Item_next(it, 1), i++)
                pos += row_extent(parent, it);
            x = pos;
        }
        y = parent->y + parent->h;
    } else {
        x = parent->x + parent->w;
        {
            const Fl_Menu_Item *it;
            int i, pos = parent->y;
            for (it = Fl_Menu_Item_first(parent->items), i = 0; it->text && i < visible_index; it = Fl_Menu_Item_next(it, 1), i++)
                pos += row_extent(parent, it);
            y = pos;
        }
    }

    layout_level(&g_menu.levels[g_menu.nlevels], child_items, 0, x, y, -1);
    g_menu.nlevels++;
    recompute_window_bounds();
}

/* -------------------------------------------------------------------
 * Drawing
 * ---------------------------------------------------------------- */

/* Thin wrapper converting absolute (root) coordinates to the popup
 * window's local space before delegating to the shared
 * Fl_Menu_Item_draw() (also used by Fl_Menu_Bar -- see Fl_Menu_Item.h).
 * cfltk's Fl_Menu_Item_draw() always draws the submenu arrow for a
 * FL_SUBMENU item, including at menubar top level, where upstream
 * suppresses it -- a minor, documented cosmetic difference. */
static void draw_row(const MenuLevel *lv, const Fl_Menu_Item *m, int ax, int ay, int w, int h, int selected) {
    Fl_Widget *win_w = FL_WIDGET(g_menu.win);
    (void)lv;
    Fl_Menu_Item_draw(m, ax - win_w->x, ay - win_w->y, w, h, g_menu.owner, selected);
}

static void draw_level(const MenuLevel *lv) {
    Fl_Widget *win_w = FL_WIDGET(g_menu.win);
    const Fl_Menu_Item *m;
    int i, pos;

    fl_draw_box(lv->horizontal ? FL_UP_BOX : FL_UP_BOX, lv->x - win_w->x, lv->y - win_w->y, lv->w, lv->h, style_bgcolor());

    pos = lv->horizontal ? lv->x : lv->y;
    for (m = Fl_Menu_Item_first(lv->items), i = 0; m->text; m = Fl_Menu_Item_next(m, 1), i++) {
        int e = row_extent(lv, m);
        if (lv->horizontal) draw_row(lv, m, pos, lv->y, e, lv->h, i == lv->selected);
        else draw_row(lv, m, lv->x, pos, lv->w, e, i == lv->selected);
        pos += e;
    }
}

static void popup_draw(Fl_Widget *self_w) {
    int i;
    /* self_w->x/y is this (top-level) window's screen position, not a
     * local drawable offset -- see Fl_Group_draw_children() in
     * Fl_Group.c for the general form of this fix. Was previously
     * masked here because draw_level() below always repaints the same
     * area using correctly window-relative coordinates. */
    fl_draw_box(FL_FLAT_BOX, 0, 0, self_w->w, self_w->h, style_bgcolor());
    if (g_menu.title) {
        /* Window-local y=0..title_h is exactly the space reserved
         * above levels[0] in run() - see union_y's comment there. */
        fl_draw_box(FL_UP_BOX, 0, 0, self_w->w, g_menu.title_h, style_bgcolor());
        fl_font(FL_HELVETICA_BOLD, FL_NORMAL_SIZE);
        fl_color(FL_FOREGROUND_COLOR);
        fl_draw(g_menu.title, FL_MENU_COL_PAD / 2, fl_height() - fl_descent() + FL_MENU_ROW_PAD / 2);
    }
    for (i = 0; i < g_menu.nlevels; i++) draw_level(&g_menu.levels[i]);
}

/* -------------------------------------------------------------------
 * Event handling
 * ---------------------------------------------------------------- */

static int hit_test_all(int ax, int ay, int *level_out) {
    int i;
    for (i = g_menu.nlevels - 1; i >= 0; i--) {
        int idx = level_hit_test(&g_menu.levels[i], ax, ay);
        if (idx >= 0) {
            *level_out = i;
            return idx;
        }
    }
    return -1;
}

static void update_hover(int ax, int ay) {
    int level, idx = hit_test_all(ax, ay, &level);
    if (idx < 0) return;
    if (g_menu.levels[level].selected != idx) {
        g_menu.levels[level].selected = idx;
        Fl_Widget_redraw(FL_WIDGET(g_menu.win));
    }
    expand_if_submenu(level, idx);
}

static void finish(const Fl_Menu_Item *result) {
    g_menu.result = result;
    g_menu.active = 0;
}

static int popup_handle(Fl_Widget *self_w, int event) {
    (void)self_w;
    switch (event) {
        case FL_PUSH:
            g_menu.seen_push = 1;
            update_hover(Fl_event_x_root(), Fl_event_y_root());
            {
                int level;
                int idx = hit_test_all(Fl_event_x_root(), Fl_event_y_root(), &level);
                if (idx < 0) finish(NULL);
            }
            return 1;

        case FL_MOVE:
        case FL_DRAG:
            update_hover(Fl_event_x_root(), Fl_event_y_root());
            return 1;

        case FL_RELEASE: {
            int level;
            int idx;
            if (!g_menu.seen_push) return 1; /* tail end of the opening click */
            idx = hit_test_all(Fl_event_x_root(), Fl_event_y_root(), &level);
            if (idx < 0) {
                finish(NULL);
            } else {
                const Fl_Menu_Item *m = level_item_at(&g_menu.levels[level], idx);
                if (m && !Fl_Menu_Item_submenu(m) && Fl_Menu_Item_active(m)) finish(m);
                /* else: released on a submenu header or inactive item -- stay open */
            }
            return 1;
        }

        case FL_KEYDOWN: {
            MenuLevel *lv = &g_menu.levels[g_menu.nlevels - 1];
            switch (Fl_event_key()) {
                case FL_Up:
                    if (!lv->horizontal) lv->selected = level_step(lv, lv->selected, -1);
                    Fl_Widget_redraw(FL_WIDGET(g_menu.win));
                    return 1;
                case FL_Down:
                    if (!lv->horizontal) lv->selected = level_step(lv, lv->selected, 1);
                    else if (lv->selected < 0) { lv->selected = level_step(lv, -1, 1); Fl_Widget_redraw(FL_WIDGET(g_menu.win)); }
                    else expand_if_submenu(g_menu.nlevels - 1, lv->selected);
                    Fl_Widget_redraw(FL_WIDGET(g_menu.win));
                    return 1;
                case FL_Right:
                    if (lv->horizontal) lv->selected = level_step(lv, lv->selected, 1);
                    else expand_if_submenu(g_menu.nlevels - 1, lv->selected);
                    Fl_Widget_redraw(FL_WIDGET(g_menu.win));
                    return 1;
                case FL_Left:
                    if (lv->horizontal) lv->selected = level_step(lv, lv->selected, -1);
                    else if (g_menu.nlevels > 1) truncate_to(g_menu.nlevels - 1);
                    Fl_Widget_redraw(FL_WIDGET(g_menu.win));
                    return 1;
                case FL_Enter:
                case FL_KP_Enter:
                case ' ': {
                    const Fl_Menu_Item *m = level_item_at(lv, lv->selected);
                    if (!m) return 1;
                    if (Fl_Menu_Item_submenu(m)) expand_if_submenu(g_menu.nlevels - 1, lv->selected);
                    else if (Fl_Menu_Item_active(m)) finish(m);
                    return 1;
                }
                case FL_Escape:
                    finish(NULL);
                    return 1;
                default:
                    return 1;
            }
        }

        case FL_SHOW:
        case FL_HIDE:
            return 1;

        default:
            return 0;
    }
}

static const Fl_WidgetOps popup_ops = {
    popup_draw,
    popup_handle,
    Fl_Window_resize,
    Fl_Window_show,
    Fl_Window_hide,
    Fl_Window_destroy,
    Fl_Window_as_group,
    Fl_Window_as_window
};

/* -------------------------------------------------------------------
 * Public entry points
 * ---------------------------------------------------------------- */

static const Fl_Menu_Item *run(const Fl_Menu_Item *self, int x, int y, int w, int h,
                                const Fl_Menu_ *owner, const Fl_Menu_Item *picked, int menubar,
                                const char *title) {
    int preselect = -1;

    memset(&g_menu, 0, sizeof(g_menu));
    g_menu.owner = owner;
    g_menu.active = 1;
    g_menu.title = title;
    if (title) {
        fl_font(FL_HELVETICA_BOLD, FL_NORMAL_SIZE);
        g_menu.title_h = fl_height() + 2 * FL_MENU_ROW_PAD;
    }

    if (picked) {
        const Fl_Menu_Item *m;
        int i;
        for (m = Fl_Menu_Item_first(self), i = 0; m->text; m = Fl_Menu_Item_next(m, 1), i++)
            if (m == picked) { preselect = i; break; }
    }

    layout_level(&g_menu.levels[0], self, menubar, x, y, preselect);
    if (menubar) {
        g_menu.levels[0].x = x;
        g_menu.levels[0].w = w;
        g_menu.levels[0].h = h;
    } else if (g_menu.levels[0].w < w) {
        g_menu.levels[0].w = w; /* pulldown at least as wide as its anchor button */
    }
    if (title) {
        int title_w;
        fl_font(FL_HELVETICA_BOLD, FL_NORMAL_SIZE); /* layout_level() above left a different font current */
        title_w = (int)fl_width(title, (int)strlen(title)) + FL_MENU_COL_PAD;
        if (g_menu.levels[0].w < title_w) g_menu.levels[0].w = title_w; /* don't clip a wide title */
    }
    g_menu.nlevels = 1;

    /* Reserve title_h of extra window space above levels[0] - the
     * level itself keeps its normal y, so none of the row hit-
     * testing/drawing code needs to account for the title at all (see
     * MenuPopupState.title_h's own comment). */
    g_menu.union_x = g_menu.levels[0].x;
    g_menu.union_y = g_menu.levels[0].y - g_menu.title_h;
    g_menu.union_x2 = g_menu.levels[0].x + g_menu.levels[0].w;
    g_menu.union_y2 = g_menu.levels[0].y + g_menu.levels[0].h;

    g_menu.win = Fl_Window_new(g_menu.union_x, g_menu.union_y,
                                g_menu.union_x2 - g_menu.union_x, g_menu.union_y2 - g_menu.union_y, NULL);
    Fl_Group_end(&g_menu.win->group);
    g_menu.win->group.widget.ops = &popup_ops;
    Fl_Window_set_border(g_menu.win, 0);

    Fl_Widget_show(FL_WIDGET(g_menu.win));
    fl_backend_grab(g_menu.win);

    while (g_menu.active) Fl_wait_for(1e20);

    fl_backend_ungrab();
    Fl_Widget_delete(FL_WIDGET(g_menu.win));
    g_menu.win = NULL;

    (void)w; (void)h;
    return g_menu.result;
}

const Fl_Menu_Item *Fl_Menu_Item_popup(const Fl_Menu_Item *self, int x, int y,
                                        const Fl_Menu_ *owner, const Fl_Menu_Item *picked) {
    return run(self, x, y, 1, 1, owner, picked, 0, NULL);
}

const Fl_Menu_Item *Fl_Menu_Item_popup_with_title(const Fl_Menu_Item *self, int x, int y,
                                                   const Fl_Menu_ *owner, const Fl_Menu_Item *picked,
                                                   const char *title) {
    return run(self, x, y, 1, 1, owner, picked, 0, title);
}

const Fl_Menu_Item *Fl_Menu_Item_pulldown(const Fl_Menu_Item *self, int x, int y, int w, int h,
                                           const Fl_Menu_ *owner, const Fl_Menu_Item *picked, int menubar) {
    if (menubar) return run(self, x, y, w, h, owner, picked, 1, NULL);
    return run(self, x, y + h, w, 1, owner, picked, 0, NULL);
}
