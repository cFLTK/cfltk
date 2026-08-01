/*
 * cfltk - fl_filename.h
 *
 * New infrastructure (not a 1:1 header translation): the small set of
 * upstream FL/filename.H directory-listing/pattern-matching utilities
 * Fl_File_Browser needs (fl_filename_list/fl_numericsort/etc., spread
 * across several small upstream .cxx files -- filename_list.cxx,
 * numericsort.c, filename_match.cxx, filename_isdir.cxx -- collected
 * here as one small module since none of them are large enough or
 * independently useful enough to warrant their own header/class-style
 * file each).
 *
 * Known differences:
 *   - No locale-to-UTF-8 filename re-encoding (upstream's
 *     fl_utf8from_mb()/fl_utf8to_mb() calls in fl_filename_list()).
 *     Filenames are passed through exactly as the OS returns them,
 *     which is correct as-is on any modern Linux system (UTF-8
 *     locale is the standard default) and is consistent with cfltk's
 *     existing UTF-8 scope (see fl_utf8.h): byte-level correctness,
 *     not encoding conversion.
 *   - No Windows/OS2/AIX/NetBSD-specific mount-point enumeration for
 *     Fl_File_Browser's "list all drives/filesystems" mode (an empty
 *     directory string) -- only the plain Linux/etc. `/etc/mnttab`-or-
 *     `/etc/mtab`-or-`/etc/fstab` fallback chain is ported, since that
 *     is the only branch upstream itself takes on this project's
 *     target platforms.
 */
#ifndef CFLTK_FL_FILENAME_H
#define CFLTK_FL_FILENAME_H

#include <dirent.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FL_PATH_MAX 4096

typedef int(Fl_File_Sort_F)(struct dirent **, struct dirent **);

/* Non-zero if `n` exists and is a directory. */
int fl_filename_isdir(const char *n);
/* Same, but skips the stat() call when `n` already ends in '/'. */
int fl_filename_isdir_quick(const char *n);

/* Lists directory `d` into a freshly malloc'd `*list` array (free with
 * fl_filename_free_list()), appending '/' to entries that are
 * themselves directories, sorted via `sort`. Returns the entry count,
 * or a negative value on error (matching scandir()'s own return
 * convention, which this wraps directly). */
int fl_filename_list(const char *d, struct dirent ***list, Fl_File_Sort_F *sort);
void fl_filename_free_list(struct dirent ***list, int n);

int fl_alphasort(struct dirent **a, struct dirent **b);
int fl_casealphasort(struct dirent **a, struct dirent **b);
int fl_numericsort(struct dirent **a, struct dirent **b);
int fl_casenumericsort(struct dirent **a, struct dirent **b);

/* Shell-glob-style matcher: '*', '?', '[set]'/'[^set]', '{a|b|c}',
 * '\x' to quote. Case-insensitive (matches upstream exactly -- this
 * is not a bug, upstream's fl_filename_match() really does
 * tolower() every character comparison). */
int fl_filename_match(const char *s, const char *p);

#ifdef __cplusplus
}
#endif

#endif
