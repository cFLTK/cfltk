/*
 * cfltk - fl_filename.h
 *
 * New infrastructure (not a 1:1 header translation): the upstream
 * FL/filename.H utilities, spread across several small upstream files
 * (filename_list.cxx, numericsort.c, filename_match.cxx,
 * filename_isdir.cxx, filename_ext.cxx, filename_setext.cxx,
 * filename_expand.cxx, filename_absolute.cxx, plus fl_filename_name()
 * from Fl_x.cxx) -- collected here as one small module since none of
 * them are large enough or independently useful enough to warrant
 * their own header/class-style file each.
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
 *   - fl_filename_expand()/_absolute()/_relative() are POSIX-only: all
 *     of upstream's WIN32/__EMX__/__APPLE__-specific branches (`\`
 *     path separators, drive letters, case-insensitive comparison) are
 *     dropped, matching this project's Linux/X11-only scope elsewhere.
 *   - No fl_open_uri()/fl_decode_uri() (shells out to a platform URI
 *     handler, e.g. xdg-open) -- out of scope for this pass; revisit
 *     if/when Dillo needs "open externally" support.
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

/* Pointer to the char after the last '/' in filename, or filename
 * itself if there is none; NULL if filename is NULL. Note this can
 * point at a trailing '\0' (e.g. fl_filename_name("/usr/") == ""). */
const char *fl_filename_name(const char *filename);

/* Pointer to the last '.' in buf (including the leading '.'), or to
 * buf's trailing '\0' if there is no extension (matches upstream's
 * actual behavior, not its docstring, which incorrectly claims NULL). */
const char *fl_filename_ext(const char *buf);

/* Replaces to's extension (as fl_filename_ext() would find it) with
 * ext, or appends ext if to has none. ext may be NULL (treated as
 * ""). Returns `to`. */
char *fl_filename_setext(char *to, int tolen, const char *ext);

/* Expands leading "~"/"~user"/"$VARNAME" components of `from` into
 * `to` (tolen bytes). Returns non-zero if any substitution was made.
 * to and from may be the same buffer. */
int fl_filename_expand(char *to, int tolen, const char *from);

/* Makes `from` absolute (prepends the current working directory,
 * collapsing "." and ".." components) into `to` (tolen bytes).
 * Returns non-zero if any change was made (0 if `from` was already
 * absolute, in which case it's copied through unchanged). */
int fl_filename_absolute(char *to, int tolen, const char *from);

/* Makes absolute path `from` relative to absolute path `base` (both
 * assumed to already be absolute -- neither is expanded/resolved
 * further), writing the result into `to` (tolen bytes). Returns
 * non-zero if any change was made. */
int fl_filename_relative_to(char *to, int tolen, const char *from, const char *base);
/* Same, using the current working directory as the base. */
int fl_filename_relative(char *to, int tolen, const char *from);

#ifdef __cplusplus
}
#endif

#endif
