/*
 * cfltk - Fl_Tile.h
 *
 * C translation of FLTK 1.3 FL/Fl_Tile.H.
 *
 * Original class : Fl_Tile : public Fl_Group -- lets the user resize
 *                   its children by dragging the shared border between
 *                   them (children must together exactly tile the
 *                   group's area, touching at their edges with no
 *                   gaps). The optional resizable() child clamps how
 *                   far borders can be dragged.
 * New C structure : struct Fl_Tile { Fl_Group group; }, embedding
 *                    Fl_Group as its first member. No fields of its
 *                    own -- see Known differences for where its
 *                    "saved original layout" snapshot lives instead
 *                    of a struct field.
 * Vtbl            : fl_tile_ops -- handle()/resize() are overridden
 *                    (resize() deliberately does NOT call
 *                    Fl_Group_resize(): Fl_Tile implements its own
 *                    enlarge/shrink-preserving-anchors algorithm);
 *                    draw() reuses Fl_Group's (Fl_Tile draws no
 *                    graphics of its own -- the "ridges" you see are
 *                    each child's own box, e.g. FL_DOWN_BOX, drawn
 *                    edge-to-edge against its neighbor).
 * Known differences:
 *   - The saved pre-drag/pre-resize child layout snapshot (upstream:
 *     the inherited, lazily-cached Fl_Group::sizes_ array) lives as
 *     file-static state in Fl_Tile.c instead of a struct field or a
 *     reuse of Fl_Group's own `sizes` cache (which cfltk already
 *     gives a different, simpler layout for Fl_Group_resize()'s own
 *     use -- see Fl_Group.h's known differences -- incompatible with
 *     the exact 4-ints-per-child-plus-resizable-block layout Fl_Tile's
 *     algorithm needs). It is recomputed fresh at the start of every
 *     drag gesture (FL_PUSH) rather than lazily cached until an
 *     explicit init_sizes() invalidation, which sidesteps a real
 *     upstream cache-invalidation hazard: forget to call init_sizes()
 *     after changing children post-construction and upstream's cached
 *     layout goes stale. Only one Fl_Tile can be mid-drag at a time
 *     process-wide, matching upstream's own use of function-static
 *     drag-state variables in Fl_Tile::handle().
 *   - No custom mouse cursor shapes while hovering a draggable border
 *     (upstream's set_cursor()/window()->cursor() calls) -- cfltk's
 *     Fl_Window has no cursor-shape API yet, consistent with the same
 *     note in Fl_Text_Display.h. Dragging itself works identically;
 *     only the visual hover-cursor hint is missing.
 */
#ifndef CFLTK_FL_TILE_H
#define CFLTK_FL_TILE_H

#include "cfltk/Fl_Group.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Fl_Tile {
    Fl_Group group;
} Fl_Tile;

extern const Fl_WidgetOps fl_tile_ops;

void Fl_Tile_init(Fl_Tile *self, int x, int y, int w, int h, const char *label);
Fl_Tile *Fl_Tile_new(int x, int y, int w, int h, const char *label);

/* Drags the intersection at (oldx,oldy) to (newx,newy), resizing
 * whichever children shared that border. Pass 0 for oldx or oldy to
 * disable dragging in that direction. Exposed publicly (matching
 * upstream) so callers can programmatically move a border. */
void Fl_Tile_position(Fl_Tile *self, int oldx, int oldy, int newx, int newy);

#ifdef __cplusplus
}
#endif

#endif
