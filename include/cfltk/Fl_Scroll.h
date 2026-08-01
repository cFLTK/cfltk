/*
 * cfltk - Fl_Scroll.h
 *
 * C translation of FLTK 1.3 FL/Fl_Scroll.H.
 *
 * Original class : Fl_Scroll : public Fl_Group (own draw()/handle()/
 *                   resize(); a viewport onto a set of children larger
 *                   than the widget itself, with a vertical and/or
 *                   horizontal Fl_Scrollbar that appear automatically
 *                   as needed).
 * New C structure : struct Fl_Scroll { Fl_Group group; Fl_Scrollbar
 *                    scrollbar; Fl_Scrollbar hscrollbar; int
 *                    xposition_, yposition_, oldx, oldy,
 *                    scrollbar_size_; }. The two scrollbars are plain
 *                    embedded members, not heap-allocated children --
 *                    same ownership shape as upstream, and safe for the
 *                    same reason: Fl_Scrollbar_destroy() (called
 *                    explicitly by Fl_Scroll_destroy() before
 *                    Fl_Group_destroy() walks the remaining, genuinely
 *                    heap-allocated children) ends in
 *                    Fl_Widget_base_destroy(), which removes the
 *                    scrollbar from the group's children array first --
 *                    exactly mirroring how upstream's automatic-storage
 *                    `scrollbar`/`hscrollbar` members remove themselves
 *                    from the children array in ~Fl_Widget() before
 *                    Fl_Group::clear() ever gets a chance to `delete`
 *                    them.
 * Inheritance     : Fl_Scroll IS-A Fl_Group IS-A Fl_Widget through
 *                    embedding, same layering as Fl_Window/Fl_Tabs.
 * Vtbl            : fl_scroll_ops -- draw()/handle()/resize()/destroy()
 *                    of its own; resize() deliberately does NOT reuse
 *                    Fl_Group_resize() (matches upstream: children keep
 *                    their size, only their position shifts by the
 *                    amount the Fl_Scroll itself moved).
 * Ownership       : owns the two embedded scrollbars (see above); real
 *                    content children owned exactly like any other
 *                    Fl_Group.
 * Known differences:
 *   - No accelerated "shift already-drawn pixels, redraw only the newly
 *     exposed strip" blit (upstream's fl_scroll(), backed by a
 *     platform-specific copy-area primitive cfltk hasn't ported).
 *     FL_DAMAGE_SCROLL just redraws the whole visible content area
 *     instead -- same pixels end up on screen, less efficient.
 *   - No color-scheme tiled background (`Fl::scheme_bg_`) -- see the
 *     project-wide "no color schemes" note in docs/DESIGN.md; the
 *     content area is always filled with color() before drawing
 *     children, which is upstream's own fallback for every box type
 *     when no scheme background is set.
 *   - The ScrollInfo struct and recalc_scrollbars() upstream exposes as
 *     `protected` (for subclasses that need to know the computed
 *     viewport/scrollbar geometry) are kept file-private here -- no
 *     cfltk subclass needs them yet. Straightforward to promote to the
 *     header if one does.
 */
#ifndef CFLTK_FL_SCROLL_H
#define CFLTK_FL_SCROLL_H

#include "cfltk/Fl_Group.h"
#include "cfltk/Fl_Scrollbar.h"

#ifdef __cplusplus
extern "C" {
#endif

/* type() values -- bit flags, same numeric values as upstream. */
#define FL_SCROLL_HORIZONTAL        1
#define FL_SCROLL_VERTICAL          2
#define FL_SCROLL_BOTH              3
#define FL_SCROLL_ALWAYS_ON         4
#define FL_SCROLL_HORIZONTAL_ALWAYS 5
#define FL_SCROLL_VERTICAL_ALWAYS   6
#define FL_SCROLL_BOTH_ALWAYS      7

typedef struct Fl_Scroll {
    Fl_Group group;
    Fl_Scrollbar scrollbar;  /* vertical */
    Fl_Scrollbar hscrollbar; /* horizontal */
    int xposition_, yposition_;
    int oldx, oldy;
    int scrollbar_size_;
} Fl_Scroll;

extern const Fl_WidgetOps fl_scroll_ops;

void Fl_Scroll_init(Fl_Scroll *self, int x, int y, int w, int h, const char *label);
Fl_Scroll *Fl_Scroll_new(int x, int y, int w, int h, const char *label);
void Fl_Scroll_destroy(Fl_Widget *self);

void Fl_Scroll_draw(Fl_Widget *self);
int Fl_Scroll_handle(Fl_Widget *self, int event);
void Fl_Scroll_resize(Fl_Widget *self, int x, int y, int w, int h);

/* Deletes every child except the two scrollbars, matching upstream's
 * clear() override (temporarily removes the scrollbars, calls the
 * plain Fl_Group clear, re-adds them). */
void Fl_Scroll_clear(Fl_Scroll *self);

static inline int Fl_Scroll_xposition(const Fl_Scroll *self) { return self->xposition_; }
static inline int Fl_Scroll_yposition(const Fl_Scroll *self) { return self->yposition_; }
void Fl_Scroll_scroll_to(Fl_Scroll *self, int x, int y);

static inline int Fl_Scroll_scrollbar_size(const Fl_Scroll *self) { return self->scrollbar_size_; }
static inline void Fl_Scroll_set_scrollbar_size(Fl_Scroll *self, int new_size) {
    if (new_size != self->scrollbar_size_) Fl_Widget_redraw(&self->group.widget);
    self->scrollbar_size_ = new_size;
}

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_SCROLL_H */
