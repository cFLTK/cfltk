/*
 * cfltk - fl_filename.c
 * See include/cfltk/fl_filename.h for scope notes.
 * Translated from src/filename_list.cxx, src/numericsort.c,
 * src/filename_match.cxx, src/filename_isdir.cxx, src/filename_ext.cxx,
 * src/filename_setext.cxx, src/filename_expand.cxx,
 * src/filename_absolute.cxx, and fl_filename_name() from src/Fl_x.cxx.
 */
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE /* scandir()/S_IFDIR under strict -std=c99 */
#endif

#include <ctype.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#include "cfltk/fl_filename.h"

#define isdirsep(c) ((c) == '/')

int fl_filename_isdir(const char *n) {
    struct stat s;
    char fn[FL_PATH_MAX];
    size_t length = strlen(n);

    if (length > 1 && length < sizeof(fn) && n[length - 1] == '/') {
        length--;
        memcpy(fn, n, length);
        fn[length] = '\0';
        n = fn;
    }
    return !stat(n, &s) && S_ISDIR(s.st_mode);
}

int fl_filename_isdir_quick(const char *n) {
    size_t len = strlen(n);
    if (len && n[len - 1] == '/') return 1;
    return fl_filename_isdir(n);
}

int fl_alphasort(struct dirent **a, struct dirent **b) { return strcmp((*a)->d_name, (*b)->d_name); }
int fl_casealphasort(struct dirent **a, struct dirent **b) { return strcasecmp((*a)->d_name, (*b)->d_name); }

static int numericsort(struct dirent **A, struct dirent **B, int cs) {
    const char *a = (*A)->d_name;
    const char *b = (*B)->d_name;
    int ret = 0;
    for (;;) {
        if (isdigit(*a & 255) && isdigit(*b & 255)) {
            int diff, magdiff;
            while (*a == '0') a++;
            while (*b == '0') b++;
            while (isdigit(*a & 255) && *a == *b) { a++; b++; }
            diff = (isdigit(*a & 255) && isdigit(*b & 255)) ? *a - *b : 0;
            magdiff = 0;
            while (isdigit(*a & 255)) { magdiff++; a++; }
            while (isdigit(*b & 255)) { magdiff--; b++; }
            if (magdiff) { ret = magdiff; break; }
            if (diff) { ret = diff; break; }
        } else {
            if (cs) { if ((ret = *a - *b)) break; }
            else { if ((ret = tolower(*a & 255) - tolower(*b & 255))) break; }
            if (!*a) break;
            a++; b++;
        }
    }
    if (!ret) return 0;
    return ret < 0 ? -1 : 1;
}

int fl_casenumericsort(struct dirent **a, struct dirent **b) { return numericsort(a, b, 0); }
int fl_numericsort(struct dirent **a, struct dirent **b) { return numericsort(a, b, 1); }

int fl_filename_match(const char *s, const char *p) {
    int matched;

    for (;;) {
        switch (*p++) {
            case '?':
                if (!*s++) return 0;
                break;

            case '*':
                if (!*p) return 1;
                while (!fl_filename_match(s, p)) if (!*s++) return 0;
                return 1;

            case '[': {
                int reverse;
                char last = 0;
                if (!*s) return 0;
                reverse = (*p == '^' || *p == '!');
                if (reverse) p++;
                matched = 0;
                while (*p) {
                    if (*p == '-' && last) {
                        if (*s <= *++p && *s >= last) matched = 1;
                        last = 0;
                    } else {
                        if (*s == *p) matched = 1;
                    }
                    last = *p++;
                    if (*p == ']') break;
                }
                if (matched == reverse) return 0;
                s++; p++;
                break;
            }

            case '{':
            NEXTCASE:
                if (fl_filename_match(s, p)) return 1;
                for (matched = 0;;) {
                    switch (*p++) {
                        case '\\': if (*p) p++; break;
                        case '{': matched++; break;
                        case '}': if (!matched--) return 0; break;
                        case '|': case ',': if (matched == 0) goto NEXTCASE; break;
                        case 0: return 0;
                        default: break;
                    }
                }
                /* unreachable */

            case '|':
            case ',':
                for (matched = 0; *p && matched >= 0;) {
                    switch (*p++) {
                        case '\\': if (*p) p++; break;
                        case '{': matched++; break;
                        case '}': matched--; break;
                        default: break;
                    }
                }
                break;

            case '}':
                break;

            case 0:
                return !*s;

            case '\\':
                if (*p) p++;
                /* fall through */
            default:
                if (tolower((unsigned char)*s) != tolower((unsigned char)*(p - 1))) return 0;
                s++;
                break;
        }
    }
}

int fl_filename_list(const char *d, struct dirent ***list, Fl_File_Sort_F *sort) {
    int n, i;
    size_t dirlen;
    char fullname[FL_PATH_MAX];

    n = scandir(d, list, NULL, (int (*)(const struct dirent **, const struct dirent **))sort);
    if (n < 0) return n;

    dirlen = strlen(d);
    if (dirlen >= sizeof(fullname) - 2) return n;

    /* Append '/' to entries that are themselves directories. d_name is
     * a fixed-size array inside each dirent scandir() allocated (sized
     * to fit the name it was given, not room to spare), so appending
     * in place would risk overflowing that allocation -- instead,
     * like upstream, allocate a fresh dirent-sized-for-the-longer-name
     * block, copy the fixed header portion plus the extended name into
     * it, and swap it in. */
    for (i = 0; i < n; i++) {
        struct dirent *de = (*list)[i];
        size_t len = strlen(de->d_name);
        size_t header_len = (size_t)((char *)de->d_name - (char *)de);
        struct dirent *newde;
        char *np;

        if (len == 0 || de->d_name[len - 1] == '/' || dirlen + len + 2 >= sizeof(fullname)) continue;

        memcpy(fullname, d, dirlen);
        np = fullname + dirlen;
        if (np != fullname && np[-1] != '/') *np++ = '/';
        memcpy(np, de->d_name, len + 1);

        if (!fl_filename_isdir(fullname)) continue;

        newde = (struct dirent *)malloc(header_len + len + 2);
        memcpy(newde, de, header_len);
        memcpy((char *)newde + header_len, de->d_name, len);
        ((char *)newde)[header_len + len] = '/';
        ((char *)newde)[header_len + len + 1] = '\0';

        free(de);
        (*list)[i] = newde;
    }

    return n;
}

void fl_filename_free_list(struct dirent ***list, int n) {
    int i;
    if (n < 0) return;
    for (i = 0; i < n; i++) free((*list)[i]);
    free(*list);
    *list = NULL;
}

/* -------------------------------------------------------------------
 * Path component/extension helpers
 * ---------------------------------------------------------------- */

const char *fl_filename_name(const char *name) {
    const char *p, *q;
    if (!name) return NULL;
    for (p = q = name; *p; ) {
        if (*p++ == '/') q = p;
    }
    return q;
}

const char *fl_filename_ext(const char *buf) {
    const char *q = NULL;
    const char *p;
    for (p = buf; *p; p++) {
        if (*p == '/') q = NULL;
        else if (*p == '.') q = p;
    }
    return q ? q : p;
}

char *fl_filename_setext(char *to, int tolen, const char *ext) {
    char *q = (char *)fl_filename_ext(to);
    int qoff = (int)(q - to);
    if (ext) snprintf(q, (size_t)(tolen - qoff), "%s", ext);
    else *q = '\0';
    return to;
}

/* -------------------------------------------------------------------
 * Expansion / absolute / relative
 * ---------------------------------------------------------------- */

int fl_filename_expand(char *to, int tolen, const char *from) {
    char *temp = (char *)malloc((size_t)tolen);
    char *start = temp;
    char *end;
    char *a;
    int ret = 0;

    snprintf(temp, (size_t)tolen, "%s", from);
    end = temp + strlen(temp);

    for (a = temp; a < end; ) {
        char *e;
        const char *value = NULL;
        char t;
        int len;

        for (e = a; e < end && !isdirsep(*e); e++) { /* find next slash */ }

        if (*a == '~') {
            if (e <= a + 1) {
                value = getenv("HOME");
            } else {
                struct passwd *pwd;
                t = *e; *e = '\0';
                pwd = getpwnam(a + 1);
                *e = t;
                if (pwd) value = pwd->pw_dir;
            }
        } else if (*a == '$') {
            t = *e; *e = '\0';
            value = getenv(a + 1);
            *e = t;
        }

        if (value) {
            if (isdirsep(value[0])) start = a;
            len = (int)strlen(value);
            if (len > 0 && isdirsep(value[len - 1])) len--;
            if ((end + 1 - e + len) >= tolen) end += tolen - (end + 1 - e + len);
            memmove(a + len, e, (size_t)(end + 1 - e));
            end = a + len + (end - e);
            *end = '\0';
            memcpy(a, value, (size_t)len);
            ret++;
        } else {
            a = e + 1;
        }
    }

    snprintf(to, (size_t)tolen, "%s", start);
    free(temp);
    return ret;
}

int fl_filename_absolute(char *to, int tolen, const char *from) {
    char *temp, *a;
    const char *start = from;

    if (isdirsep(*from) || *from == '|') {
        snprintf(to, (size_t)tolen, "%s", from);
        return 0;
    }

    temp = (char *)malloc((size_t)tolen);
    if (!getcwd(temp, (size_t)tolen)) {
        snprintf(to, (size_t)tolen, "%s", from);
        free(temp);
        return 0;
    }
    a = temp + strlen(temp);
    if (a > temp && isdirsep(*(a - 1))) a--;

    /* collapse leading "." / ".." components of `start` against `a` */
    while (*start == '.') {
        if (start[1] == '.' && isdirsep(start[2])) {
            char *b;
            for (b = a - 1; b >= temp && !isdirsep(*b); b--) { /* empty */ }
            if (b < temp) break;
            a = b;
            start += 3;
        } else if (isdirsep(start[1])) {
            start += 2;
        } else if (!start[1]) {
            start++;
            break;
        } else {
            break;
        }
    }

    *a++ = '/';
    snprintf(a, (size_t)(tolen - (a - temp)), "%s", start);
    snprintf(to, (size_t)tolen, "%s", temp);
    free(temp);
    return 1;
}

int fl_filename_relative_to(char *to, int tolen, const char *from, const char *base) {
    const char *slash;
    char *newslash;
    char *cwd_buf = NULL;
    char *cwd = NULL;

    if (base) cwd = cwd_buf = strdup(base);

    if (from[0] == '\0' || !isdirsep(*from)) {
        snprintf(to, (size_t)tolen, "%s", from);
        free(cwd_buf);
        return 0;
    }
    if (!cwd || cwd[0] == '\0' || !isdirsep(*cwd)) {
        snprintf(to, (size_t)tolen, "%s", from);
        free(cwd_buf);
        return 0;
    }

    if (!strcmp(from, cwd)) {
        snprintf(to, (size_t)tolen, ".");
        free(cwd_buf);
        return 1;
    }

    for (slash = from, newslash = cwd; *slash != '\0' && *newslash != '\0'; slash++, newslash++) {
        if (isdirsep(*slash) && isdirsep(*newslash)) continue;
        if (*slash != *newslash) break;
    }

    if (*newslash == '\0' && *slash != '\0' && !isdirsep(*slash) &&
        (newslash == cwd || !isdirsep(newslash[-1])))
        newslash--;

    while (!isdirsep(*slash) && slash > from) slash--;
    if (isdirsep(*slash)) slash++;

    if (isdirsep(*newslash)) newslash--;
    if (*newslash != '\0') {
        while (!isdirsep(*newslash) && newslash > cwd) newslash--;
    }

    to[0] = '\0';
    to[tolen - 1] = '\0';

    while (*newslash != '\0') {
        if (isdirsep(*newslash)) {
            size_t curlen = strlen(to);
            snprintf(to + curlen, (size_t)tolen - curlen, "../");
        }
        newslash++;
    }
    {
        size_t curlen = strlen(to);
        snprintf(to + curlen, (size_t)tolen - curlen, "%s", slash);
    }

    free(cwd_buf);
    return 1;
}

int fl_filename_relative(char *to, int tolen, const char *from) {
    char cwd_buf[FL_PATH_MAX];
    if (!getcwd(cwd_buf, sizeof(cwd_buf))) {
        snprintf(to, (size_t)tolen, "%s", from);
        return 0;
    }
    return fl_filename_relative_to(to, tolen, from, cwd_buf);
}
