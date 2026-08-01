/*
 * cfltk - fl_filename.c
 * See include/cfltk/fl_filename.h for scope notes.
 * Translated from src/filename_list.cxx, src/numericsort.c,
 * src/filename_match.cxx, src/filename_isdir.cxx.
 */
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE /* scandir()/S_IFDIR under strict -std=c99 */
#endif

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <dirent.h>

#include "cfltk/fl_filename.h"

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
