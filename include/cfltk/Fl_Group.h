/*
 * cfltk - Fl_Group.h
 *
 * C translation of FLTK 1.3 FL/Fl_Group.H.
 *
 * Original class : Fl_Group : public Fl_Widget
 * New C structure : struct Fl_Group { Fl_Widget widget; ... }, embedding
 *                    Fl_Widget as its first member.
 * Inheritance     : Fl_Group IS-A Fl_Widget through embedding.
 * Vtbl            : fl_group_ops provides draw()/handle()/resize()/
 *                    destroy()/as_group(); every widget that contains
 *                    children (Fl_Window included) reuses these by
 *                    embedding Fl_Group and, where it needs to specialize
 *                    behavior, copying fl_group_ops into its own ops
 *                    table with the relevant slots replaced (see
 *                    Fl_Window.c).
 * Ownership       : a group owns every child added with Fl_Group_add();
 *                    Fl_Widget_delete() on a group recursively deletes
 *                    all children, matching upstream's
 *                    Fl_Group::~Fl_Group()/clear().
 * Known differences:
 *   - No hidden `sizes_` cache yet: Fl_Group_resize() uses upstream's
 *     proportional-resize algorithm but recomputes original child
 *     geometry from init_sizes() explicitly rather than lazily caching
 *     it on first resize; functionally equivalent, see Fl_Group.c.
 *   - No Forms-compatibility kludges (_ddfdesign_kludge, forms_end).
 */
#ifndef CFLTK_FL_GROUP_H
#define CFLTK_FL_GROUP_H

#include "cfltk/Fl_Widget.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Fl_Group {
    Fl_Widget widget;

    Fl_Widget **children_array; /* heap array, capacity >= children_count */
    int children_count;
    int children_capacity;

    Fl_Widget *saved_focus;
    Fl_Widget *resizable_widget; /* defaults to &group->widget, like upstream */
    int *sizes;                  /* NULL until Fl_Group_init_sizes(); 4 ints
                                     per child plus 4 for the group itself,
                                     same layout as upstream sizes_ */
};

extern const Fl_WidgetOps fl_group_ops;

void Fl_Group_init(Fl_Group *self, int x, int y, int w, int h, const char *label);
Fl_Group *Fl_Group_new(int x, int y, int w, int h, const char *label);

void Fl_Group_draw(Fl_Widget *self);
int  Fl_Group_handle(Fl_Widget *self, int event);
void Fl_Group_resize(Fl_Widget *self, int x, int y, int w, int h);
void Fl_Group_destroy(Fl_Widget *self);
Fl_Group *Fl_Group_as_group(Fl_Widget *self);

/* begin()/end()/current() -- upstream builds the widget tree implicitly:
 * any Fl_Widget_init() while a group is "current" auto-adds itself to it.
 * We keep that mechanism (Fl_Group_begin/_end/_current) because Dillo and
 * most FLTK example code rely on it to build layouts declaratively. */
void Fl_Group_begin(Fl_Group *self);
void Fl_Group_end(Fl_Group *self);
Fl_Group *Fl_Group_current(void);
void Fl_Group_set_current(Fl_Group *g);

static inline int Fl_Group_children(const Fl_Group *self) { return self->children_count; }
static inline Fl_Widget *Fl_Group_child(const Fl_Group *self, int n) { return self->children_array[n]; }
int Fl_Group_find(const Fl_Group *self, const Fl_Widget *w);

void Fl_Group_add(Fl_Group *self, Fl_Widget *w);
void Fl_Group_insert(Fl_Group *self, Fl_Widget *w, int index);
void Fl_Group_insert_before(Fl_Group *self, Fl_Widget *w, Fl_Widget *before);
void Fl_Group_remove(Fl_Group *self, Fl_Widget *w);
void Fl_Group_remove_at(Fl_Group *self, int index);
/* Deletes (Fl_Widget_delete) every child, recursively; leaves the group
 * itself intact and empty, mirroring Fl_Group::clear(). */
void Fl_Group_clear(Fl_Group *self);

static inline void Fl_Group_set_resizable(Fl_Group *self, Fl_Widget *w) { self->resizable_widget = w; }
static inline Fl_Widget *Fl_Group_resizable(const Fl_Group *self) { return self->resizable_widget; }
void Fl_Group_add_resizable(Fl_Group *self, Fl_Widget *w);
void Fl_Group_init_sizes(Fl_Group *self);

static inline void Fl_Group_set_clip_children(Fl_Group *self, int c) {
    if (c) self->widget.flags |= FL_WIDGET_CLIP_CHILDREN;
    else self->widget.flags &= ~(unsigned)FL_WIDGET_CLIP_CHILDREN;
}
static inline unsigned int Fl_Group_clip_children(const Fl_Group *self) {
    return (self->widget.flags & FL_WIDGET_CLIP_CHILDREN) ? 1u : 0u;
}

/* Internal use: called by Fl_Widget_activate()/_deactivate(). */
void Fl_Group_activate_children(Fl_Group *self);
void Fl_Group_deactivate_children(Fl_Group *self);

/* Internal use: draws one child respecting outside-label + damage rules,
 * used by Fl_Group_draw() and reusable by widgets that embed a Fl_Group
 * and want upstream's exact child-drawing behavior (e.g. Fl_Window). */
void Fl_Group_draw_child(Fl_Group *self, Fl_Widget *w);
void Fl_Group_draw_children(Fl_Group *self);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_GROUP_H */
