/*
 * cfltk - Fl_Preferences.h
 *
 * C translation of FLTK 1.3 FL/Fl_Preferences.H / src/Fl_Preferences.cxx.
 *
 * Original class : Fl_Preferences (not a widget -- a persistent,
 *                   hierarchical name/value settings store backed by a
 *                   text file, plus its two private helper classes
 *                   Node (an in-memory tree of groups+entries) and
 *                   RootNode (owns the file path and read()/write())).
 * New C structure : struct Fl_Preferences { Fl_Preferences_Node *node;
 *                    Fl_Preferences_RootNode *root_node; } -- a cheap
 *                    "view" into a shared tree, same as upstream: many
 *                    Fl_Preferences values (one per group you navigate
 *                    into) can point at nodes within the same tree,
 *                    and only destroying the *root* one (whose node
 *                    has no parent) actually flushes-and-frees the
 *                    whole tree, matching upstream's own
 *                    ~Fl_Preferences() exactly.
 * Ownership       : Fl_Preferences_init_group() does NOT give you a
 *                    separately destroyable object -- do not call
 *                    Fl_Preferences_destroy() on it; only the root
 *                    Fl_Preferences (from _init_root()/_init_path())
 *                    owns the tree. Destroying the root flushes first
 *                    if anything changed (matches flush()'s own dirty
 *                    check).
 * Known differences:
 *   - POSIX-only paths: USER root is "$HOME/.fltk/<vendor>/<app>.prefs",
 *     SYSTEM root is "/etc/fltk/<vendor>/<app>.prefs" -- upstream's
 *     WIN32 (registry-derived AppData) and __APPLE__ (Library/
 *     Preferences) branches are dropped, matching this project's
 *     Linux/X11-only scope elsewhere.
 *   - No child-index cache (upstream's Node::index_/createIndex()):
 *     group()/groupIndex-style lookups are a plain O(n) walk of the
 *     child linked-list instead of an amortized-O(1) cached array --
 *     correctness-identical, just without the optimization; cfltk has
 *     no client with a preferences tree large enough for this to
 *     matter.
 *   - No Fl_Preferences(ID) reconstruction, no groupIndex-based
 *     constructor (Fl_Preferences(parent,int)), no NULL-parent
 *     "runtime/plugin database" constructor, no copy constructor, no
 *     newUUID() -- all niche upstream features with no known cfltk
 *     client; add if one shows up.
 *   - No getUserdataPath() (creates a sibling directory next to the
 *     .prefs file for large binary blobs) -- straightforward to add
 *     later if needed, omitted for now since nothing calls it yet.
 */
#ifndef CFLTK_FL_PREFERENCES_H
#define CFLTK_FL_PREFERENCES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FL_PREFERENCES_SYSTEM = 0,
    FL_PREFERENCES_USER = 1
} Fl_Preferences_Root;

typedef struct Fl_Preferences_Node Fl_Preferences_Node;
typedef struct Fl_Preferences_RootNode Fl_Preferences_RootNode;

typedef struct Fl_Preferences {
    Fl_Preferences_Node *node;
    Fl_Preferences_RootNode *root_node;
} Fl_Preferences;

/* Opens (creating if necessary) "$HOME/.fltk/<vendor>/<application>.prefs"
 * (FL_PREFERENCES_USER) or "/etc/fltk/<vendor>/<application>.prefs"
 * (FL_PREFERENCES_SYSTEM), reading it if it already exists. */
void Fl_Preferences_init_root(Fl_Preferences *self, Fl_Preferences_Root root, const char *vendor, const char *application);
/* Opens "<path>/<application>.prefs" (or just <path> verbatim if
 * application is NULL). */
void Fl_Preferences_init_path(Fl_Preferences *self, const char *path, const char *vendor, const char *application);
/* Opens (creating if necessary) the child group `group` of `parent`
 * (which may itself be a child group -- group may contain '/'s to
 * reach further down the hierarchy in one call). Do not destroy the
 * result independently of the root it came from -- see "Ownership"
 * above. */
void Fl_Preferences_init_group(Fl_Preferences *self, Fl_Preferences *parent, const char *group);
/* Only meaningful (and only actually frees/flushes anything) for a
 * root Fl_Preferences (see "Ownership" above); a no-op for a child
 * group view. */
void Fl_Preferences_destroy(Fl_Preferences *self);

const char *Fl_Preferences_name(Fl_Preferences *self);
const char *Fl_Preferences_path(Fl_Preferences *self);

int Fl_Preferences_groups(Fl_Preferences *self);
const char *Fl_Preferences_group(Fl_Preferences *self, int num_group);
/* `key` may be a '/'-separated path; "." is the current node, "./" the
 * topmost node, and a leading "./" makes the rest relative to the
 * topmost node -- matches upstream's Node::search() exactly. */
int Fl_Preferences_group_exists(Fl_Preferences *self, const char *key);
int Fl_Preferences_delete_group(Fl_Preferences *self, const char *group);
int Fl_Preferences_delete_all_groups(Fl_Preferences *self);

int Fl_Preferences_entries(Fl_Preferences *self);
const char *Fl_Preferences_entry(Fl_Preferences *self, int index);
int Fl_Preferences_entry_exists(Fl_Preferences *self, const char *key);
int Fl_Preferences_delete_entry(Fl_Preferences *self, const char *key);
int Fl_Preferences_delete_all_entries(Fl_Preferences *self);

/* Deletes all groups and all entries in this group. */
int Fl_Preferences_clear(Fl_Preferences *self);

int Fl_Preferences_set_int(Fl_Preferences *self, const char *key, int value);
int Fl_Preferences_set_float(Fl_Preferences *self, const char *key, float value);
/* precision: number of significant digits (printf "%.*g"). */
int Fl_Preferences_set_float_p(Fl_Preferences *self, const char *key, float value, int precision);
int Fl_Preferences_set_double(Fl_Preferences *self, const char *key, double value);
int Fl_Preferences_set_double_p(Fl_Preferences *self, const char *key, double value, int precision);
/* Control characters, '\\', and DEL are escaped on write (matches
 * upstream's Node file-format exactly, so a database written by real
 * FLTK reads correctly here and vice versa). */
int Fl_Preferences_set_string(Fl_Preferences *self, const char *key, const char *value);
/* Stored hex-encoded (2 chars/byte), matching upstream. */
int Fl_Preferences_set_data(Fl_Preferences *self, const char *key, const void *value, int size);

int Fl_Preferences_get_int(Fl_Preferences *self, const char *key, int *value, int default_value);
int Fl_Preferences_get_float(Fl_Preferences *self, const char *key, float *value, float default_value);
int Fl_Preferences_get_double(Fl_Preferences *self, const char *key, double *value, double default_value);
/* Writes at most maxsize-1 bytes plus a trailing NUL into value. */
int Fl_Preferences_get_string(Fl_Preferences *self, const char *key, char *value, const char *default_value, int maxsize);
/* Writes at most maxsize bytes into value (no trailing NUL implied --
 * this is raw binary data, matching upstream). */
int Fl_Preferences_get_data(Fl_Preferences *self, const char *key, void *value, const void *default_value, int default_size, int maxsize);

/* Length in bytes of the *encoded* value string (matches upstream's
 * own documented-but-slightly-surprising behavior: for set_data(),
 * this is the hex string's length, i.e. 2x the original byte count,
 * not the original byte count itself). */
int Fl_Preferences_size(Fl_Preferences *self, const char *key);

/* Writes the database to disk now if anything changed since it was
 * last loaded/flushed. Called automatically when the root
 * Fl_Preferences is destroyed. */
void Fl_Preferences_flush(Fl_Preferences *self);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_PREFERENCES_H */
