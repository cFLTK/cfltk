/*
 * cfltk - Fl_Help_View.c
 * See include/cfltk/Fl_Help_View.h for the class-conversion notes and
 * the documented scope cuts (no tables, no inline images, no text
 * selection). Translated from src/Fl_Help_View.cxx.
 */
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE /* strdup() under strict -std=c99 */
#endif

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "cfltk/Fl_Help_View.h"
#include "cfltk/Fl.h"
#include "cfltk/fl_draw.h"
#include "cfltk/fl_utf8.h"

/* HV_RIGHT/HV_CENTER/HV_LEFT mirror upstream's `enum { RIGHT = -1, CENTER, LEFT };` */
#define HV_RIGHT  (-1)
#define HV_CENTER 0
#define HV_LEFT   1

/* -------------------------------------------------------------------
 * A fixed-capacity word buffer standing in for upstream's
 * HV_Edit_Buffer (a dynamically-growing class introduced to fix a
 * stack-buffer overflow, STR #3275). A single accumulated "word" or
 * <PRE> line here is bounded by realistic HTML help content; overflow
 * silently truncates rather than reallocating, matching this
 * project's general fixed-buffer-with-bounds-check convention (see
 * e.g. Fl_Preferences.c).
 * ---------------------------------------------------------------- */
#define HV_BUF_CAP 8192
typedef struct HVBuf {
    char data[HV_BUF_CAP];
    int size;
} HVBuf;

static void hv_clear(HVBuf *b) {
    b->size = 0;
    b->data[0] = '\0';
}
static void hv_add_char(HVBuf *b, char c) {
    if (b->size < HV_BUF_CAP - 1) {
        b->data[b->size++] = c;
        b->data[b->size] = '\0';
    }
}
static void hv_add_str(HVBuf *b, const char *s, int n) {
    int i;
    if (n < 0) n = (int)strlen(s);
    for (i = 0; i < n; i++) hv_add_char(b, s[i]);
}
static void hv_add_ucs(HVBuf *b, int ucs) {
    char tmp[6];
    int len = fl_utf8encode((unsigned)ucs, tmp);
    if (len < 1) len = 1;
    hv_add_str(b, tmp, len);
}
static int hv_cmp(const HVBuf *b, const char *s) { return strcasecmp(b->data, s) == 0; }
static double hv_width(const HVBuf *b) { return fl_width(b->data, b->size); }

/* Left-margin stack for format(), mirrors upstream's local `fl_margins`. */
typedef struct HVMargins {
    int depth;
    int stack[100];
} HVMargins;

static int hv_margins_clear(HVMargins *m) {
    m->depth = 0;
    return m->stack[0] = 4;
}
static int hv_margins_pop(HVMargins *m) {
    if (m->depth > 0) {
        m->depth--;
        return m->stack[m->depth];
    }
    return 4;
}
static int hv_margins_push(HVMargins *m, int indent) {
    int xx = m->stack[m->depth] + indent;
    if (m->depth < 99) {
        m->depth++;
        m->stack[m->depth] = xx;
    }
    return xx;
}

/* -------------------------------------------------------------------
 * quote_char(): HTML entity ("&name;"/"&#nn;"/"&#xnn;") -> Unicode
 * code point, or -1 if `p` isn't a recognized entity. Direct port of
 * the upstream table (see FL/Fl_Help_View.H's "Quoted char names").
 * ---------------------------------------------------------------- */
static int quote_char(const char *p) {
    static const struct {
        const char *name;
        int namelen;
        int code;
    } names[] = {
        { "Aacute;", 7, 193 },  { "aacute;", 7, 225 },  { "Acirc;", 6, 194 },
        { "acirc;", 6, 226 },   { "acute;", 6, 180 },   { "AElig;", 6, 198 },
        { "aelig;", 6, 230 },   { "Agrave;", 7, 192 },  { "agrave;", 7, 224 },
        { "amp;", 4, '&' },     { "Aring;", 6, 197 },   { "aring;", 6, 229 },
        { "Atilde;", 7, 195 },  { "atilde;", 7, 227 },  { "Auml;", 5, 196 },
        { "auml;", 5, 228 },    { "brvbar;", 7, 166 },  { "bull;", 5, 0x2022 },
        { "Ccedil;", 7, 199 },  { "ccedil;", 7, 231 },  { "cedil;", 6, 184 },
        { "cent;", 5, 162 },    { "copy;", 5, 169 },    { "curren;", 7, 164 },
        { "deg;", 4, 176 },     { "divide;", 7, 247 },  { "Eacute;", 7, 201 },
        { "eacute;", 7, 233 },  { "Ecirc;", 6, 202 },   { "ecirc;", 6, 234 },
        { "Egrave;", 7, 200 },  { "egrave;", 7, 232 },  { "ETH;", 4, 208 },
        { "eth;", 4, 240 },     { "Euml;", 5, 203 },    { "euml;", 5, 235 },
        { "euro;", 5, 0x20ac }, { "frac12;", 7, 189 },  { "frac14;", 7, 188 },
        { "frac34;", 7, 190 },  { "gt;", 3, '>' },      { "Iacute;", 7, 205 },
        { "iacute;", 7, 237 },  { "Icirc;", 6, 206 },   { "icirc;", 6, 238 },
        { "iexcl;", 6, 161 },   { "Igrave;", 7, 204 },  { "igrave;", 7, 236 },
        { "iquest;", 7, 191 },  { "Iuml;", 5, 207 },    { "iuml;", 5, 239 },
        { "laquo;", 6, 171 },   { "lt;", 3, '<' },      { "macr;", 5, 175 },
        { "micro;", 6, 181 },   { "middot;", 7, 183 },  { "nbsp;", 5, ' ' },
        { "not;", 4, 172 },     { "Ntilde;", 7, 209 },  { "ntilde;", 7, 241 },
        { "Oacute;", 7, 211 },  { "oacute;", 7, 243 },  { "Ocirc;", 6, 212 },
        { "ocirc;", 6, 244 },   { "Ograve;", 7, 210 },  { "ograve;", 7, 242 },
        { "ordf;", 5, 170 },    { "ordm;", 5, 186 },    { "Oslash;", 7, 216 },
        { "oslash;", 7, 248 },  { "Otilde;", 7, 213 },  { "otilde;", 7, 245 },
        { "Ouml;", 5, 214 },    { "ouml;", 5, 246 },    { "para;", 5, 182 },
        { "permil;", 7, 0x2030 }, { "plusmn;", 7, 177 }, { "pound;", 6, 163 },
        { "quot;", 5, '\"' },   { "raquo;", 6, 187 },   { "reg;", 4, 174 },
        { "sect;", 5, 167 },    { "shy;", 4, 173 },     { "sup1;", 5, 185 },
        { "sup2;", 5, 178 },    { "sup3;", 5, 179 },    { "szlig;", 6, 223 },
        { "THORN;", 6, 222 },   { "thorn;", 6, 254 },   { "times;", 6, 215 },
        { "trade;", 6, 0x2122 }, { "Uacute;", 7, 218 }, { "uacute;", 7, 250 },
        { "Ucirc;", 6, 219 },   { "ucirc;", 6, 251 },   { "Ugrave;", 7, 217 },
        { "ugrave;", 7, 249 },  { "uml;", 4, 168 },     { "Uuml;", 5, 220 },
        { "uuml;", 5, 252 },    { "Yacute;", 7, 221 },  { "yacute;", 7, 253 },
        { "yen;", 4, 165 },     { "Yuml;", 5, 0x0178 }, { "yuml;", 5, 255 }
    };
    size_t i;

    if (!strchr(p, ';')) return -1;
    if (*p == '#') {
        if (*(p + 1) == 'x' || *(p + 1) == 'X') return (int)strtol(p + 2, NULL, 16);
        else return atoi(p + 1);
    }
    for (i = 0; i < sizeof(names) / sizeof(names[0]); i++)
        if (strncmp(p, names[i].name, (size_t)names[i].namelen) == 0) return names[i].code;

    return -1;
}

/* -------------------------------------------------------------------
 * get_attr()/get_color()/get_align(): direct ports.
 * ---------------------------------------------------------------- */
static const char *get_attr(const char *p, const char *n, char *buf, int bufsize) {
    char name[255], *ptr, quote;

    buf[0] = '\0';

    while (*p && *p != '>') {
        while (isspace((unsigned char)*p)) p++;
        if (*p == '>' || !*p) return NULL;

        for (ptr = name; *p && !isspace((unsigned char)*p) && *p != '=' && *p != '>';) {
            if (ptr < name + sizeof(name) - 1) *ptr++ = *p++;
            else p++;
        }
        *ptr = '\0';

        if (isspace((unsigned char)*p) || !*p || *p == '>') {
            buf[0] = '\0';
        } else {
            if (*p == '=') p++;

            for (ptr = buf; *p && !isspace((unsigned char)*p) && *p != '>';) {
                if (*p == '\'' || *p == '\"') {
                    quote = *p++;
                    while (*p && *p != quote) {
                        if ((ptr - buf + 1) < bufsize) *ptr++ = *p++;
                        else p++;
                    }
                    if (*p == quote) p++;
                } else if ((ptr - buf + 1) < bufsize) {
                    *ptr++ = *p++;
                } else {
                    p++;
                }
            }
            *ptr = '\0';
        }

        if (strcasecmp(n, name) == 0) return buf;
        else buf[0] = '\0';

        if (*p == '>') return NULL;
    }

    return NULL;
}

static Fl_Color get_color(const char *n, Fl_Color c) {
    static const struct {
        const char *name;
        int r, g, b;
    } colors[] = {
        { "black", 0x00, 0x00, 0x00 },   { "red", 0xff, 0x00, 0x00 },
        { "green", 0x00, 0x80, 0x00 },   { "yellow", 0xff, 0xff, 0x00 },
        { "blue", 0x00, 0x00, 0xff },    { "magenta", 0xff, 0x00, 0xff },
        { "fuchsia", 0xff, 0x00, 0xff }, { "cyan", 0x00, 0xff, 0xff },
        { "aqua", 0x00, 0xff, 0xff },    { "white", 0xff, 0xff, 0xff },
        { "gray", 0x80, 0x80, 0x80 },    { "grey", 0x80, 0x80, 0x80 },
        { "lime", 0x00, 0xff, 0x00 },    { "maroon", 0x80, 0x00, 0x00 },
        { "navy", 0x00, 0x00, 0x80 },    { "olive", 0x80, 0x80, 0x00 },
        { "purple", 0x80, 0x00, 0x80 },  { "silver", 0xc0, 0xc0, 0xc0 },
        { "teal", 0x00, 0x80, 0x80 }
    };
    size_t i;
    long rgb;
    int r, g, b;

    if (!n || !n[0]) return c;

    if (n[0] == '#') {
        rgb = strtol(n + 1, NULL, 16);
        if (strlen(n) > 4) {
            r = (int)(rgb >> 16);
            g = (int)((rgb >> 8) & 255);
            b = (int)(rgb & 255);
        } else {
            r = (int)((rgb >> 8) * 17);
            g = (int)(((rgb >> 4) & 15) * 17);
            b = (int)((rgb & 15) * 17);
        }
        return fl_rgb_color((uchar)r, (uchar)g, (uchar)b);
    }

    for (i = 0; i < sizeof(colors) / sizeof(colors[0]); i++)
        if (strcasecmp(n, colors[i].name) == 0) return fl_rgb_color((uchar)colors[i].r, (uchar)colors[i].g, (uchar)colors[i].b);

    return c;
}

static int get_align(const char *p, int a) {
    char buf[255];
    if (get_attr(p, "ALIGN", buf, sizeof(buf)) == NULL) return a;
    if (strcasecmp(buf, "CENTER") == 0) return HV_CENTER;
    else if (strcasecmp(buf, "RIGHT") == 0) return HV_RIGHT;
    else return HV_LEFT;
}

/* -------------------------------------------------------------------
 * Font stack: initfont()/pushfont()/popfont(), operating on
 * self->fstack_/nfonts_ (upstream: Fl_Help_Font_Stack).
 * ---------------------------------------------------------------- */
static void initfont(Fl_Help_View *self, Fl_Font *f, Fl_Fontsize *s, Fl_Color *c) {
    *f = self->textfont_;
    *s = self->textsize_;
    *c = self->textcolor_;
    self->nfonts_ = 0;
    self->fstack_[0].f = *f;
    self->fstack_[0].s = *s;
    self->fstack_[0].c = *c;
    fl_font(*f, *s);
    fl_color(*c);
}
static void pushfont3(Fl_Help_View *self, Fl_Font f, Fl_Fontsize s, Fl_Color c) {
    if (self->nfonts_ < CFLTK_HELP_MAX_FONT_STACK - 1) self->nfonts_++;
    self->fstack_[self->nfonts_].f = f;
    self->fstack_[self->nfonts_].s = s;
    self->fstack_[self->nfonts_].c = c;
    fl_font(f, s);
    fl_color(c);
}
static void pushfont2(Fl_Help_View *self, Fl_Font f, Fl_Fontsize s) { pushfont3(self, f, s, self->textcolor_); }
static void popfont(Fl_Help_View *self, Fl_Font *f, Fl_Fontsize *s, Fl_Color *c) {
    if (self->nfonts_ > 0) self->nfonts_--;
    *f = self->fstack_[self->nfonts_].f;
    *s = self->fstack_[self->nfonts_].s;
    *c = self->fstack_[self->nfonts_].c;
    fl_font(*f, *s);
    fl_color(*c);
}

/* -------------------------------------------------------------------
 * Block/link/target lists.
 * ---------------------------------------------------------------- */
static Fl_Help_Block *add_block(Fl_Help_View *self, const char *s, int xx, int yy, int ww, int hh, unsigned char border) {
    Fl_Help_Block *temp;
    if (self->nblocks_ >= self->ablocks_) {
        self->ablocks_ += 16;
        self->blocks_ = (Fl_Help_Block *)realloc(self->blocks_, sizeof(Fl_Help_Block) * (size_t)self->ablocks_);
    }
    temp = self->blocks_ + self->nblocks_;
    memset(temp, 0, sizeof(*temp));
    temp->start = s;
    temp->end = s;
    temp->x = xx;
    temp->y = yy;
    temp->w = ww;
    temp->h = hh;
    temp->border = border;
    temp->bgcolor = self->bgcolor_;
    self->nblocks_++;
    return temp;
}

static void add_link(Fl_Help_View *self, const char *n, int xx, int yy, int ww, int hh) {
    Fl_Help_Link *temp;
    char *target;
    if (self->nlinks_ >= self->alinks_) {
        self->alinks_ += 16;
        self->links_ = (Fl_Help_Link *)realloc(self->links_, sizeof(Fl_Help_Link) * (size_t)self->alinks_);
    }
    temp = self->links_ + self->nlinks_;
    temp->x = xx;
    temp->y = yy;
    temp->w = xx + ww;
    temp->h = yy + hh;
    snprintf(temp->filename, sizeof(temp->filename), "%s", n);
    if ((target = strrchr(temp->filename, '#')) != NULL) {
        *target++ = '\0';
        snprintf(temp->name, sizeof(temp->name), "%s", target);
    } else {
        temp->name[0] = '\0';
    }
    self->nlinks_++;
}

static void add_target(Fl_Help_View *self, const char *n, int yy) {
    Fl_Help_Target *temp;
    if (self->ntargets_ >= self->atargets_) {
        self->atargets_ += 16;
        self->targets_ = (Fl_Help_Target *)realloc(self->targets_, sizeof(Fl_Help_Target) * (size_t)self->atargets_);
    }
    temp = self->targets_ + self->ntargets_;
    temp->y = yy;
    snprintf(temp->name, sizeof(temp->name), "%s", n);
    self->ntargets_++;
}

static int compare_targets(const void *a, const void *b) {
    return strcasecmp(((const Fl_Help_Target *)a)->name, ((const Fl_Help_Target *)b)->name);
}

static int do_align(Fl_Help_View *self, Fl_Help_Block *block, int line, int xx, int a, int *l) {
    int offset;
    switch (a) {
        case HV_RIGHT: offset = block->w - xx; break;
        case HV_CENTER: offset = (block->w - xx) / 2; break;
        default: offset = 0; break;
    }
    block->line[line] = block->x + offset;
    if (line < 31) line++;
    while (*l < self->nlinks_) {
        self->links_[*l].x += offset;
        self->links_[*l].w += offset;
        (*l)++;
    }
    return line;
}

static void free_data(Fl_Help_View *self) {
    if (self->value_) {
        free((void *)self->value_);
        self->value_ = NULL;
    }
    if (self->nblocks_) {
        free(self->blocks_);
        self->ablocks_ = 0;
        self->nblocks_ = 0;
        self->blocks_ = NULL;
    }
    if (self->nlinks_) {
        free(self->links_);
        self->alinks_ = 0;
        self->nlinks_ = 0;
        self->links_ = NULL;
    }
    if (self->ntargets_) {
        free(self->targets_);
        self->atargets_ = 0;
        self->ntargets_ = 0;
        self->targets_ = NULL;
    }
}

/* -------------------------------------------------------------------
 * format(): word-wrapping layout pass. Faithful port of
 * Fl_Help_View::format() minus <TABLE>/<TR>/<TD>/<TH> and <IMG>
 * handling -- see the header's "Known differences".
 * ---------------------------------------------------------------- */
static void format(Fl_Help_View *self) {
    Fl_Widget *self_w = &self->group.widget;
    uchar b;
    int done;
    Fl_Help_Block *block;
    const char *ptr, *start, *attrs;
    HVBuf buf;
    char attr[1024];
    char linkdest[1024];
    int xx, yy, ww, hh;
    int line, links;
    Fl_Font font = self->textfont_;
    Fl_Fontsize fsize = self->textsize_;
    Fl_Color fcolor = self->textcolor_;
    int talign, newalign, head, pre, needspace;
    HVMargins margins;
    int scrollsize;

    b = Fl_Widget_box(self_w) ? Fl_Widget_box(self_w) : FL_DOWN_BOX;

    scrollsize = self->scrollbar_size_ ? self->scrollbar_size_ : Fl_scrollbar_size();
    self->hsize_ = self_w->w - scrollsize - fl_box_dw(b);

    done = 0;
    while (!done) {
        done = 1;
        self->nblocks_ = 0;
        self->nlinks_ = 0;
        self->ntargets_ = 0;
        self->size_ = 0;
        self->bgcolor_ = Fl_Widget_color(self_w);
        self->textcolor_ = self->defcolor_;
        self->linkcolor_ = fl_contrast(FL_BLUE, Fl_Widget_color(self_w));

        snprintf(self->title_, sizeof(self->title_), "Untitled");

        if (!self->value_) return;

        initfont(self, &font, &fsize, &fcolor);

        line = 0;
        links = 0;
        xx = hv_margins_clear(&margins);
        yy = fsize + 2;
        ww = 0;
        hh = 0;
        block = add_block(self, self->value_, xx, yy, self->hsize_, 0, 0);
        head = 0;
        pre = 0;
        talign = HV_LEFT;
        newalign = HV_LEFT;
        needspace = 0;
        linkdest[0] = '\0';

        for (ptr = self->value_, hv_clear(&buf); *ptr;) {
            if ((*ptr == '<' || isspace((unsigned char)*ptr)) && buf.size > 0) {
                ww = (int)hv_width(&buf);

                if (!head && !pre) {
                    if (ww > self->hsize_) { self->hsize_ = ww; done = 0; break; }

                    if (needspace && xx > block->x) ww += (int)fl_width(" ", 1);

                    if ((xx + ww) > block->w) {
                        line = do_align(self, block, line, xx, newalign, &links);
                        xx = block->x;
                        yy += hh;
                        block->h += hh;
                        hh = 0;
                    }

                    if (linkdest[0]) add_link(self, linkdest, xx, yy - fsize, ww, fsize);

                    xx += ww;
                    if ((fsize + 2) > hh) hh = fsize + 2;
                    needspace = 0;
                } else if (pre) {
                    if (linkdest[0]) add_link(self, linkdest, xx, yy - hh, ww, hh);
                    xx += ww;
                    if ((fsize + 2) > hh) hh = fsize + 2;

                    while (isspace((unsigned char)*ptr)) {
                        if (*ptr == '\n') {
                            if (xx > self->hsize_) break;
                            line = do_align(self, block, line, xx, newalign, &links);
                            xx = block->x;
                            yy += hh;
                            block->h += hh;
                            hh = fsize + 2;
                        } else {
                            xx += (int)fl_width(" ", 1);
                        }
                        if ((fsize + 2) > hh) hh = fsize + 2;
                        ptr++;
                    }

                    if (xx > self->hsize_) { self->hsize_ = xx; done = 0; break; }
                    needspace = 0;
                } else {
                    while (isspace((unsigned char)*ptr)) ptr++;
                }

                hv_clear(&buf);
            }

            if (*ptr == '<') {
                start = ptr;
                ptr++;

                if (strncmp(ptr, "!--", 3) == 0) {
                    ptr += 3;
                    if ((ptr = strstr(ptr, "-->")) != NULL) { ptr += 3; continue; }
                    else break;
                }

                while (*ptr && *ptr != '>' && !isspace((unsigned char)*ptr)) hv_add_char(&buf, *ptr++);
                attrs = ptr;
                while (*ptr && *ptr != '>') ptr++;
                if (*ptr == '>') ptr++;

                if (hv_cmp(&buf, "HEAD")) {
                    head = 1;
                } else if (hv_cmp(&buf, "/HEAD")) {
                    head = 0;
                } else if (hv_cmp(&buf, "TITLE")) {
                    char *st = self->title_;
                    char *stend = self->title_ + sizeof(self->title_) - 1;
                    while (*ptr != '<' && *ptr && st < stend) *st++ = *ptr++;
                    *st = '\0';
                    hv_clear(&buf);
                } else if (hv_cmp(&buf, "A")) {
                    if (get_attr(attrs, "NAME", attr, sizeof(attr)) != NULL) add_target(self, attr, yy - fsize - 2);
                    if (get_attr(attrs, "HREF", attr, sizeof(attr)) != NULL) snprintf(linkdest, sizeof(linkdest), "%s", attr);
                } else if (hv_cmp(&buf, "/A")) {
                    linkdest[0] = '\0';
                } else if (hv_cmp(&buf, "BODY")) {
                    self->bgcolor_ = get_color(get_attr(attrs, "BGCOLOR", attr, sizeof(attr)), Fl_Widget_color(self_w));
                    self->textcolor_ = get_color(get_attr(attrs, "TEXT", attr, sizeof(attr)), self->defcolor_);
                    self->linkcolor_ = get_color(get_attr(attrs, "LINK", attr, sizeof(attr)), fl_contrast(FL_BLUE, Fl_Widget_color(self_w)));
                } else if (hv_cmp(&buf, "BR")) {
                    line = do_align(self, block, line, xx, newalign, &links);
                    xx = block->x;
                    block->h += hh;
                    yy += hh;
                    hh = 0;
                } else if (hv_cmp(&buf, "CENTER") || hv_cmp(&buf, "P") ||
                           hv_cmp(&buf, "H1") || hv_cmp(&buf, "H2") || hv_cmp(&buf, "H3") ||
                           hv_cmp(&buf, "H4") || hv_cmp(&buf, "H5") || hv_cmp(&buf, "H6") ||
                           hv_cmp(&buf, "UL") || hv_cmp(&buf, "OL") || hv_cmp(&buf, "DL") ||
                           hv_cmp(&buf, "LI") || hv_cmp(&buf, "DD") || hv_cmp(&buf, "DT") ||
                           hv_cmp(&buf, "HR") || hv_cmp(&buf, "PRE")) {
                    block->end = start;
                    line = do_align(self, block, line, xx, newalign, &links);
                    newalign = hv_cmp(&buf, "CENTER") ? HV_CENTER : HV_LEFT;
                    xx = block->x;
                    block->h += hh;

                    if (hv_cmp(&buf, "UL") || hv_cmp(&buf, "OL") || hv_cmp(&buf, "DL")) {
                        block->h += fsize + 2;
                        xx = hv_margins_push(&margins, 4 * fsize);
                    }

                    if (tolower((unsigned char)buf.data[0]) == 'h' && isdigit((unsigned char)buf.data[1])) {
                        font = FL_HELVETICA_BOLD;
                        fsize = self->textsize_ + '7' - buf.data[1];
                    } else if (hv_cmp(&buf, "DT")) {
                        font = self->textfont_ | FL_ITALIC;
                        fsize = self->textsize_;
                    } else if (hv_cmp(&buf, "PRE")) {
                        font = FL_COURIER;
                        fsize = self->textsize_;
                        pre = 1;
                    } else {
                        font = self->textfont_;
                        fsize = self->textsize_;
                    }

                    pushfont2(self, font, fsize);

                    yy = block->y + block->h;
                    hh = 0;

                    if ((tolower((unsigned char)buf.data[0]) == 'h' && isdigit((unsigned char)buf.data[1])) ||
                        hv_cmp(&buf, "DD") || hv_cmp(&buf, "DT") || hv_cmp(&buf, "P")) {
                        yy += fsize + 2;
                    } else if (hv_cmp(&buf, "HR")) {
                        hh += 2 * fsize;
                        yy += fsize;
                    }

                    block = add_block(self, start, xx, yy, self->hsize_, 0, 0);

                    needspace = 0;
                    line = 0;

                    if (hv_cmp(&buf, "CENTER")) newalign = talign = HV_CENTER;
                    else newalign = get_align(attrs, talign);
                } else if (hv_cmp(&buf, "/CENTER") || hv_cmp(&buf, "/P") ||
                           hv_cmp(&buf, "/H1") || hv_cmp(&buf, "/H2") || hv_cmp(&buf, "/H3") ||
                           hv_cmp(&buf, "/H4") || hv_cmp(&buf, "/H5") || hv_cmp(&buf, "/H6") ||
                           hv_cmp(&buf, "/PRE") || hv_cmp(&buf, "/UL") || hv_cmp(&buf, "/OL") ||
                           hv_cmp(&buf, "/DL")) {
                    line = do_align(self, block, line, xx, newalign, &links);
                    xx = block->x;
                    block->end = ptr;

                    if (hv_cmp(&buf, "/UL") || hv_cmp(&buf, "/OL") || hv_cmp(&buf, "/DL")) {
                        xx = hv_margins_pop(&margins);
                        block->h += fsize + 2;
                    } else if (hv_cmp(&buf, "/PRE")) {
                        pre = 0;
                        hh = 0;
                    } else if (hv_cmp(&buf, "/CENTER")) {
                        talign = HV_LEFT;
                    }

                    popfont(self, &font, &fsize, &fcolor);

                    while (isspace((unsigned char)*ptr)) ptr++;

                    block->h += hh;
                    yy += hh;

                    if (tolower((unsigned char)buf.data[2]) == 'l') yy += fsize + 2;

                    block = add_block(self, ptr, xx, yy, self->hsize_, 0, 0);

                    needspace = 0;
                    hh = 0;
                    line = 0;
                    newalign = talign;
                } else if (hv_cmp(&buf, "FONT")) {
                    if (get_attr(attrs, "FACE", attr, sizeof(attr)) != NULL) {
                        if (!strncasecmp(attr, "helvetica", 9) || !strncasecmp(attr, "arial", 5) || !strncasecmp(attr, "sans", 4)) font = FL_HELVETICA;
                        else if (!strncasecmp(attr, "times", 5) || !strncasecmp(attr, "serif", 5)) font = FL_TIMES;
                        else if (!strncasecmp(attr, "symbol", 6)) font = FL_SYMBOL;
                        else font = FL_COURIER;
                    }
                    if (get_attr(attrs, "SIZE", attr, sizeof(attr)) != NULL) {
                        if (isdigit((unsigned char)attr[0])) fsize = (int)(self->textsize_ * pow(1.2, atoi(attr) - 3.0));
                        else fsize = (int)(fsize * pow(1.2, atoi(attr)));
                    }
                    pushfont2(self, font, fsize);
                } else if (hv_cmp(&buf, "/FONT")) {
                    popfont(self, &font, &fsize, &fcolor);
                } else if (hv_cmp(&buf, "B") || hv_cmp(&buf, "STRONG")) {
                    font |= FL_BOLD;
                    pushfont2(self, font, fsize);
                } else if (hv_cmp(&buf, "I") || hv_cmp(&buf, "EM")) {
                    font |= FL_ITALIC;
                    pushfont2(self, font, fsize);
                } else if (hv_cmp(&buf, "CODE") || hv_cmp(&buf, "TT")) {
                    font = FL_COURIER;
                    pushfont2(self, font, fsize);
                } else if (hv_cmp(&buf, "KBD")) {
                    font = FL_COURIER_BOLD;
                    pushfont2(self, font, fsize);
                } else if (hv_cmp(&buf, "VAR")) {
                    font = FL_COURIER_ITALIC;
                    pushfont2(self, font, fsize);
                } else if (hv_cmp(&buf, "/B") || hv_cmp(&buf, "/STRONG") ||
                           hv_cmp(&buf, "/I") || hv_cmp(&buf, "/EM") ||
                           hv_cmp(&buf, "/CODE") || hv_cmp(&buf, "/TT") ||
                           hv_cmp(&buf, "/KBD") || hv_cmp(&buf, "/VAR")) {
                    popfont(self, &font, &fsize, &fcolor);
                }
                hv_clear(&buf);
            } else if (*ptr == '\n' && pre) {
                if (linkdest[0]) add_link(self, linkdest, xx, yy - hh, ww, hh);
                if (xx > self->hsize_) { self->hsize_ = xx; done = 0; break; }
                line = do_align(self, block, line, xx, newalign, &links);
                xx = block->x;
                yy += hh;
                block->h += hh;
                needspace = 0;
                ptr++;
            } else if (isspace((unsigned char)*ptr)) {
                needspace = 1;
                if (pre) xx += (int)fl_width(" ", 1);
                ptr++;
            } else if (*ptr == '&') {
                int qch;
                ptr++;
                qch = quote_char(ptr);
                if (qch < 0) hv_add_char(&buf, '&');
                else {
                    hv_add_ucs(&buf, qch);
                    ptr = strchr(ptr, ';') + 1;
                }
                if ((fsize + 2) > hh) hh = fsize + 2;
            } else {
                hv_add_char(&buf, *ptr++);
                if ((fsize + 2) > hh) hh = fsize + 2;
            }
        }

        if (buf.size > 0 && !head) {
            ww = (int)hv_width(&buf);

            if (ww > self->hsize_) { self->hsize_ = ww; done = 0; break; }

            if (needspace && xx > block->x) ww += (int)fl_width(" ", 1);

            if ((xx + ww) > block->w) {
                line = do_align(self, block, line, xx, newalign, &links);
                xx = block->x;
                yy += hh;
                block->h += hh;
                hh = 0;
            }

            if (linkdest[0]) add_link(self, linkdest, xx, yy - fsize, ww, fsize);

            xx += ww;
        }

        do_align(self, block, line, xx, newalign, &links);

        block->end = ptr;
        self->size_ = yy + hh;
    }

    if (self->ntargets_ > 1) qsort(self->targets_, (size_t)self->ntargets_, sizeof(Fl_Help_Target), compare_targets);

    {
        int dx = fl_box_dw(b) - fl_box_dx(b);
        int dy = fl_box_dh(b) - fl_box_dy(b);
        int ss = self->scrollbar_size_ ? self->scrollbar_size_ : Fl_scrollbar_size();
        int dw = fl_box_dw(b) + ss;
        int dh = fl_box_dh(b);
        Fl_Widget *sb = FL_WIDGET(self->scrollbar_);
        Fl_Widget *hsb = FL_WIDGET(self->hscrollbar_);

        if (self->hsize_ > (self_w->w - dw)) {
            Fl_Widget_show(hsb);
            dh += ss;

            if (self->size_ < (self_w->h - dh)) {
                Fl_Widget_hide(sb);
                Fl_Widget_resize(hsb, self_w->x + fl_box_dx(b), self_w->y + self_w->h - ss - dy, self_w->w - fl_box_dw(b), ss);
            } else {
                Fl_Widget_show(sb);
                Fl_Widget_resize(sb, self_w->x + self_w->w - ss - dx, self_w->y + fl_box_dy(b), ss, self_w->h - ss - fl_box_dh(b));
                Fl_Widget_resize(hsb, self_w->x + fl_box_dx(b), self_w->y + self_w->h - ss - dy, self_w->w - ss - fl_box_dw(b), ss);
            }
        } else {
            Fl_Widget_hide(hsb);

            if (self->size_ < (self_w->h - dh)) {
                Fl_Widget_hide(sb);
            } else {
                Fl_Widget_resize(sb, self_w->x + self_w->w - ss - dx, self_w->y + fl_box_dy(b), ss, self_w->h - fl_box_dh(b));
                Fl_Widget_show(sb);
            }
        }

        Fl_Group_init_sizes(&self->group);

        if (Fl_Widget_visible(sb)) {
            int temph = self_w->h - fl_box_dh(b);
            if (Fl_Widget_visible(hsb)) temph -= ss;
            if ((self->topline_ + temph) > self->size_) Fl_Help_View_set_topline(self, self->size_ - temph);
            else Fl_Help_View_set_topline(self, self->topline_);
        } else {
            Fl_Help_View_set_topline(self, 0);
        }

        if (Fl_Widget_visible(hsb)) {
            int tempw = self_w->w - ss - fl_box_dw(b);
            if ((self->leftline_ + tempw) > self->hsize_) Fl_Help_View_set_leftline(self, self->hsize_ - tempw);
            else Fl_Help_View_set_leftline(self, self->leftline_);
        } else {
            Fl_Help_View_set_leftline(self, 0);
        }
    }
}

/* -------------------------------------------------------------------
 * draw(): re-tokenizes each visible block's [start,end) text run and
 * paints it. Faithful port of Fl_Help_View::draw() minus selection
 * highlighting, <TABLE> cell backgrounds/borders, and <IMG>.
 * ---------------------------------------------------------------- */
static void help_view_draw(Fl_Widget *self_w) {
    Fl_Help_View *self = (Fl_Help_View *)self_w;
    int i;
    const Fl_Help_Block *block;
    const char *ptr, *attrs;
    HVBuf buf;
    char attr[1024];
    int xx, yy, ww, hh;
    int line;
    Fl_Font font = self->textfont_;
    Fl_Fontsize fsize = self->textsize_;
    Fl_Color fcolor = self->textcolor_;
    int head, pre, needspace;
    uchar b = Fl_Widget_box(self_w) ? Fl_Widget_box(self_w) : FL_DOWN_BOX;
    int underline, xtra_ww;

    ww = self_w->w;
    hh = self_w->h;

    fl_draw_box(b, self_w->x, self_w->y, ww, hh, self->bgcolor_);

    {
        Fl_Widget *sb = FL_WIDGET(self->scrollbar_);
        Fl_Widget *hsb = FL_WIDGET(self->hscrollbar_);
        if (Fl_Widget_visible(hsb) || Fl_Widget_visible(sb)) {
            int scrollsize = self->scrollbar_size_ ? self->scrollbar_size_ : Fl_scrollbar_size();
            int hor_vis = Fl_Widget_visible(hsb);
            int ver_vis = Fl_Widget_visible(sb);
            int scorn_x = self_w->x + ww - (ver_vis ? scrollsize : 0) - fl_box_dw(b) + fl_box_dx(b);
            int scorn_y = self_w->y + hh - (hor_vis ? scrollsize : 0) - fl_box_dh(b) + fl_box_dy(b);

            if (hor_vis) {
                if (hsb->h != scrollsize) {
                    Fl_Widget_resize(hsb, self_w->x, scorn_y, scorn_x - self_w->x, scrollsize);
                    Fl_Group_init_sizes(&self->group);
                }
                Fl_Widget_draw(hsb);
                hh -= scrollsize;
            }
            if (ver_vis) {
                if (sb->w != scrollsize) {
                    Fl_Widget_resize(sb, scorn_x, self_w->y, scrollsize, scorn_y - self_w->y);
                    Fl_Group_init_sizes(&self->group);
                }
                Fl_Widget_draw(sb);
                ww -= scrollsize;
            }
            if (hor_vis && ver_vis) {
                fl_color(FL_GRAY);
                fl_rectf(scorn_x, scorn_y, scrollsize, scrollsize);
            }
        }
    }

    if (!self->value_) return;

    fl_push_clip(self_w->x + fl_box_dx(b), self_w->y + fl_box_dy(b), ww - fl_box_dw(b), hh - fl_box_dh(b));
    fl_color(self->textcolor_);

    for (i = 0, block = self->blocks_; i < self->nblocks_; i++, block++) {
        if ((block->y + block->h) >= self->topline_ && block->y < (self->topline_ + self_w->h)) {
            line = 0;
            xx = block->line[line];
            yy = block->y - self->topline_;
            hh = 0;
            pre = 0;
            head = 0;
            needspace = 0;
            underline = 0;

            initfont(self, &font, &fsize, &fcolor);

            for (ptr = block->start, hv_clear(&buf); ptr < block->end;) {
                if ((*ptr == '<' || isspace((unsigned char)*ptr)) && buf.size > 0) {
                    if (!head && !pre) {
                        ww = (int)hv_width(&buf);

                        if (needspace && xx > block->x) xx += (int)fl_width(" ", 1);

                        if ((xx + ww) > block->w) {
                            if (line < 31) line++;
                            xx = block->line[line];
                            yy += hh;
                            hh = 0;
                        }

                        fl_draw(buf.data, xx + self_w->x - self->leftline_, yy + self_w->y);
                        hv_clear(&buf);
                        if (underline) {
                            xtra_ww = isspace((unsigned char)*ptr) ? (int)fl_width(" ", 1) : 0;
                            fl_xyline(xx + self_w->x - self->leftline_, yy + self_w->y + 1,
                                      xx + self_w->x - self->leftline_ + ww + xtra_ww);
                        }

                        xx += ww;
                        if ((fsize + 2) > hh) hh = fsize + 2;
                        needspace = 0;
                    } else if (pre) {
                        while (isspace((unsigned char)*ptr)) {
                            if (*ptr == '\n') {
                                fl_draw(buf.data, xx + self_w->x - self->leftline_, yy + self_w->y);
                                if (underline)
                                    fl_xyline(xx + self_w->x - self->leftline_, yy + self_w->y + 1,
                                              xx + self_w->x - self->leftline_ + (int)hv_width(&buf));
                                hv_clear(&buf);
                                if (line < 31) line++;
                                xx = block->line[line];
                                yy += hh;
                                hh = fsize + 2;
                            } else if (*ptr == '\t') {
                                hv_add_char(&buf, ' ');
                                while (buf.size & 7) hv_add_char(&buf, ' ');
                            } else {
                                hv_add_char(&buf, ' ');
                            }
                            if ((fsize + 2) > hh) hh = fsize + 2;
                            ptr++;
                        }

                        if (buf.size > 0) {
                            fl_draw(buf.data, xx + self_w->x - self->leftline_, yy + self_w->y);
                            ww = (int)hv_width(&buf);
                            hv_clear(&buf);
                            if (underline)
                                fl_xyline(xx + self_w->x - self->leftline_, yy + self_w->y + 1,
                                          xx + self_w->x - self->leftline_ + ww);
                            xx += ww;
                        }

                        needspace = 0;
                    } else {
                        hv_clear(&buf);
                        while (isspace((unsigned char)*ptr)) ptr++;
                    }
                }

                if (*ptr == '<') {
                    ptr++;

                    if (strncmp(ptr, "!--", 3) == 0) {
                        ptr += 3;
                        if ((ptr = strstr(ptr, "-->")) != NULL) { ptr += 3; continue; }
                        else break;
                    }

                    while (*ptr && *ptr != '>' && !isspace((unsigned char)*ptr)) hv_add_char(&buf, *ptr++);
                    attrs = ptr;
                    while (*ptr && *ptr != '>') ptr++;
                    if (*ptr == '>') ptr++;

                    if (hv_cmp(&buf, "HEAD")) {
                        head = 1;
                    } else if (hv_cmp(&buf, "BR")) {
                        if (line < 31) line++;
                        xx = block->line[line];
                        yy += hh;
                        hh = 0;
                    } else if (hv_cmp(&buf, "HR")) {
                        fl_line(block->x + self_w->x, yy + self_w->y, block->w + self_w->x, yy + self_w->y);
                        if (line < 31) line++;
                        xx = block->line[line];
                        yy += 2 * fsize;
                        hh = 0;
                    } else if (hv_cmp(&buf, "CENTER") || hv_cmp(&buf, "P") ||
                               hv_cmp(&buf, "H1") || hv_cmp(&buf, "H2") || hv_cmp(&buf, "H3") ||
                               hv_cmp(&buf, "H4") || hv_cmp(&buf, "H5") || hv_cmp(&buf, "H6") ||
                               hv_cmp(&buf, "UL") || hv_cmp(&buf, "OL") || hv_cmp(&buf, "DL") ||
                               hv_cmp(&buf, "LI") || hv_cmp(&buf, "DD") || hv_cmp(&buf, "DT") ||
                               hv_cmp(&buf, "PRE")) {
                        if (tolower((unsigned char)buf.data[0]) == 'h') {
                            font = FL_HELVETICA_BOLD;
                            fsize = self->textsize_ + '7' - buf.data[1];
                        } else if (hv_cmp(&buf, "DT")) {
                            font = self->textfont_ | FL_ITALIC;
                            fsize = self->textsize_;
                        } else if (hv_cmp(&buf, "PRE")) {
                            font = FL_COURIER;
                            fsize = self->textsize_;
                            pre = 1;
                        }

                        if (hv_cmp(&buf, "LI")) {
                            unsigned char bullet[4] = { 0xe2, 0x80, 0xa2, 0x00 };
                            fl_draw((const char *)bullet, xx - fsize + self_w->x - self->leftline_, yy + self_w->y);
                        }

                        pushfont2(self, font, fsize);
                        hv_clear(&buf);
                    } else if (hv_cmp(&buf, "A") && get_attr(attrs, "HREF", attr, sizeof(attr)) != NULL) {
                        fl_color(self->linkcolor_);
                        underline = 1;
                    } else if (hv_cmp(&buf, "/A")) {
                        fl_color(self->textcolor_);
                        underline = 0;
                    } else if (hv_cmp(&buf, "FONT")) {
                        /* Faithful upstream quirk, not a porting bug: a
                         * <FONT COLOR> mutates the self->textcolor_
                         * member directly, and </FONT> only pops the
                         * local font-stack (font/size/color) used for
                         * fl_font()/fl_color() -- it does not restore
                         * this member. The color "leaks" into every
                         * later block's initfont() until the next
                         * format() pass resets it from defcolor_. */
                        if (get_attr(attrs, "COLOR", attr, sizeof(attr)) != NULL) self->textcolor_ = get_color(attr, self->textcolor_);

                        if (get_attr(attrs, "FACE", attr, sizeof(attr)) != NULL) {
                            if (!strncasecmp(attr, "helvetica", 9) || !strncasecmp(attr, "arial", 5) || !strncasecmp(attr, "sans", 4)) font = FL_HELVETICA;
                            else if (!strncasecmp(attr, "times", 5) || !strncasecmp(attr, "serif", 5)) font = FL_TIMES;
                            else if (!strncasecmp(attr, "symbol", 6)) font = FL_SYMBOL;
                            else font = FL_COURIER;
                        }

                        if (get_attr(attrs, "SIZE", attr, sizeof(attr)) != NULL) {
                            if (isdigit((unsigned char)attr[0])) fsize = (int)(self->textsize_ * pow(1.2, atof(attr) - 3.0));
                            else fsize = (int)(fsize * pow(1.2, atof(attr) - 3.0));
                        }

                        pushfont2(self, font, fsize);
                    } else if (hv_cmp(&buf, "/FONT")) {
                        popfont(self, &font, &fsize, &fcolor);
                    } else if (hv_cmp(&buf, "U")) {
                        underline = 1;
                    } else if (hv_cmp(&buf, "/U")) {
                        underline = 0;
                    } else if (hv_cmp(&buf, "B") || hv_cmp(&buf, "STRONG")) {
                        font |= FL_BOLD;
                        pushfont2(self, font, fsize);
                    } else if (hv_cmp(&buf, "TD") || hv_cmp(&buf, "TH")) {
                        /* Tables are not laid out (see header's Known
                         * differences); still honor bold/plain TH/TD
                         * so any stray text at least keeps the right
                         * weight. */
                        if (tolower((unsigned char)buf.data[1]) == 'h') { font |= FL_BOLD; pushfont2(self, font, fsize); }
                        else pushfont2(self, self->textfont_, fsize);
                    } else if (hv_cmp(&buf, "I") || hv_cmp(&buf, "EM")) {
                        font |= FL_ITALIC;
                        pushfont2(self, font, fsize);
                    } else if (hv_cmp(&buf, "CODE") || hv_cmp(&buf, "TT")) {
                        font = FL_COURIER;
                        pushfont2(self, font, fsize);
                    } else if (hv_cmp(&buf, "KBD")) {
                        font = FL_COURIER_BOLD;
                        pushfont2(self, font, fsize);
                    } else if (hv_cmp(&buf, "VAR")) {
                        font = FL_COURIER_ITALIC;
                        pushfont2(self, font, fsize);
                    } else if (hv_cmp(&buf, "/HEAD")) {
                        head = 0;
                    } else if (hv_cmp(&buf, "/H1") || hv_cmp(&buf, "/H2") || hv_cmp(&buf, "/H3") ||
                               hv_cmp(&buf, "/H4") || hv_cmp(&buf, "/H5") || hv_cmp(&buf, "/H6") ||
                               hv_cmp(&buf, "/B") || hv_cmp(&buf, "/STRONG") ||
                               hv_cmp(&buf, "/I") || hv_cmp(&buf, "/EM") ||
                               hv_cmp(&buf, "/CODE") || hv_cmp(&buf, "/TT") ||
                               hv_cmp(&buf, "/KBD") || hv_cmp(&buf, "/VAR") ||
                               hv_cmp(&buf, "/TD") || hv_cmp(&buf, "/TH")) {
                        popfont(self, &font, &fsize, &fcolor);
                    } else if (hv_cmp(&buf, "/PRE")) {
                        popfont(self, &font, &fsize, &fcolor);
                        pre = 0;
                    }
                    hv_clear(&buf);
                } else if (*ptr == '\n' && pre) {
                    fl_draw(buf.data, xx + self_w->x - self->leftline_, yy + self_w->y);
                    hv_clear(&buf);
                    if (line < 31) line++;
                    xx = block->line[line];
                    yy += hh;
                    hh = fsize + 2;
                    needspace = 0;
                    ptr++;
                } else if (isspace((unsigned char)*ptr)) {
                    if (pre) {
                        if (*ptr == ' ') {
                            hv_add_char(&buf, ' ');
                        } else {
                            hv_add_char(&buf, ' ');
                            while (buf.size & 7) hv_add_char(&buf, ' ');
                        }
                    }
                    ptr++;
                    needspace = 1;
                } else if (*ptr == '&') {
                    int qch;
                    ptr++;
                    qch = quote_char(ptr);
                    if (qch < 0) hv_add_char(&buf, '&');
                    else {
                        hv_add_ucs(&buf, qch);
                        ptr = strchr(ptr, ';') + 1;
                    }
                    if ((fsize + 2) > hh) hh = fsize + 2;
                } else {
                    hv_add_char(&buf, *ptr++);
                    if ((fsize + 2) > hh) hh = fsize + 2;
                }
            }

            if (buf.size > 0 && !pre && !head) {
                ww = (int)hv_width(&buf);

                if (needspace && xx > block->x) xx += (int)fl_width(" ", 1);

                if ((xx + ww) > block->w) {
                    if (line < 31) line++;
                    xx = block->line[line];
                    yy += hh;
                    hh = 0;
                }
            }

            if (buf.size > 0 && !head) {
                fl_draw(buf.data, xx + self_w->x - self->leftline_, yy + self_w->y);
                if (underline)
                    fl_xyline(xx + self_w->x - self->leftline_, yy + self_w->y + 1, xx + self_w->x - self->leftline_ + ww);
            }
        }
    }

    fl_pop_clip();
}

/* -------------------------------------------------------------------
 * find()/find_link()/follow_link()
 * ---------------------------------------------------------------- */
int Fl_Help_View_find(Fl_Help_View *self, const char *s, int p) {
    int i, c;
    Fl_Help_Block *blk;
    const char *bp, *bs, *sp;

    if (!s || !self->value_) return -1;

    if (p < 0 || p >= (int)strlen(self->value_)) p = 0;
    else if (p > 0) p++;

    for (i = self->nblocks_, blk = self->blocks_; i > 0; i--, blk++) {
        if (blk->end < (self->value_ + p)) continue;

        if (blk->start < (self->value_ + p)) bp = self->value_ + p;
        else bp = blk->start;

        for (sp = s, bs = bp; *sp && *bp && bp < blk->end; bp++) {
            if (*bp == '<') {
                while (*bp && bp < blk->end && *bp != '>') bp++;
                continue;
            } else if (*bp == '&') {
                if ((c = quote_char(bp + 1)) < 0) c = '&';
                else bp = strchr(bp + 1, ';') + 1;
            } else {
                c = (unsigned char)*bp;
            }

            if (tolower(*sp) == tolower(c)) sp++;
            else {
                sp = s;
                bs++;
                bp = bs;
            }
        }

        if (!*sp) {
            Fl_Help_View_set_topline(self, blk->y - blk->h);
            return (int)(blk->end - self->value_);
        }
    }

    return -1;
}

static Fl_Help_Link *find_link(Fl_Help_View *self, int xx, int yy) {
    int i;
    Fl_Help_Link *linkp;
    for (i = self->nlinks_, linkp = self->links_; i > 0; i--, linkp++) {
        if (xx >= linkp->x && xx < linkp->w && yy >= linkp->y && yy < linkp->h) break;
    }
    return i ? linkp : NULL;
}

static void follow_link(Fl_Help_View *self, Fl_Help_Link *linkp) {
    char target[32];
    snprintf(target, sizeof(target), "%s", linkp->name);
    Fl_Widget_set_changed(&self->group.widget);

    if (strcmp(linkp->filename, self->filename_) != 0 && linkp->filename[0]) {
        char temp[2 * FL_PATH_MAX];
        size_t len;

        if (linkp->filename[0] == '/' || strchr(linkp->filename, ':') != NULL)
            snprintf(temp, sizeof(temp), "%s", linkp->filename);
        else if (self->directory_[0])
            snprintf(temp, sizeof(temp), "%s/%s", self->directory_, linkp->filename);
        else
            snprintf(temp, sizeof(temp), "%s", linkp->filename);

        if (linkp->name[0]) {
            len = strlen(temp);
            snprintf(temp + len, sizeof(temp) - len, "#%s", linkp->name);
        }

        Fl_Help_View_load(self, temp);
    } else if (target[0]) {
        Fl_Help_View_set_topline_target(self, target);
    } else {
        Fl_Help_View_set_topline(self, 0);
    }

    Fl_Help_View_set_leftline(self, 0);
}

/* -------------------------------------------------------------------
 * handle()
 * ---------------------------------------------------------------- */
static int help_view_handle(Fl_Widget *self_w, int event) {
    Fl_Help_View *self = (Fl_Help_View *)self_w;
    static Fl_Help_Link *linkp = NULL;
    int xx = Fl_event_x() - self_w->x + self->leftline_;
    int yy = Fl_event_y() - self_w->y + self->topline_;

    switch (event) {
        case FL_FOCUS:
            Fl_Widget_redraw(self_w);
            return 1;
        case FL_UNFOCUS:
            Fl_Widget_redraw(self_w);
            return 1;
        case FL_ENTER:
            Fl_Group_handle(self_w, event);
            return 1;
        case FL_LEAVE:
            break;
        case FL_MOVE:
            return 1;
        case FL_PUSH:
            if (Fl_Group_handle(self_w, event)) return 1;
            linkp = find_link(self, xx, yy);
            return 1;
        case FL_DRAG:
            return 1;
        case FL_RELEASE:
            if (linkp) {
                if (Fl_event_is_click()) follow_link(self, linkp);
                linkp = NULL;
                return 1;
            }
            return 1;
        default:
            break;
    }
    return Fl_Group_handle(self_w, event);
}

/* -------------------------------------------------------------------
 * load()/value()/resize()/topline()/leftline()/text*()
 * ---------------------------------------------------------------- */
int Fl_Help_View_load(Fl_Help_View *self, const char *f) {
    FILE *fp;
    long len;
    char *target;
    char *slash;
    const char *localname;
    char error[2 * FL_PATH_MAX];
    char newname[FL_PATH_MAX];

    snprintf(newname, sizeof(newname), "%s", f);
    if ((target = strrchr(newname, '#')) != NULL) *target++ = '\0';

    if (self->link_) localname = self->link_(&self->group.widget, newname);
    else localname = self->filename_;

    if (!localname) return 0;

    free_data(self);

    snprintf(self->filename_, sizeof(self->filename_), "%s", newname);
    snprintf(self->directory_, sizeof(self->directory_), "%s", newname);

    if ((slash = strrchr(self->directory_, '/')) == NULL) self->directory_[0] = '\0';
    else if (slash > self->directory_ && slash[-1] != '/') *slash = '\0';

    if (strncmp(localname, "file:", 5) == 0) localname += 5;

    fp = fopen(localname, "rb");
    if (fp) {
        fseek(fp, 0, SEEK_END);
        len = ftell(fp);
        rewind(fp);

        self->value_ = (const char *)calloc((size_t)len + 1, 1);
        if (fread((void *)self->value_, 1, (size_t)len, fp) == 0) { /* use default 0 */ }
        fclose(fp);
    } else {
        snprintf(error, sizeof(error),
                 "<HTML><HEAD><TITLE>Error</TITLE></HEAD>"
                 "<BODY><H1>Error</H1>"
                 "<P>Unable to follow the link \"%s\" - %s.</P></BODY>",
                 localname, strerror(errno));
        self->value_ = strdup(error);
    }

    format(self);

    if (target) Fl_Help_View_set_topline_target(self, target);
    else Fl_Help_View_set_topline(self, 0);

    return 0;
}

void Fl_Help_View_set_value(Fl_Help_View *self, const char *val) {
    free_data(self);
    Fl_Widget_set_changed(&self->group.widget);

    if (!val) return;

    self->value_ = strdup(val);

    format(self);

    Fl_Help_View_set_topline(self, 0);
    Fl_Help_View_set_leftline(self, 0);
}

static void help_view_resize(Fl_Widget *self_w, int x, int y, int w, int h) {
    Fl_Help_View *self = (Fl_Help_View *)self_w;
    uchar b = Fl_Widget_box(self_w) ? Fl_Widget_box(self_w) : FL_DOWN_BOX;
    int scrollsize;

    Fl_Widget_default_resize(self_w, x, y, w, h);

    scrollsize = self->scrollbar_size_ ? self->scrollbar_size_ : Fl_scrollbar_size();
    Fl_Widget_resize(FL_WIDGET(self->scrollbar_), self_w->x + self_w->w - scrollsize - fl_box_dw(b) + fl_box_dx(b),
                      self_w->y + fl_box_dy(b), scrollsize, self_w->h - scrollsize - fl_box_dh(b));
    Fl_Widget_resize(FL_WIDGET(self->hscrollbar_), self_w->x + fl_box_dx(b),
                      self_w->y + self_w->h - scrollsize - fl_box_dh(b) + fl_box_dy(b), self_w->w - scrollsize - fl_box_dw(b), scrollsize);

    format(self);
}

void Fl_Help_View_set_topline_target(Fl_Help_View *self, const char *n) {
    Fl_Help_Target key, *target;
    if (self->ntargets_ == 0) return;
    snprintf(key.name, sizeof(key.name), "%s", n);
    target = (Fl_Help_Target *)bsearch(&key, self->targets_, (size_t)self->ntargets_, sizeof(Fl_Help_Target), compare_targets);
    if (target) Fl_Help_View_set_topline(self, target->y);
}

void Fl_Help_View_set_topline(Fl_Help_View *self, int top) {
    int scrollsize;
    Fl_Widget *self_w = &self->group.widget;
    if (!self->value_) return;

    scrollsize = self->scrollbar_size_ ? self->scrollbar_size_ : Fl_scrollbar_size();
    if (self->size_ < (self_w->h - scrollsize) || top < 0) top = 0;
    else if (top > self->size_) top = self->size_;

    self->topline_ = top;
    Fl_Scrollbar_set_value_range(self->scrollbar_, self->topline_, self_w->h - scrollsize, 0, self->size_);
    Fl_Widget_do_callback(self_w);
    Fl_Widget_redraw(self_w);
}

void Fl_Help_View_set_leftline(Fl_Help_View *self, int left) {
    int scrollsize;
    Fl_Widget *self_w = &self->group.widget;
    if (!self->value_) return;

    scrollsize = self->scrollbar_size_ ? self->scrollbar_size_ : Fl_scrollbar_size();
    if (self->hsize_ < (self_w->w - scrollsize) || left < 0) left = 0;
    else if (left > self->hsize_) left = self->hsize_;

    self->leftline_ = left;
    Fl_Scrollbar_set_value_range(self->hscrollbar_, self->leftline_, self_w->w - scrollsize, 0, self->hsize_);
    Fl_Widget_redraw(self_w);
}

void Fl_Help_View_set_textcolor(Fl_Help_View *self, Fl_Color c) {
    if (self->textcolor_ == self->defcolor_) self->textcolor_ = c;
    self->defcolor_ = c;
}
void Fl_Help_View_set_textfont(Fl_Help_View *self, Fl_Font f) {
    self->textfont_ = f;
    format(self);
}
void Fl_Help_View_set_textsize(Fl_Help_View *self, Fl_Fontsize s) {
    self->textsize_ = s;
    format(self);
}

/* -------------------------------------------------------------------
 * Scrollbar callbacks / construction.
 * ---------------------------------------------------------------- */
static void scrollbar_cb(Fl_Widget *s, void *data) {
    (void)data;
    Fl_Help_View *self = (Fl_Help_View *)Fl_Widget_parent(s);
    Fl_Help_View_set_topline(self, Fl_Scrollbar_value((Fl_Scrollbar *)s));
}
static void hscrollbar_cb(Fl_Widget *s, void *data) {
    (void)data;
    Fl_Help_View *self = (Fl_Help_View *)Fl_Widget_parent(s);
    Fl_Help_View_set_leftline(self, Fl_Scrollbar_value((Fl_Scrollbar *)s));
}

static void help_view_destroy(Fl_Widget *self_w) {
    Fl_Help_View *self = (Fl_Help_View *)self_w;
    free_data(self);
    Fl_Group_destroy(self_w);
}

const Fl_WidgetOps fl_help_view_ops = {
    help_view_draw,
    help_view_handle,
    help_view_resize,
    NULL, NULL,
    help_view_destroy,
    Fl_Group_as_group,
    NULL
};

void Fl_Help_View_init(Fl_Help_View *self, int x, int y, int w, int h, const char *label) {
    Fl_Widget *self_w;
    int sbsize = Fl_scrollbar_size();

    Fl_Group_init(&self->group, x, y, w, h, label);
    self_w = &self->group.widget;
    self_w->ops = &fl_help_view_ops;

    Fl_Widget_set_colors(self_w, FL_BACKGROUND2_COLOR, FL_SELECTION_COLOR);

    self->title_[0] = '\0';
    self->defcolor_ = FL_FOREGROUND_COLOR;
    self->bgcolor_ = FL_BACKGROUND_COLOR;
    self->textcolor_ = FL_FOREGROUND_COLOR;
    self->linkcolor_ = FL_SELECTION_COLOR;
    self->textfont_ = FL_TIMES;
    self->textsize_ = 12;
    self->value_ = NULL;

    self->ablocks_ = 0;
    self->nblocks_ = 0;
    self->blocks_ = NULL;

    self->link_ = NULL;

    self->alinks_ = 0;
    self->nlinks_ = 0;
    self->links_ = NULL;

    self->atargets_ = 0;
    self->ntargets_ = 0;
    self->targets_ = NULL;

    self->directory_[0] = '\0';
    self->filename_[0] = '\0';

    self->topline_ = 0;
    self->leftline_ = 0;
    self->size_ = 0;
    self->hsize_ = 0;
    self->scrollbar_size_ = 0;
    self->nfonts_ = 0;

    self->scrollbar_ = Fl_Scrollbar_new(x + w - sbsize, y, sbsize, h - sbsize, NULL);
    Fl_Scrollbar_set_value_range(self->scrollbar_, 0, h, 0, 1);
    Fl_Valuator_set_step(&self->scrollbar_->slider.valuator, 8.0);
    Fl_Widget_show(FL_WIDGET(self->scrollbar_));
    Fl_Widget_set_callback(FL_WIDGET(self->scrollbar_), scrollbar_cb, NULL);

    self->hscrollbar_ = Fl_Scrollbar_new(x, y + h - sbsize, w - sbsize, sbsize, NULL);
    Fl_Scrollbar_set_value_range(self->hscrollbar_, 0, w, 0, 1);
    Fl_Valuator_set_step(&self->hscrollbar_->slider.valuator, 8.0);
    Fl_Widget_show(FL_WIDGET(self->hscrollbar_));
    Fl_Widget_set_callback(FL_WIDGET(self->hscrollbar_), hscrollbar_cb, NULL);
    Fl_Widget_set_type(FL_WIDGET(self->hscrollbar_), FL_HORIZONTAL);

    Fl_Group_end(&self->group);

    Fl_Widget_resize(self_w, x, y, w, h);
}

Fl_Help_View *Fl_Help_View_new(int x, int y, int w, int h, const char *label) {
    Fl_Help_View *self = (Fl_Help_View *)malloc(sizeof(Fl_Help_View));
    Fl_Help_View_init(self, x, y, w, h, label);
    return self;
}
