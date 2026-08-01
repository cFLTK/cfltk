/*
 * cfltk - Fl_Widget.h
 *
 * C translation of FLTK 1.3 FL/Fl_Widget.H.
 *
 * Original class : Fl_Widget (abstract base of every widget)
 * New C structure : struct Fl_Widget, always embedded as the first member
 *                    of every derived widget struct (Fl_Group, Fl_Window,
 *                    Fl_Box, ...), which is what makes plain pointer
 *                    up-casting safe (see FL_WIDGET() below).
 * Inheritance     : composition-by-embedding; see docs/DESIGN.md.
 * Vtbl            : Fl_WidgetOps, reached through Fl_Widget::ops. draw(),
 *                    handle(), resize(), show(), hide() and the destructor
 *                    are all virtual in upstream FLTK and become function
 *                    pointers here. as_group()/as_window() are kept as
 *                    virtuals too (upstream already uses them to avoid
 *                    dynamic_cast), and we reuse that mechanism to
 *                    implement checked casts instead of introducing a
 *                    separate RTTI tag.
 * Ownership       : a widget is owned by its parent Fl_Group once added
 *                    (Fl_Group_add). Top-level windows and not-yet-added
 *                    widgets are owned by whoever called their *_new().
 *                    Fl_Widget_delete() frees exactly one widget; deleting
 *                    a group deletes its children recursively (see
 *                    Fl_Group.h).
 * Known differences:
 *   - No hidden copy constructor/assignment-operator trap: just don't
 *     memcpy an Fl_Widget behind its own back.
 *   - Fl_Label::draw()/measure() become fl_label_draw()/fl_label_measure()
 *     in fl_draw.h instead of struct methods.
 *   - RTTI-avoidance uses as_group()/as_window() virtuals (matches
 *     upstream) rather than the type() Forms-compatibility byte, which is
 *     preserved unchanged for its original purpose.
 */
#ifndef CFLTK_FL_WIDGET_H
#define CFLTK_FL_WIDGET_H

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include "cfltk/Enumerations.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Fl_Widget Fl_Widget;
typedef struct Fl_Group Fl_Group;
typedef struct Fl_Window Fl_Window;
typedef struct Fl_Image Fl_Image;
typedef struct Fl_WidgetOps Fl_WidgetOps;

typedef void (Fl_Callback)(Fl_Widget *widget, void *data);
typedef void (Fl_Callback0)(Fl_Widget *widget);
typedef void (Fl_Callback1)(Fl_Widget *widget, long data);

/* Mirrors the protected Fl_Label struct. Plain data; drawing/measuring are
 * free functions in fl_draw.h (fl_label_draw, fl_label_measure). */
typedef struct Fl_Label {
    const char *value;
    Fl_Image *image;
    Fl_Image *deimage;
    Fl_Font font;
    Fl_Fontsize size;
    Fl_Color color;
    Fl_Align align;
    uchar type; /* Fl_Labeltype */
} Fl_Label;

/* Fl_Widget::flags_ bit values (protected enum in upstream). Exposed here
 * because C has no "protected"; treat as internal-use-only outside of
 * widget implementations. Values match upstream exactly. */
enum Fl_Widget_Flags {
    FL_WIDGET_INACTIVE       = 1u << 0,
    FL_WIDGET_INVISIBLE      = 1u << 1,
    FL_WIDGET_OUTPUT         = 1u << 2,
    FL_WIDGET_NOBORDER       = 1u << 3,
    FL_WIDGET_FORCE_POSITION = 1u << 4,
    FL_WIDGET_NON_MODAL      = 1u << 5,
    FL_WIDGET_SHORTCUT_LABEL = 1u << 6,
    FL_WIDGET_CHANGED        = 1u << 7,
    FL_WIDGET_OVERRIDE       = 1u << 8,
    FL_WIDGET_VISIBLE_FOCUS  = 1u << 9,
    FL_WIDGET_COPIED_LABEL   = 1u << 10,
    FL_WIDGET_CLIP_CHILDREN  = 1u << 11,
    FL_WIDGET_MENU_WINDOW    = 1u << 12,
    FL_WIDGET_TOOLTIP_WINDOW = 1u << 13,
    FL_WIDGET_MODAL          = 1u << 14,
    FL_WIDGET_NO_OVERLAY     = 1u << 15,
    FL_WIDGET_COPIED_TOOLTIP = 1u << 17,
    FL_WIDGET_FULLSCREEN     = 1u << 18
};

/* Damage bits (FL/Fl_Widget.H friends: Fl_Group/Fl_Window use these
 * directly, so upstream keeps them public via Fl_Widget.H comments near
 * fl_draw.H; reproduced here for the same reason). */
#define FL_DAMAGE_CHILD   0x01
#define FL_DAMAGE_EXPOSE  0x02
#define FL_DAMAGE_SCROLL  0x04
#define FL_DAMAGE_OVERLAY 0x08
#define FL_DAMAGE_USER1   0x10
#define FL_DAMAGE_USER2   0x20
#define FL_DAMAGE_ALL     0xFF

struct Fl_WidgetOps {
    void        (*draw)(Fl_Widget *self);
    int         (*handle)(Fl_Widget *self, int event);
    void        (*resize)(Fl_Widget *self, int x, int y, int w, int h);
    void        (*show)(Fl_Widget *self);
    void        (*hide)(Fl_Widget *self);
    /* Virtual destructor equivalent: tear down this widget's own state
     * (and recurse into children for groups) but do not free `self`
     * itself -- see Fl_Widget_delete(). */
    void        (*destroy)(Fl_Widget *self);
    Fl_Group   *(*as_group)(Fl_Widget *self);
    Fl_Window  *(*as_window)(Fl_Widget *self);
};

struct Fl_Widget {
    const Fl_WidgetOps *ops;

    Fl_Group *parent;
    Fl_Callback *callback;
    void *user_data;

    int x, y, w, h;

    Fl_Label label;

    unsigned int flags;
    Fl_Color color;
    Fl_Color color2;
    uchar type;
    uchar damage;
    uchar box;   /* Fl_Boxtype */
    uchar when;  /* Fl_When */

    const char *tooltip;
};

/* -------------------------------------------------------------------
 * Construction / destruction.
 *
 * Fl_Widget has no public constructor in upstream FLTK (protected ctor,
 * "you can't create one of these"); Fl_Widget_init() plays that role and
 * is meant to be called from a concrete widget's own _init(), e.g.
 * Fl_Box_init() calls Fl_Widget_init(&box->widget, &fl_box_ops, ...).
 * ---------------------------------------------------------------- */

void Fl_Widget_init(Fl_Widget *self, const Fl_WidgetOps *ops,
                     int x, int y, int w, int h, const char *label);

/* Shared teardown every concrete destroy() must call as its last step
 * (equivalent to the implicit base-class destructor call in C++):
 * removes the widget from its parent group and frees a copied
 * label/tooltip if owned. Does not free `self`. */
void Fl_Widget_base_destroy(Fl_Widget *self);

/* Virtual dispatch equivalent of `delete widget` without freeing memory:
 * calls ops->destroy(self). */
void Fl_Widget_destroy(Fl_Widget *self);

/* Virtual dispatch + free(self). Only call on heap-allocated widgets
 * obtained from a matching Fl_*_new(). Safe to call on NULL. */
void Fl_Widget_delete(Fl_Widget *self);

/* -------------------------------------------------------------------
 * Checked casts (contract requirement: never use C++-style casts).
 * In debug builds (NDEBUG not defined) these assert the dynamic type;
 * in release builds they compile down to a plain reinterpret cast.
 * ---------------------------------------------------------------- */

#define FL_WIDGET(x) ((Fl_Widget *)(x))

static inline Fl_Group *Fl_Widget_as_group(Fl_Widget *w) {
    return (w && w->ops && w->ops->as_group) ? w->ops->as_group(w) : NULL;
}
static inline Fl_Window *Fl_Widget_as_window(Fl_Widget *w) {
    return (w && w->ops && w->ops->as_window) ? w->ops->as_window(w) : NULL;
}

#ifndef NDEBUG
#define FL_GROUP(x)  (assert(Fl_Widget_as_group(FL_WIDGET(x)) != NULL), (Fl_Group *)(x))
#define FL_WINDOW(x) (assert(Fl_Widget_as_window(FL_WIDGET(x)) != NULL), (Fl_Window *)(x))
#else
#define FL_GROUP(x)  ((Fl_Group *)(x))
#define FL_WINDOW(x) ((Fl_Window *)(x))
#endif

/* -------------------------------------------------------------------
 * Virtual dispatch entry points.
 * ---------------------------------------------------------------- */

void Fl_Widget_draw(Fl_Widget *self);
int  Fl_Widget_handle(Fl_Widget *self, int event);
void Fl_Widget_resize(Fl_Widget *self, int x, int y, int w, int h);
void Fl_Widget_show(Fl_Widget *self);
void Fl_Widget_hide(Fl_Widget *self);
/* Resizes only if the geometry actually changed, then redraws. Returns
 * non-zero if it did. Used by Fl_Browser_ to reposition its scrollbars
 * without an unconditional redraw on every draw() pass. */
int Fl_Widget_damage_resize(Fl_Widget *self, int x, int y, int w, int h);

/* Default handle()/resize()/show()/hide() bodies, exposed so subclasses
 * that override one virtual can still call "the base class version" the
 * way C++ does with Fl_Widget::handle(event). */
int  Fl_Widget_default_handle(Fl_Widget *self, int event);
void Fl_Widget_default_resize(Fl_Widget *self, int x, int y, int w, int h);
void Fl_Widget_default_show(Fl_Widget *self);
void Fl_Widget_default_hide(Fl_Widget *self);

/* -------------------------------------------------------------------
 * Geometry.
 * ---------------------------------------------------------------- */

static inline int Fl_Widget_x(const Fl_Widget *self) { return self->x; }
static inline int Fl_Widget_y(const Fl_Widget *self) { return self->y; }
static inline int Fl_Widget_w(const Fl_Widget *self) { return self->w; }
static inline int Fl_Widget_h(const Fl_Widget *self) { return self->h; }

static inline void Fl_Widget_position(Fl_Widget *self, int X, int Y) {
    Fl_Widget_resize(self, X, Y, self->w, self->h);
}
static inline void Fl_Widget_size(Fl_Widget *self, int W, int H) {
    Fl_Widget_resize(self, self->x, self->y, W, H);
}

/* -------------------------------------------------------------------
 * Label / box / color / alignment.
 * ---------------------------------------------------------------- */

static inline const char *Fl_Widget_label(const Fl_Widget *self) { return self->label.value; }
void Fl_Widget_set_label(Fl_Widget *self, const char *text);
void Fl_Widget_copy_label(Fl_Widget *self, const char *new_label);

static inline int Fl_Widget_is_label_copied(const Fl_Widget *self) {
    return (self->flags & FL_WIDGET_COPIED_LABEL) ? 1 : 0;
}

static inline uchar Fl_Widget_labeltype(const Fl_Widget *self) { return self->label.type; }
static inline void Fl_Widget_set_labeltype(Fl_Widget *self, uchar t) { self->label.type = t; }

static inline Fl_Color Fl_Widget_labelcolor(const Fl_Widget *self) { return self->label.color; }
static inline void Fl_Widget_set_labelcolor(Fl_Widget *self, Fl_Color c) { self->label.color = c; }

static inline Fl_Font Fl_Widget_labelfont(const Fl_Widget *self) { return self->label.font; }
static inline void Fl_Widget_set_labelfont(Fl_Widget *self, Fl_Font f) { self->label.font = f; }

static inline Fl_Fontsize Fl_Widget_labelsize(const Fl_Widget *self) { return self->label.size; }
static inline void Fl_Widget_set_labelsize(Fl_Widget *self, Fl_Fontsize s) { self->label.size = s; }

static inline Fl_Align Fl_Widget_align(const Fl_Widget *self) { return self->label.align; }
static inline void Fl_Widget_set_align(Fl_Widget *self, Fl_Align a) { self->label.align = a; }

static inline uchar Fl_Widget_box(const Fl_Widget *self) { return self->box; }
static inline void Fl_Widget_set_box(Fl_Widget *self, uchar new_box) { self->box = new_box; }

static inline Fl_Color Fl_Widget_color(const Fl_Widget *self) { return self->color; }
static inline void Fl_Widget_set_color(Fl_Widget *self, Fl_Color bg) { self->color = bg; }

static inline Fl_Color Fl_Widget_selection_color(const Fl_Widget *self) { return self->color2; }
static inline void Fl_Widget_set_selection_color(Fl_Widget *self, Fl_Color c) { self->color2 = c; }
static inline void Fl_Widget_set_colors(Fl_Widget *self, Fl_Color bg, Fl_Color sel) {
    self->color = bg;
    self->color2 = sel;
}

static inline const char *Fl_Widget_tooltip(const Fl_Widget *self) { return self->tooltip; }
void Fl_Widget_set_tooltip(Fl_Widget *self, const char *text);

/* -------------------------------------------------------------------
 * Callbacks.
 * ---------------------------------------------------------------- */

static inline Fl_Callback *Fl_Widget_callback(const Fl_Widget *self) { return self->callback; }
static inline void Fl_Widget_set_callback(Fl_Widget *self, Fl_Callback *cb, void *data) {
    self->callback = cb;
    self->user_data = data;
}
static inline void *Fl_Widget_user_data(const Fl_Widget *self) { return self->user_data; }
static inline void Fl_Widget_set_user_data(Fl_Widget *self, void *v) { self->user_data = v; }
static inline long Fl_Widget_argument(const Fl_Widget *self) { return (long)(intptr_t)self->user_data; }
static inline void Fl_Widget_set_argument(Fl_Widget *self, long v) { self->user_data = (void *)(intptr_t)v; }

static inline uchar Fl_Widget_when(const Fl_Widget *self) { return self->when; }
static inline void Fl_Widget_set_when(Fl_Widget *self, uchar i) { self->when = i; }

void Fl_Widget_do_callback(Fl_Widget *self);
void Fl_Widget_do_callback_for(Fl_Widget *self, Fl_Widget *target, void *arg);
void Fl_Widget_default_callback(Fl_Widget *widget, void *data);

/* -------------------------------------------------------------------
 * Visibility / activation / state flags.
 * ---------------------------------------------------------------- */

static inline int Fl_Widget_visible(const Fl_Widget *self) { return !(self->flags & FL_WIDGET_INVISIBLE); }
int Fl_Widget_visible_r(const Fl_Widget *self);
static inline void Fl_Widget_set_visible(Fl_Widget *self) { self->flags &= ~(unsigned)FL_WIDGET_INVISIBLE; }
static inline void Fl_Widget_clear_visible(Fl_Widget *self) { self->flags |= FL_WIDGET_INVISIBLE; }

static inline int Fl_Widget_active(const Fl_Widget *self) { return !(self->flags & FL_WIDGET_INACTIVE); }
int Fl_Widget_active_r(const Fl_Widget *self);
void Fl_Widget_activate(Fl_Widget *self);
void Fl_Widget_deactivate(Fl_Widget *self);
static inline void Fl_Widget_set_active(Fl_Widget *self) { self->flags &= ~(unsigned)FL_WIDGET_INACTIVE; }
static inline void Fl_Widget_clear_active(Fl_Widget *self) { self->flags |= FL_WIDGET_INACTIVE; }

static inline int Fl_Widget_output(const Fl_Widget *self) { return (self->flags & FL_WIDGET_OUTPUT) ? 1 : 0; }
static inline void Fl_Widget_set_output(Fl_Widget *self) { self->flags |= FL_WIDGET_OUTPUT; }
static inline void Fl_Widget_clear_output(Fl_Widget *self) { self->flags &= ~(unsigned)FL_WIDGET_OUTPUT; }

static inline int Fl_Widget_takesevents(const Fl_Widget *self) {
    return !(self->flags & (FL_WIDGET_INACTIVE | FL_WIDGET_INVISIBLE | FL_WIDGET_OUTPUT));
}

static inline int Fl_Widget_changed(const Fl_Widget *self) { return (self->flags & FL_WIDGET_CHANGED) ? 1 : 0; }
static inline void Fl_Widget_set_changed(Fl_Widget *self) { self->flags |= FL_WIDGET_CHANGED; }
static inline void Fl_Widget_clear_changed(Fl_Widget *self) { self->flags &= ~(unsigned)FL_WIDGET_CHANGED; }

int Fl_Widget_take_focus(Fl_Widget *self);
static inline void Fl_Widget_set_visible_focus(Fl_Widget *self) { self->flags |= FL_WIDGET_VISIBLE_FOCUS; }
static inline void Fl_Widget_clear_visible_focus(Fl_Widget *self) { self->flags &= ~(unsigned)FL_WIDGET_VISIBLE_FOCUS; }
static inline int Fl_Widget_visible_focus(const Fl_Widget *self) { return (self->flags & FL_WIDGET_VISIBLE_FOCUS) ? 1 : 0; }

/* -------------------------------------------------------------------
 * Damage / redraw.
 * ---------------------------------------------------------------- */

static inline uchar Fl_Widget_damage(const Fl_Widget *self) { return self->damage; }
static inline void Fl_Widget_clear_damage(Fl_Widget *self, uchar c) { self->damage = c; }
void Fl_Widget_set_damage(Fl_Widget *self, uchar c);
void Fl_Widget_set_damage_area(Fl_Widget *self, uchar c, int x, int y, int w, int h);
void Fl_Widget_redraw(Fl_Widget *self);
void Fl_Widget_redraw_label(Fl_Widget *self);

/* -------------------------------------------------------------------
 * Tree navigation.
 * ---------------------------------------------------------------- */

static inline Fl_Group *Fl_Widget_parent(const Fl_Widget *self) { return self->parent; }
/* Internal use only, mirrors the "for hacks only" upstream comment: bypasses
 * Fl_Group's normal add/remove bookkeeping. Prefer Fl_Group_add/_remove. */
static inline void Fl_Widget_set_parent(Fl_Widget *self, Fl_Group *p) { self->parent = p; }

int Fl_Widget_contains(const Fl_Widget *self, const Fl_Widget *w);
static inline int Fl_Widget_inside(const Fl_Widget *self, const Fl_Widget *wgt) {
    return wgt ? Fl_Widget_contains(wgt, self) : 0;
}

Fl_Window *Fl_Widget_window(const Fl_Widget *self);
Fl_Window *Fl_Widget_top_window(const Fl_Widget *self);

static inline uchar Fl_Widget_type(const Fl_Widget *self) { return self->type; }
static inline void Fl_Widget_set_type(Fl_Widget *self, uchar t) { self->type = t; }

/* -------------------------------------------------------------------
 * Shortcut testing ('&x' in the label) and the focus-box/backdrop
 * drawing helpers upstream keeps protected on Fl_Widget for subclass
 * draw() methods (e.g. Fl_Button) to call.
 * ---------------------------------------------------------------- */

/* Unicode code point of the '&x' shortcut in label text t, or 0. Known
 * difference: decodes a single byte, not full UTF-8 -- non-ASCII label
 * shortcuts are unsupported until fl_utf8decode() is ported. */
unsigned int Fl_Widget_label_shortcut(const char *t);
int Fl_Widget_test_shortcut_str(const char *t, int require_alt);
/* True if flags has SHORTCUT_LABEL set and the current event matches
 * this widget's label() shortcut. */
int Fl_Widget_test_shortcut(const Fl_Widget *self);

/* Label drawing helpers a widget's draw() calls to paint its own
 * label(), translated from the three overloads of the protected
 * Fl_Widget::draw_label(). draw_label_at() always draws; draw_label_in()
 * skips drawing if align() requests an outside position (Fl_Group is
 * responsible for those, see Fl_Group_draw_children's
 * draw_outside_label()); draw_label() is the common case, using the
 * widget's own box-inset rectangle. */
void Fl_Widget_draw_label_at(const Fl_Widget *self, int x, int y, int w, int h, Fl_Align align);
void Fl_Widget_draw_label_in(const Fl_Widget *self, int x, int y, int w, int h);
void Fl_Widget_draw_label(const Fl_Widget *self);

void Fl_Widget_draw_focus(const Fl_Widget *self, uchar boxtype, int x, int y, int w, int h);
/* No-op until Fl_Image is implemented (label.image is always NULL for
 * now) -- kept as a call site so widgets that port draw_backdrop() calls
 * verbatim don't need editing again once images land. */
void Fl_Widget_draw_backdrop(const Fl_Widget *self);

#define FL_RESERVED_TYPE 100

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_WIDGET_H */
