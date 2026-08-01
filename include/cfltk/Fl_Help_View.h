/*
 * cfltk - Fl_Help_View.h
 *
 * C translation of FLTK 1.3 FL/Fl_Help_View.H / src/Fl_Help_View.cxx.
 *
 * Original class : Fl_Help_View : public Fl_Group -- a small HTML 2.0-ish
 *                   mini-renderer with its own tokenizer, word-wrapping
 *                   layout pass (format()) and a second re-tokenizing
 *                   draw pass (draw()), plus a vertical/horizontal
 *                   Fl_Scrollbar pair embedded by value.
 * New C structure : struct Fl_Help_View { Fl_Group group; ...; a small
 *                    fixed-size font-attribute stack (100 entries,
 *                    matching upstream's Fl_Help_Font_Stack capacity)
 *                    inlined as plain arrays instead of a separate
 *                    class; Fl_Scrollbar *scrollbar_, *hscrollbar_ --
 *                    heap-allocated and auto-added as children, same
 *                    reasoning as Fl_Spinner.h's "Ownership" note (NOT
 *                    embedded by value, because Fl_Group_clear()
 *                    unconditionally free()s every child).
 *
 * Known differences (all deliberate scope cuts -- src/Fl_Help_View.cxx
 * is ~3700 lines, by far the largest single item in cfltk's gap-closing
 * pass; porting 100% of it, including its table layout engine and
 * streaming image decoder, would cost more than the rest of this pass
 * combined for a feature (FLTK's *own* built-in help-file viewer) that
 * Dillo -- which brings its own HTML/CSS engine -- has no reason to
 * exercise. What's kept below is a faithful, from-upstream-source port
 * of the parts that matter for real help/about-box content: headings,
 * paragraphs, lists, preformatted text, basic character styling, named
 * colors, links (including in-page #targets), and HTML entities.):
 *   - No <TABLE>/<TR>/<TD>/<TH> layout. These tags (and their text) are
 *     simply not special-cased by format()/draw(), so a table's cell
 *     text flows as ordinary paragraph text with no column alignment --
 *     visusable, not upstream-identical.
 *   - No inline <IMG> support (no Fl_Shared_Image dependency pulled into
 *     this file). IMG tags are silently skipped, same as any other
 *     unrecognized tag.
 *   - No mouse text selection (upstream's copy-on-select machinery,
 *     itself explicitly a set of file-static globals to avoid an ABI
 *     break -- see the .cxx's own comment above Fl_Help_View::selected).
 *     select_all()/clear_selection() are therefore not exposed either.
 *   - No fl_cursor() hinting on link hover (matches the existing
 *     project-wide known difference: cfltk doesn't implement per-widget
 *     cursor changes yet, see Fl_File_Input.h).
 *   - load() does not special-case http:/https:/ftp:/mailto:/news:/ipp:
 *     URLs via fl_open_uri() (cfltk's fl_filename.h doesn't implement
 *     it either -- see its own known-differences note); such a "link"
 *     is attempted as a local file open like any other and produces
 *     upstream's own "Unable to follow the link" error page on failure,
 *     which is a reasonable fallback for the same non-goal (Dillo does
 *     its own networking).
 *   - Fl_Help_Font_Stack's overflow bound (100 entries) is enforced but
 *     not user-configurable (upstream hardcodes it too, via
 *     MAX_FL_HELP_FS_ELTS).
 */
#ifndef CFLTK_FL_HELP_VIEW_H
#define CFLTK_FL_HELP_VIEW_H

#include "cfltk/Fl_Group.h"
#include "cfltk/Fl_Scrollbar.h"
#include "cfltk/fl_filename.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef const char *(Fl_Help_Func)(Fl_Widget *, const char *);

typedef struct Fl_Help_Block {
    const char *start, *end;
    unsigned char border;
    Fl_Color bgcolor;
    int x, y, w, h;
    int line[32];
} Fl_Help_Block;

typedef struct Fl_Help_Link {
    char filename[192];
    char name[32];
    int x, y, w, h;
} Fl_Help_Link;

typedef struct Fl_Help_Target {
    char name[32];
    int y;
} Fl_Help_Target;

typedef struct Fl_Help_Font_Style {
    Fl_Font f;
    Fl_Fontsize s;
    Fl_Color c;
} Fl_Help_Font_Style;

#define CFLTK_HELP_MAX_FONT_STACK 100

typedef struct Fl_Help_View {
    Fl_Group group;

    char title_[1024];
    Fl_Color defcolor_, bgcolor_, textcolor_, linkcolor_;
    Fl_Font textfont_;
    Fl_Fontsize textsize_;
    const char *value_;

    Fl_Help_Font_Style fstack_[CFLTK_HELP_MAX_FONT_STACK];
    size_t nfonts_;

    int nblocks_, ablocks_;
    Fl_Help_Block *blocks_;

    Fl_Help_Func *link_;

    int nlinks_, alinks_;
    Fl_Help_Link *links_;

    int ntargets_, atargets_;
    Fl_Help_Target *targets_;

    char directory_[FL_PATH_MAX];
    char filename_[FL_PATH_MAX];
    int topline_, leftline_, size_, hsize_, scrollbar_size_;

    Fl_Scrollbar *scrollbar_;
    Fl_Scrollbar *hscrollbar_;
} Fl_Help_View;

extern const Fl_WidgetOps fl_help_view_ops;

void Fl_Help_View_init(Fl_Help_View *self, int x, int y, int w, int h, const char *label);
Fl_Help_View *Fl_Help_View_new(int x, int y, int w, int h, const char *label);

static inline const char *Fl_Help_View_directory(const Fl_Help_View *self) {
    return self->directory_[0] ? self->directory_ : NULL;
}
static inline const char *Fl_Help_View_filename(const Fl_Help_View *self) {
    return self->filename_[0] ? self->filename_ : NULL;
}

/* Finds `s` starting at buffer offset `p` (0 = from the start); scrolls
 * to it and returns the offset just past the match, or -1 if not found. */
int Fl_Help_View_find(Fl_Help_View *self, const char *s, int p);

/* Link-rewrite hook: called on load() and on following a link; must
 * return a path openable as a local file, or NULL to leave value()
 * unchanged. See Fl_Help_Func above. */
static inline void Fl_Help_View_set_link(Fl_Help_View *self, Fl_Help_Func *fn) { self->link_ = fn; }

int Fl_Help_View_load(Fl_Help_View *self, const char *f);

static inline int Fl_Help_View_size(const Fl_Help_View *self) { return self->size_; }

static inline Fl_Color Fl_Help_View_textcolor(const Fl_Help_View *self) { return self->defcolor_; }
void Fl_Help_View_set_textcolor(Fl_Help_View *self, Fl_Color c);

static inline Fl_Font Fl_Help_View_textfont(const Fl_Help_View *self) { return self->textfont_; }
void Fl_Help_View_set_textfont(Fl_Help_View *self, Fl_Font f);

static inline Fl_Fontsize Fl_Help_View_textsize(const Fl_Help_View *self) { return self->textsize_; }
void Fl_Help_View_set_textsize(Fl_Help_View *self, Fl_Fontsize s);

static inline const char *Fl_Help_View_title(const Fl_Help_View *self) { return self->title_; }

void Fl_Help_View_set_topline(Fl_Help_View *self, int top);
void Fl_Help_View_set_topline_target(Fl_Help_View *self, const char *target_name);
static inline int Fl_Help_View_topline(const Fl_Help_View *self) { return self->topline_; }

void Fl_Help_View_set_leftline(Fl_Help_View *self, int left);
static inline int Fl_Help_View_leftline(const Fl_Help_View *self) { return self->leftline_; }

void Fl_Help_View_set_value(Fl_Help_View *self, const char *val);
static inline const char *Fl_Help_View_value(const Fl_Help_View *self) { return self->value_; }

static inline int Fl_Help_View_scrollbar_size(const Fl_Help_View *self) { return self->scrollbar_size_; }
static inline void Fl_Help_View_set_scrollbar_size(Fl_Help_View *self, int new_size) { self->scrollbar_size_ = new_size; }

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_HELP_VIEW_H */
