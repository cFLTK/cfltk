/*
 * cfltk - Fl_File_Browser.h
 *
 * C translation of FLTK 1.3 FL/Fl_File_Browser.H.
 *
 * Original class : Fl_File_Browser : public Fl_Browser -- lists
 *                   directory contents (or, given an empty directory
 *                   string, mounted filesystems) as browser lines,
 *                   with directories shown in bold and sorted first,
 *                   optionally filtered by a shell-glob filename
 *                   pattern. Reuses essentially all of Fl_Browser's
 *                   line-storage machinery (add()/insert()/clear()/
 *                   FL_BLINE), overriding only item_height()/
 *                   item_width()/item_draw()/full_height().
 * New C structure : struct Fl_File_Browser { Fl_Browser browser;
 *                    int filetype_; const char *directory_; uchar
 *                    iconsize_; const char *pattern_; }, embedding
 *                    Fl_Browser as its first member (which itself
 *                    embeds Fl_Browser_, which embeds Fl_Group...).
 *                    A per-line Fl_File_Icon* pointer would normally
 *                    ride in FL_BLINE's existing generic `void *data`
 *                    field (exactly like upstream stores it there) --
 *                    see Known differences for why nothing is ever
 *                    stored there in this translation.
 * Vtbl            : reuses fl_browser_ops verbatim (no widget-level
 *                    draw()/handle()/resize()/destroy() override --
 *                    Fl_Browser_destroy() already correctly frees
 *                    every FL_BLINE through the shared struct prefix,
 *                    exactly as it does for a plain Fl_Browser). Item
 *                    behavior uses its own fl_file_browser_item_ops,
 *                    built once at first use by copying
 *                    fl_browser_item_ops and substituting
 *                    item_height/item_width/item_draw/full_height
 *                    (see Fl_File_Browser.c -- a plain aggregate copy
 *                    of an extern const struct isn't a C constant
 *                    expression, so this happens at runtime rather
 *                    than as a static initializer).
 * Known differences:
 *   - No Fl_File_Icon (the vector-icon-per-MIME-pattern system) --
 *     entries are listed with text only, no icon column. This is a
 *     graceful degradation upstream itself already supports: real
 *     upstream Fl_File_Browser checks `Fl_File_Icon::first() == NULL`
 *     and falls back to text-only rendering with no icon-space
 *     reservation whenever no icons have been registered, which is
 *     unconditionally the case here. Consistent with the existing
 *     "Fl_Browser has no icon support" note in docs/DESIGN.md.
 *   - load()'s "list all mounted filesystems" mode (an empty
 *     directory string) only ports the plain Linux/etc. `/etc/mnttab`-
 *     or-`/etc/mtab`-or-`/etc/fstab` fallback chain, not the Windows/
 *     OS2/macOS/AIX/NetBSD-specific branches, matching
 *     fl_filename.h's own scope note.
 */
#ifndef CFLTK_FL_FILE_BROWSER_H
#define CFLTK_FL_FILE_BROWSER_H

#include "cfltk/Fl_Browser.h"
#include "cfltk/fl_filename.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FL_FILE_BROWSER_FILES 0
#define FL_FILE_BROWSER_DIRECTORIES 1

typedef struct Fl_File_Browser {
    Fl_Browser browser;
    int filetype_;
    const char *directory_;
    uchar iconsize_;
    const char *pattern_;
} Fl_File_Browser;

void Fl_File_Browser_init(Fl_File_Browser *self, int x, int y, int w, int h, const char *label);
Fl_File_Browser *Fl_File_Browser_new(int x, int y, int w, int h, const char *label);

static inline uchar Fl_File_Browser_iconsize(const Fl_File_Browser *self) { return self->iconsize_; }
void Fl_File_Browser_set_iconsize(Fl_File_Browser *self, uchar s);

void Fl_File_Browser_set_filter(Fl_File_Browser *self, const char *pattern);
static inline const char *Fl_File_Browser_filter(const Fl_File_Browser *self) { return self->pattern_; }

static inline int Fl_File_Browser_filetype(const Fl_File_Browser *self) { return self->filetype_; }
static inline void Fl_File_Browser_set_filetype(Fl_File_Browser *self, int t) { self->filetype_ = t; }

/* Loads `directory` into the browser (clearing whatever was there),
 * returning the number of files found (matching upstream's
 * documented, slightly odd, return value -- it is not strictly the
 * number of lines added, see Fl_File_Browser.c). Pass "" to list
 * mounted filesystems instead (see Known differences). `sort`
 * defaults to fl_numericsort when NULL. */
int Fl_File_Browser_load(Fl_File_Browser *self, const char *directory, Fl_File_Sort_F *sort);

#ifdef __cplusplus
}
#endif

#endif
