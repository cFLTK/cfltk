/*
 * cfltk - Fl_Tabs.h
 *
 * C translation of FLTK 1.3 FL/Fl_Tabs.H.
 *
 * Original class : Fl_Tabs : public Fl_Group (own draw()/handle(); the
 *                   "file card tabs" widget -- each direct child is a
 *                   card, its label() is printed on a tab, clicking a
 *                   tab show()s that child and hide()s all the others).
 * New C structure : struct Fl_Tabs { Fl_Group group; Fl_Widget *push_;
 *                    int *tab_pos; int *tab_width; int tab_count; };
 *                    embedding Fl_Group (which embeds Fl_Widget) as its
 *                    first member, same layering as Fl_Window.
 * Inheritance     : Fl_Tabs IS-A Fl_Group IS-A Fl_Widget through
 *                    embedding; FL_WIDGET(t), FL_GROUP(t) and plain
 *                    &t->group.widget all yield the same address.
 * Vtbl            : fl_tabs_ops -- draw()/handle() of its own, resize()
 *                    reuses Fl_Group_resize() as-is (proportional child
 *                    resize, no tab-specific behavior), destroy() frees
 *                    tab_pos/tab_width before delegating to
 *                    Fl_Group_destroy() for the children.
 * Ownership       : owns tab_pos/tab_width (recomputed whenever the
 *                    child count changes); children owned exactly like
 *                    any other Fl_Group.
 * Known differences: no tooltip integration (`Fl_Tooltip::current()`/
 *                   `::enter()` calls in upstream's FL_MOVE handler are
 *                   dropped -- cfltk has no tooltip subsystem, see
 *                   docs/DESIGN.md); FL_MOVE therefore falls through to
 *                   plain Fl_Group_handle(), losing only the
 *                   per-tab-hover tooltip switch, not any layout/value
 *                   behavior.
 */
#ifndef CFLTK_FL_TABS_H
#define CFLTK_FL_TABS_H

#include "cfltk/Fl_Group.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Fl_Tabs {
    Fl_Group group;
    Fl_Widget *push_;
    int *tab_pos;   /* nc+1 left edges, x-offsets from the tabs' own x() */
    int *tab_width; /* nc widths */
    int tab_count;  /* size the above two arrays were last allocated for */
} Fl_Tabs;

extern const Fl_WidgetOps fl_tabs_ops;

void Fl_Tabs_init(Fl_Tabs *self, int x, int y, int w, int h, const char *label);
Fl_Tabs *Fl_Tabs_new(int x, int y, int w, int h, const char *label);
void Fl_Tabs_destroy(Fl_Widget *self);

void Fl_Tabs_draw(Fl_Widget *self);
int Fl_Tabs_handle(Fl_Widget *self, int event);

/* The visible child (first visible one, or the last child forced
 * visible if none are); also normalizes visibility (hides every other
 * child), matching upstream's value()'s side effects. */
Fl_Widget *Fl_Tabs_value(Fl_Tabs *self);
/* Makes newvalue the visible tab, hides the rest. Returns non-zero if
 * that changed anything. */
int Fl_Tabs_set_value(Fl_Tabs *self, Fl_Widget *newvalue);

/* The tab currently down-clicked on (until FL_RELEASE), or NULL. */
static inline Fl_Widget *Fl_Tabs_push(const Fl_Tabs *self) { return self->push_; }
int Fl_Tabs_set_push(Fl_Tabs *self, Fl_Widget *o);

/* Child whose tab is under (event_x, event_y), or NULL. */
Fl_Widget *Fl_Tabs_which(Fl_Tabs *self, int event_x, int event_y);

/* Position/size available to children: (*rx,*ry,*rw,*rh). If there are
 * no children yet, tabh selects where the (not-yet-existing) tab strip
 * would go: 0 = calculated height at top, -1 = calculated height at
 * bottom, >0 = that height at top, <-1 = that height (negated) at
 * bottom. Ignored (uses child(0)'s geometry) once there are children. */
void Fl_Tabs_client_area(Fl_Tabs *self, int *rx, int *ry, int *rw, int *rh, int tabh);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_TABS_H */
