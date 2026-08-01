/*
 * cfltk - Fl_Preferences.c
 * See include/cfltk/Fl_Preferences.h for the class-conversion notes.
 * Translated from src/Fl_Preferences.cxx.
 */
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE /* strdup() under strict -std=c99 */
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cfltk/Fl_Preferences.h"

/* -------------------------------------------------------------------
 * Node: an in-memory tree of groups (child_/next_ linked list) each
 * holding a growable array of name/value entries.
 * ---------------------------------------------------------------- */

typedef struct Fl_Preferences_Entry {
    char *name;
    char *value; /* NULL means "annotation line" (comment), not a real entry */
} Fl_Preferences_Entry;

struct Fl_Preferences_Node {
    Fl_Preferences_Node *child, *next, *parent;
    char *path; /* full path from the topmost node, e.g. "./Sub/Group" */
    Fl_Preferences_Entry *entry;
    int n_entry, cap_entry;
    int dirty;
};

/* Matches upstream's Node::lastEntrySet: genuinely a class-static in
 * the original (not per-node) -- during sequential file parsing, a
 * '+'-continuation line always immediately follows the entry it
 * extends, so one process-wide "last entry written" slot is enough
 * (and is exactly what upstream itself relies on). */
static Fl_Preferences_Node *g_last_entry_node = NULL;
static int g_last_entry_index = -1;

static Fl_Preferences_Node *node_new(const char *path) {
    Fl_Preferences_Node *n = (Fl_Preferences_Node *)calloc(1, sizeof(Fl_Preferences_Node));
    n->path = path ? strdup(path) : NULL;
    return n;
}

static void node_free_entries(Fl_Preferences_Node *n) {
    int i;
    for (i = 0; i < n->n_entry; i++) {
        free(n->entry[i].name);
        free(n->entry[i].value);
    }
    free(n->entry);
    n->entry = NULL;
    n->n_entry = n->cap_entry = 0;
}

static void node_free(Fl_Preferences_Node *n) {
    Fl_Preferences_Node *c, *cx;
    if (!n) return;
    for (c = n->child; c; c = cx) {
        cx = c->next;
        node_free(c);
    }
    node_free_entries(n);
    if (g_last_entry_node == n) { g_last_entry_node = NULL; g_last_entry_index = -1; }
    free(n->path);
    free(n);
}

static const char *node_name(Fl_Preferences_Node *n) {
    char *r;
    if (!n->path) return NULL;
    r = strrchr(n->path, '/');
    return r ? r + 1 : n->path;
}

static void node_set_parent(Fl_Preferences_Node *n, Fl_Preferences_Node *pn) {
    char buf[1024];
    n->parent = pn;
    n->next = pn->child;
    pn->child = n;
    snprintf(buf, sizeof(buf), "%s/%s", pn->path, n->path);
    free(n->path);
    n->path = strdup(buf);
}

/* Finds (creating any missing intermediate groups) the node at `path`
 * (a full path from the topmost node, e.g. "./A/B"), starting the
 * search at `n`. Matches upstream's Node::find() exactly. */
static Fl_Preferences_Node *node_find(Fl_Preferences_Node *n, const char *path) {
    size_t len = strlen(n->path);
    if (strncmp(path, n->path, len) != 0) return NULL;
    if (path[len] == 0) return n;
    if (path[len] == '/') {
        Fl_Preferences_Node *c;
        const char *s, *e;
        char namebuf[256];
        Fl_Preferences_Node *nd;
        for (c = n->child; c; c = c->next) {
            Fl_Preferences_Node *nn = node_find(c, path);
            if (nn) return nn;
        }
        s = path + len + 1;
        e = strchr(s, '/');
        if (e) snprintf(namebuf, sizeof(namebuf), "%.*s", (int)(e - s), s);
        else snprintf(namebuf, sizeof(namebuf), "%s", s);
        nd = node_new(namebuf);
        node_set_parent(nd, n);
        return node_find(nd, path);
    }
    return NULL;
}

/* Adds (or finds an existing) child group named `path` (may itself
 * contain '/'s) directly under `n`. Matches upstream's
 * Node::addChild(). */
static Fl_Preferences_Node *node_add_child(Fl_Preferences_Node *n, const char *path) {
    char buf[1024];
    Fl_Preferences_Node *nd;
    snprintf(buf, sizeof(buf), "%s/%s", n->path, path);
    nd = node_find(n, buf);
    n->dirty = 1;
    return nd;
}

/* Read-only search for an existing group, honoring the "."/"./"
 * relative-path rules. Matches upstream's Node::search(). Caller must
 * pass offset=0. */
static Fl_Preferences_Node *node_search(Fl_Preferences_Node *n, const char *path, int offset) {
    int len;
    if (offset == 0) {
        if (path[0] == '.') {
            if (path[1] == 0) return n;
            if (path[1] == '/') {
                Fl_Preferences_Node *top = n;
                while (top->parent) top = top->parent;
                if (path[2] == 0) return top;
                return node_search(top, path + 2, 2);
            }
        }
        offset = (int)strlen(n->path) + 1;
    }
    len = (int)strlen(n->path);
    if (len < offset - 1) return NULL;
    len -= offset;
    if (len <= 0 || strncmp(path, n->path + offset, (size_t)len) == 0) {
        if (len > 0 && path[len] == 0) return n;
        if (len <= 0 || path[len] == '/') {
            Fl_Preferences_Node *c;
            for (c = n->child; c; c = c->next) {
                Fl_Preferences_Node *nn = node_search(c, path, offset);
                if (nn) return nn;
            }
        }
    }
    return NULL;
}

static int node_n_children(Fl_Preferences_Node *n) {
    int cnt = 0;
    Fl_Preferences_Node *c;
    for (c = n->child; c; c = c->next) cnt++;
    return cnt;
}

/* nth child in creation order (upstream's own linked list is built by
 * prepending, i.e. stored newest-first, so childNode() walks it
 * reversed to present a stable oldest-first order -- matched here). */
static Fl_Preferences_Node *node_child_at(Fl_Preferences_Node *n, int ix) {
    int total = node_n_children(n);
    int want = total - ix - 1;
    Fl_Preferences_Node *c;
    if (ix < 0 || ix >= total) return NULL;
    for (c = n->child; c; c = c->next) {
        if (want-- == 0) return c;
    }
    return NULL;
}

static int node_get_entry_index(Fl_Preferences_Node *n, const char *name) {
    int i;
    for (i = 0; i < n->n_entry; i++) {
        if (strcmp(name, n->entry[i].name) == 0) return i;
    }
    return -1;
}

static const char *node_get(Fl_Preferences_Node *n, const char *name) {
    int i = node_get_entry_index(n, name);
    return i >= 0 ? n->entry[i].value : NULL;
}

/* Creates or updates entry `name` = `value` (value may be NULL for a
 * comment/annotation line kept only for round-tripping the file).
 * Matches upstream's Node::set(name,value). */
static void node_set_kv(Fl_Preferences_Node *n, const char *name, const char *value) {
    int i = node_get_entry_index(n, name);
    if (i >= 0) {
        if (!value) return;
        if (!n->entry[i].value || strcmp(value, n->entry[i].value) != 0) {
            free(n->entry[i].value);
            n->entry[i].value = strdup(value);
            n->dirty = 1;
        }
        g_last_entry_node = n;
        g_last_entry_index = i;
        return;
    }
    if (n->cap_entry == n->n_entry) {
        n->cap_entry = n->cap_entry ? n->cap_entry * 2 : 10;
        n->entry = (Fl_Preferences_Entry *)realloc(n->entry, (size_t)n->cap_entry * sizeof(Fl_Preferences_Entry));
    }
    n->entry[n->n_entry].name = strdup(name);
    n->entry[n->n_entry].value = value ? strdup(value) : NULL;
    g_last_entry_node = n;
    g_last_entry_index = n->n_entry;
    n->n_entry++;
    n->dirty = 1;
}

/* Parses one non-continuation line from the file ("name:value",
 * "name" alone (empty value), or a ';'/'#' comment). Matches
 * upstream's Node::set(line). */
static void node_set_line(Fl_Preferences_Node *n, const char *line) {
    int dirt = n->dirty;
    if (line[0] == ';' || line[0] == 0 || line[0] == '#') {
        node_set_kv(n, line, NULL);
    } else {
        const char *c = strchr(line, ':');
        if (c) {
            char namebuf[256];
            size_t len = (size_t)(c - line);
            if (len >= sizeof(namebuf)) len = sizeof(namebuf) - 1;
            memcpy(namebuf, line, len);
            namebuf[len] = '\0';
            node_set_kv(n, namebuf, c + 1);
        } else {
            node_set_kv(n, line, "");
        }
    }
    n->dirty = dirt;
}

/* Appends more text to the most-recently-set entry (a '+'-continuation
 * line). Matches upstream's Node::add(line). */
static void node_add_line(const char *line) {
    char *dst;
    size_t a, b;
    if (!g_last_entry_node || g_last_entry_index < 0 || g_last_entry_index >= g_last_entry_node->n_entry) return;
    dst = g_last_entry_node->entry[g_last_entry_index].value;
    a = dst ? strlen(dst) : 0;
    b = strlen(line);
    dst = (char *)realloc(dst, a + b + 1);
    memcpy(dst + a, line, b + 1);
    g_last_entry_node->entry[g_last_entry_index].value = dst;
    g_last_entry_node->dirty = 1;
}

static int node_delete_entry(Fl_Preferences_Node *n, const char *name) {
    int ix = node_get_entry_index(n, name);
    if (ix < 0) return 0;
    free(n->entry[ix].name);
    free(n->entry[ix].value);
    memmove(n->entry + ix, n->entry + ix + 1, (size_t)(n->n_entry - ix - 1) * sizeof(Fl_Preferences_Entry));
    n->n_entry--;
    n->dirty = 1;
    return 1;
}

static void node_delete_all_entries(Fl_Preferences_Node *n) {
    node_free_entries(n);
    n->dirty = 1;
}

static void node_delete_all_children(Fl_Preferences_Node *n) {
    Fl_Preferences_Node *c, *cx;
    for (c = n->child; c; c = cx) {
        cx = c->next;
        node_free(c);
    }
    n->child = NULL;
    n->dirty = 1;
}

/* Detaches `n` from its parent's child list and frees it (and its
 * whole subtree). Matches upstream's Node::remove(). */
static int node_remove(Fl_Preferences_Node *n) {
    int found = 0;
    if (n->parent) {
        Fl_Preferences_Node **link = &n->parent->child;
        while (*link) {
            if (*link == n) { *link = n->next; found = 1; break; }
            link = &(*link)->next;
        }
        n->parent->dirty = 1;
    }
    n->next = NULL;
    node_free(n);
    return found;
}

static int node_dirty(Fl_Preferences_Node *n) {
    Fl_Preferences_Node *c;
    if (n->dirty) return 1;
    if (n->next && node_dirty(n->next)) return 1;
    for (c = n->child; c; c = c->next) if (node_dirty(c)) return 1;
    return 0;
}

/* Writes this node's entries then recurses into siblings-then-children,
 * matching upstream's Node::write() traversal order exactly (so the
 * file's group ordering matches real FLTK's). */
static void node_write(Fl_Preferences_Node *n, FILE *f) {
    int i;
    if (n->next) node_write(n->next, f);
    fprintf(f, "\n[%s]\n\n", n->path);
    for (i = 0; i < n->n_entry; i++) {
        const char *src = n->entry[i].value;
        if (src) {
            size_t cnt;
            fprintf(f, "%s:", n->entry[i].name);
            for (cnt = 0; cnt < 60 && src[cnt]; cnt++) { /* count */ }
            fwrite(src, cnt, 1, f);
            fputc('\n', f);
            src += cnt;
            while (*src) {
                for (cnt = 0; cnt < 80 && src[cnt]; cnt++) { /* count */ }
                fputc('+', f);
                fwrite(src, cnt, 1, f);
                fputc('\n', f);
                src += cnt;
            }
        } else {
            fprintf(f, "%s\n", n->entry[i].name);
        }
    }
    if (n->child) node_write(n->child, f);
    n->dirty = 0;
}

/* -------------------------------------------------------------------
 * RootNode: owns the file path and read()/write().
 * ---------------------------------------------------------------- */

struct Fl_Preferences_RootNode {
    Fl_Preferences_Node *top; /* the "." node this root owns */
    char *filename;           /* NULL = in-memory only, never read/written */
    char *vendor, *application;
};

static void make_path_for_file(const char *filename) {
    char buf[1024];
    char *p;
    snprintf(buf, sizeof(buf), "%s", filename);
    for (p = strchr(buf + 1, '/'); p; p = strchr(p + 1, '/')) {
        *p = '\0';
        mkdir(buf, 0755);
        *p = '/';
    }
}

static void root_node_read(Fl_Preferences_RootNode *r) {
    char buf[1024];
    FILE *f;
    Fl_Preferences_Node *nd = r->top;

    if (!r->filename) return;
    f = fopen(r->filename, "rb");
    if (!f) return;

    if (!fgets(buf, sizeof(buf), f)) { fclose(f); return; }
    if (!fgets(buf, sizeof(buf), f)) { fclose(f); return; }
    if (!fgets(buf, sizeof(buf), f)) { fclose(f); return; }

    for (;;) {
        if (!fgets(buf, sizeof(buf), f)) break;
        if (buf[0] == '[') {
            size_t end = strcspn(buf + 1, "]\n\r");
            buf[end + 1] = '\0';
            nd = node_find(r->top, buf + 1);
        } else if (buf[0] == '+') {
            size_t end = strcspn(buf + 1, "\n\r");
            if (end != 0) {
                buf[end + 1] = '\0';
                node_add_line(buf + 1);
            }
        } else {
            size_t end = strcspn(buf, "\n\r");
            if (end != 0) {
                buf[end] = '\0';
                if (nd) node_set_line(nd, buf);
            }
        }
    }
    fclose(f);
}

static int root_node_write(Fl_Preferences_RootNode *r) {
    FILE *f;
    if (!r->filename) return -1;
    make_path_for_file(r->filename);
    f = fopen(r->filename, "wb");
    if (!f) return -1;
    fprintf(f, "; FLTK preferences file format 1.0\n");
    fprintf(f, "; vendor: %s\n", r->vendor);
    fprintf(f, "; application: %s\n", r->application);
    node_write(r->top, f);
    fclose(f);
    if (strncmp(r->filename, "/etc/fltk/", 10) == 0) {
        char pathbuf[1024];
        char *p;
        snprintf(pathbuf, sizeof(pathbuf), "%s", r->filename);
        p = pathbuf + 9;
        do {
            *p = '\0';
            chmod(pathbuf, 0755);
            *p = '/';
            p = strchr(p + 1, '/');
        } while (p);
        chmod(r->filename, 0644);
    }
    return 0;
}

static Fl_Preferences_RootNode *root_node_new_root(Fl_Preferences_Root root, const char *vendor, const char *application) {
    Fl_Preferences_RootNode *r = (Fl_Preferences_RootNode *)calloc(1, sizeof(Fl_Preferences_RootNode));
    char filename[1024];
    const char *home;

    filename[0] = '\0';
    if (root == FL_PREFERENCES_USER && (home = getenv("HOME")) != NULL) {
        size_t hl;
        snprintf(filename, sizeof(filename), "%s", home);
        hl = strlen(filename);
        if (hl == 0 || filename[hl - 1] != '/') snprintf(filename + hl, sizeof(filename) - hl, "/.fltk/");
        else snprintf(filename + hl, sizeof(filename) - hl, ".fltk/");
    } else {
        snprintf(filename, sizeof(filename), "/etc/fltk/");
    }
    {
        size_t len = strlen(filename);
        snprintf(filename + len, sizeof(filename) - len, "%s/%s.prefs", vendor, application);
    }

    r->filename = strdup(filename);
    r->vendor = strdup(vendor);
    r->application = strdup(application);
    r->top = node_new(".");
    root_node_read(r);
    return r;
}

static Fl_Preferences_RootNode *root_node_new_path(const char *path, const char *vendor, const char *application) {
    Fl_Preferences_RootNode *r = (Fl_Preferences_RootNode *)calloc(1, sizeof(Fl_Preferences_RootNode));
    char filename[1024];

    if (!vendor) vendor = "unknown";
    if (!application) {
        application = "unknown";
        r->filename = strdup(path);
    } else {
        snprintf(filename, sizeof(filename), "%s/%s.prefs", path, application);
        r->filename = strdup(filename);
    }
    r->vendor = strdup(vendor);
    r->application = strdup(application);
    r->top = node_new(".");
    root_node_read(r);
    return r;
}

static void root_node_free(Fl_Preferences_RootNode *r) {
    if (node_dirty(r->top)) root_node_write(r);
    free(r->filename);
    free(r->vendor);
    free(r->application);
    node_free(r->top);
    free(r);
}

/* -------------------------------------------------------------------
 * Text/binary value encoding (Fl_Preferences::set(...,const char*)/
 * decodeText()/decodeHex() etc.)
 * ---------------------------------------------------------------- */

static char *encode_text(const char *text) {
    const char *s = text ? text : "";
    int n = 0, ns = 0;
    char *buffer, *d;
    for (; *s; s++) { n++; if ((unsigned char)*s < 32 || *s == '\\' || (unsigned char)*s == 0x7f) ns += 4; }
    if (!ns) return strdup(text ? text : "");
    buffer = (char *)malloc((size_t)(n + ns + 1));
    d = buffer;
    for (s = text; *s; ) {
        unsigned char c = (unsigned char)*s;
        if (c == '\\') { *d++ = '\\'; *d++ = '\\'; s++; }
        else if (c == '\n') { *d++ = '\\'; *d++ = 'n'; s++; }
        else if (c == '\r') { *d++ = '\\'; *d++ = 'r'; s++; }
        else if (c < 32 || c == 0x7f) { *d++ = '\\'; *d++ = (char)('0' + ((c >> 6) & 3)); *d++ = (char)('0' + ((c >> 3) & 7)); *d++ = (char)('0' + (c & 7)); s++; }
        else { *d++ = *s++; }
    }
    *d = '\0';
    return buffer;
}

static char *decode_text(const char *src) {
    int len = 0;
    const char *s = src;
    char *dst, *d;
    for (; *s; s++, len++) {
        if (*s == '\\') {
            if (isdigit((unsigned char)s[1])) s += 3;
            else s += 1;
        }
    }
    dst = (char *)malloc((size_t)len + 1);
    d = dst;
    for (s = src; *s; s++) {
        char c = *s;
        if (c == '\\') {
            if (s[1] == '\\') { *d++ = c; s++; }
            else if (s[1] == 'n') { *d++ = '\n'; s++; }
            else if (s[1] == 'r') { *d++ = '\r'; s++; }
            else if (isdigit((unsigned char)s[1])) { *d++ = (char)(((s[1] - '0') << 6) + ((s[2] - '0') << 3) + (s[3] - '0')); s += 3; }
            else s++;
        } else {
            *d++ = c;
        }
    }
    *d = '\0';
    return dst;
}

static char *encode_hex(const void *data, int dsize) {
    static const char lu[] = "0123456789abcdef";
    const unsigned char *s = (const unsigned char *)data;
    char *buffer = (char *)malloc((size_t)dsize * 2 + 1), *d = buffer;
    for (; dsize > 0; dsize--) {
        unsigned char v = *s++;
        *d++ = lu[v >> 4];
        *d++ = lu[v & 0xf];
    }
    *d = '\0';
    return buffer;
}

static void *decode_hex(const char *src, int *size) {
    int n = (int)strlen(src) / 2;
    unsigned char *data = (unsigned char *)malloc((size_t)(n > 0 ? n : 1)), *d = data;
    const char *s = src;
    int i;
    *size = n;
    for (i = n; i > 0; i--) {
        int v;
        char x = (char)tolower((unsigned char)*s++);
        v = (x >= 'a') ? x - 'a' + 10 : x - '0';
        v <<= 4;
        x = (char)tolower((unsigned char)*s++);
        v += (x >= 'a') ? x - 'a' + 10 : x - '0';
        *d++ = (unsigned char)v;
    }
    return data;
}

/* -------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------- */

void Fl_Preferences_init_root(Fl_Preferences *self, Fl_Preferences_Root root, const char *vendor, const char *application) {
    self->root_node = root_node_new_root(root, vendor, application);
    self->node = self->root_node->top;
}

void Fl_Preferences_init_path(Fl_Preferences *self, const char *path, const char *vendor, const char *application) {
    self->root_node = root_node_new_path(path, vendor, application);
    self->node = self->root_node->top;
}

void Fl_Preferences_init_group(Fl_Preferences *self, Fl_Preferences *parent, const char *group) {
    self->root_node = parent->root_node;
    self->node = node_add_child(parent->node, group);
}

void Fl_Preferences_destroy(Fl_Preferences *self) {
    if (self->node && !self->node->parent) root_node_free(self->root_node);
    self->node = NULL;
    self->root_node = NULL;
}

const char *Fl_Preferences_name(Fl_Preferences *self) { return node_name(self->node); }
const char *Fl_Preferences_path(Fl_Preferences *self) { return self->node->path; }

int Fl_Preferences_groups(Fl_Preferences *self) { return node_n_children(self->node); }
const char *Fl_Preferences_group(Fl_Preferences *self, int num_group) {
    Fl_Preferences_Node *n = node_child_at(self->node, num_group);
    return n ? node_name(n) : NULL;
}
int Fl_Preferences_group_exists(Fl_Preferences *self, const char *key) { return node_search(self->node, key, 0) != NULL; }
int Fl_Preferences_delete_group(Fl_Preferences *self, const char *group) {
    Fl_Preferences_Node *n = node_search(self->node, group, 0);
    return n ? node_remove(n) : 0;
}
int Fl_Preferences_delete_all_groups(Fl_Preferences *self) { node_delete_all_children(self->node); return 1; }

int Fl_Preferences_entries(Fl_Preferences *self) { return self->node->n_entry; }
const char *Fl_Preferences_entry(Fl_Preferences *self, int index) {
    if (index < 0 || index >= self->node->n_entry) return NULL;
    return self->node->entry[index].name;
}
int Fl_Preferences_entry_exists(Fl_Preferences *self, const char *key) { return node_get_entry_index(self->node, key) >= 0; }
int Fl_Preferences_delete_entry(Fl_Preferences *self, const char *key) { return node_delete_entry(self->node, key); }
int Fl_Preferences_delete_all_entries(Fl_Preferences *self) { node_delete_all_entries(self->node); return 1; }

int Fl_Preferences_clear(Fl_Preferences *self) {
    int r1 = Fl_Preferences_delete_all_groups(self);
    int r2 = Fl_Preferences_delete_all_entries(self);
    return r1 && r2;
}

int Fl_Preferences_set_int(Fl_Preferences *self, const char *key, int value) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", value);
    node_set_kv(self->node, key, buf);
    return 1;
}
int Fl_Preferences_set_float(Fl_Preferences *self, const char *key, float value) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%g", (double)value);
    node_set_kv(self->node, key, buf);
    return 1;
}
int Fl_Preferences_set_float_p(Fl_Preferences *self, const char *key, float value, int precision) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%.*g", precision, (double)value);
    node_set_kv(self->node, key, buf);
    return 1;
}
int Fl_Preferences_set_double(Fl_Preferences *self, const char *key, double value) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%g", value);
    node_set_kv(self->node, key, buf);
    return 1;
}
int Fl_Preferences_set_double_p(Fl_Preferences *self, const char *key, double value, int precision) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%.*g", precision, value);
    node_set_kv(self->node, key, buf);
    return 1;
}
int Fl_Preferences_set_string(Fl_Preferences *self, const char *key, const char *value) {
    char *enc = encode_text(value);
    node_set_kv(self->node, key, enc);
    free(enc);
    return 1;
}
int Fl_Preferences_set_data(Fl_Preferences *self, const char *key, const void *value, int size) {
    char *enc = encode_hex(value, size);
    node_set_kv(self->node, key, enc);
    free(enc);
    return 1;
}

int Fl_Preferences_get_int(Fl_Preferences *self, const char *key, int *value, int default_value) {
    const char *v = node_get(self->node, key);
    *value = v ? atoi(v) : default_value;
    return v != NULL;
}
int Fl_Preferences_get_float(Fl_Preferences *self, const char *key, float *value, float default_value) {
    const char *v = node_get(self->node, key);
    *value = v ? (float)atof(v) : default_value;
    return v != NULL;
}
int Fl_Preferences_get_double(Fl_Preferences *self, const char *key, double *value, double default_value) {
    const char *v = node_get(self->node, key);
    *value = v ? atof(v) : default_value;
    return v != NULL;
}
int Fl_Preferences_get_string(Fl_Preferences *self, const char *key, char *value, const char *default_value, int maxsize) {
    const char *v = node_get(self->node, key);
    if (v && strchr(v, '\\')) {
        char *w = decode_text(v);
        snprintf(value, (size_t)maxsize, "%s", w);
        free(w);
        return 1;
    }
    if (!v) v = default_value;
    if (v) snprintf(value, (size_t)maxsize, "%s", v);
    else if (maxsize > 0) value[0] = '\0';
    return v != default_value;
}
int Fl_Preferences_get_data(Fl_Preferences *self, const char *key, void *value, const void *default_value, int default_size, int maxsize) {
    const char *v = node_get(self->node, key);
    if (v) {
        int dsize;
        void *w = decode_hex(v, &dsize);
        memmove(value, w, (size_t)(dsize > maxsize ? maxsize : dsize));
        free(w);
        return 1;
    }
    if (default_value) memmove(value, default_value, (size_t)(default_size > maxsize ? maxsize : default_size));
    return 0;
}

int Fl_Preferences_size(Fl_Preferences *self, const char *key) {
    const char *v = node_get(self->node, key);
    return v ? (int)strlen(v) : 0;
}

void Fl_Preferences_flush(Fl_Preferences *self) {
    if (self->root_node && node_dirty(self->node)) root_node_write(self->root_node);
}
