/*
 * cfltk - fl_draw.c
 *
 * Backend-independent half of FL/fl_draw.H: color math, box drawing
 * (the fl_box_table dispatch, translated from src/fl_boxtype.cxx), and
 * label drawing/measuring (translated from the protected Fl_Label
 * methods in src/Fl_Widget.cxx). Everything that actually touches a
 * display -- fl_graphics_driver itself -- is installed by the platform
 * backend (src/backend/x11/fl_x11_driver.c).
 */
#include <stdlib.h>
#include <string.h>

#include "cfltk/fl_draw.h"
#include "cfltk/fl_colormap.h"
#include "cfltk/Fl_Image.h"
#include "../backend/fl_backend.h"

static const Fl_Graphics_Driver *g_driver = NULL;

void fl_set_graphics_driver(const Fl_Graphics_Driver *driver) { g_driver = driver; }

/* Lazily opens the display/installs the driver on first use, exactly
 * like upstream's fl_open_display() being called implicitly by font
 * measurement -- this is what lets client code measure text (e.g.
 * Fl_Browser building its item list) before any window is shown, a
 * very common FLTK usage pattern. Previously this only happened as a
 * side effect of Fl_Widget_show() on a window, which crashed any code
 * that measured text first (fl_graphics_driver() returning NULL). */
const Fl_Graphics_Driver *fl_graphics_driver(void) {
    if (!g_driver) fl_backend_init();
    return g_driver;
}

/* ------------------------------------------------------------------ */
/* Color math (src/fl_color.cxx)                                       */
/* ------------------------------------------------------------------ */

Fl_Color fl_color_average(Fl_Color color1, Fl_Color color2, float weight) {
    unsigned rgb1, rgb2;
    uchar r, g, b;

    rgb1 = (color1 & 0xffffff00u) ? color1 : fl_colormap[color1 & 255];
    rgb2 = (color2 & 0xffffff00u) ? color2 : fl_colormap[color2 & 255];

    r = (uchar)(((uchar)(rgb1 >> 24)) * weight + ((uchar)(rgb2 >> 24)) * (1 - weight));
    g = (uchar)(((uchar)(rgb1 >> 16)) * weight + ((uchar)(rgb2 >> 16)) * (1 - weight));
    b = (uchar)(((uchar)(rgb1 >> 8)) * weight + ((uchar)(rgb2 >> 8)) * (1 - weight));

    return fl_rgb_color(r, g, b);
}

Fl_Color fl_inactive(Fl_Color c) { return fl_color_average(c, FL_GRAY, 0.33f); }

Fl_Color fl_contrast(Fl_Color fg, Fl_Color bg) {
    unsigned c1, c2;
    int l1, l2;

    c1 = (fg & 0xffffff00u) ? fg : fl_colormap[fg];
    c2 = (bg & 0xffffff00u) ? bg : fl_colormap[bg];

    l1 = ((c1 >> 24) * 30 + ((c1 >> 16) & 255) * 59 + ((c1 >> 8) & 255) * 11) / 100;
    l2 = ((c2 >> 24) * 30 + ((c2 >> 16) & 255) * 59 + ((c2 >> 8) & 255) * 11) / 100;

    if ((l1 - l2) > 99) return fg;
    else if ((l2 - l1) > 99) return fg;
    else if (l2 > 127) return FL_BLACK;
    else return FL_WHITE;
}

/* ------------------------------------------------------------------ */
/* Text                                                                 */
/* ------------------------------------------------------------------ */

void fl_draw(const char *str, int x, int y) {
    if (str) fl_draw_text(str, (int)strlen(str), x, y);
}

/* ------------------------------------------------------------------ */
/* Box drawing (src/fl_boxtype.cxx)                                    */
/* ------------------------------------------------------------------ */

#define D1 2 /* BORDER_WIDTH, matches upstream's configure default of 2 */
#define D2 (D1 + D1)

static int g_box_drawing_active = 1;

void fl_set_box_drawing_active(int active) { g_box_drawing_active = active; }
int fl_box_drawing_active(void) { return g_box_drawing_active; }

Fl_Color fl_box_color(Fl_Color c) { return g_box_drawing_active ? c : fl_inactive(c); }
void fl_set_box_color(Fl_Color c) { fl_color(fl_box_color(c)); }

/* 'A'..'X' -> one of 24 standard grayscale steps, dimmed when the box
 * being drawn is inactive. Mirrors fl_gray_ramp()/active_ramp/inactive_ramp. */
static const uchar k_active_ramp[24] = {
    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
    44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55
};
static const uchar k_inactive_ramp[24] = {
    43, 43, 44, 44, 44, 45, 45, 46, 46, 46, 47, 47,
    48, 48, 48, 49, 49, 49, 50, 50, 51, 51, 52, 52
};

static void fl_ramp_frame(const char *s, int x, int y, int w, int h) {
    const uchar *g = (g_box_drawing_active ? k_active_ramp : k_inactive_ramp) - 'A';
    if (h <= 0 || w <= 0) return;
    for (; *s;) {
        fl_color(g[(int)*s++]);
        fl_xyline(x, y, x + w - 1);
        y++; if (--h <= 0) break;
        fl_color(g[(int)*s++]);
        fl_yxline(x, y + h - 1, y);
        x++; if (--w <= 0) break;
        fl_color(g[(int)*s++]);
        fl_xyline(x, y + h - 1, x + w - 1);
        if (--h <= 0) break;
        fl_color(g[(int)*s++]);
        fl_yxline(x + w - 1, y + h - 1, y);
        if (--w <= 0) break;
    }
}

static void fl_ramp_frame2(const char *s, int x, int y, int w, int h) {
    const uchar *g = (g_box_drawing_active ? k_active_ramp : k_inactive_ramp) - 'A';
    if (h <= 0 || w <= 0) return;
    for (; *s;) {
        fl_color(g[(int)*s++]);
        fl_xyline(x, y + h - 1, x + w - 1);
        if (--h <= 0) break;
        fl_color(g[(int)*s++]);
        fl_yxline(x + w - 1, y + h - 1, y);
        if (--w <= 0) break;
        fl_color(g[(int)*s++]);
        fl_xyline(x, y, x + w - 1);
        y++; if (--h <= 0) break;
        fl_color(g[(int)*s++]);
        fl_yxline(x, y + h - 1, y);
        x++; if (--w <= 0) break;
    }
}

static void fl_no_box(int x, int y, int w, int h, Fl_Color c) { (void)x; (void)y; (void)w; (void)h; (void)c; }

static void fl_flat_box(int x, int y, int w, int h, Fl_Color c) { fl_rectf_c(x, y, w, h, fl_box_color(c)); }

static void fl_thin_up_frame(int x, int y, int w, int h, Fl_Color c) { (void)c; fl_ramp_frame2("HHWW", x, y, w, h); }
static void fl_thin_up_box(int x, int y, int w, int h, Fl_Color c) {
    fl_thin_up_frame(x, y, w, h, c);
    fl_set_box_color(c);
    fl_rectf(x + 1, y + 1, w - 2, h - 2);
}
static void fl_thin_down_frame(int x, int y, int w, int h, Fl_Color c) { (void)c; fl_ramp_frame2("WWHH", x, y, w, h); }
static void fl_thin_down_box(int x, int y, int w, int h, Fl_Color c) {
    fl_thin_down_frame(x, y, w, h, c);
    fl_set_box_color(c);
    fl_rectf(x + 1, y + 1, w - 2, h - 2);
}

static void fl_up_frame(int x, int y, int w, int h, Fl_Color c) { (void)c; fl_ramp_frame2("AAWWMMTT", x, y, w, h); }
static void fl_up_box(int x, int y, int w, int h, Fl_Color c) {
    fl_up_frame(x, y, w, h, c);
    fl_set_box_color(c);
    fl_rectf(x + D1, y + D1, w - D2, h - D2);
}
static void fl_down_frame(int x, int y, int w, int h, Fl_Color c) { (void)c; fl_ramp_frame2("WWMMPPAA", x, y, w, h); }
static void fl_down_box(int x, int y, int w, int h, Fl_Color c) {
    fl_down_frame(x, y, w, h, c);
    fl_set_box_color(c);
    fl_rectf(x + D1, y + D1, w - D2, h - D2);
}

static void fl_engraved_frame(int x, int y, int w, int h, Fl_Color c) { (void)c; fl_ramp_frame("HHWWWWHH", x, y, w, h); }
static void fl_engraved_box(int x, int y, int w, int h, Fl_Color c) {
    fl_engraved_frame(x, y, w, h, c);
    fl_set_box_color(c);
    fl_rectf(x + 2, y + 2, w - 4, h - 4);
}
static void fl_embossed_frame(int x, int y, int w, int h, Fl_Color c) { (void)c; fl_ramp_frame("WWHHHHWW", x, y, w, h); }
static void fl_embossed_box(int x, int y, int w, int h, Fl_Color c) {
    fl_embossed_frame(x, y, w, h, c);
    fl_set_box_color(c);
    fl_rectf(x + 2, y + 2, w - 4, h - 4);
}

static void fl_border_frame(int x, int y, int w, int h, Fl_Color c) {
    fl_set_box_color(c);
    fl_rect(x, y, w, h);
}
static void fl_border_box(int x, int y, int w, int h, Fl_Color c) {
    fl_set_box_color(FL_BLACK);
    fl_rect(x, y, w, h);
    fl_set_box_color(c);
    fl_rectf(x + 1, y + 1, w - 2, h - 2);
}

typedef struct Fl_Box_Table_Entry {
    Fl_Box_Draw_F *fn;
    uchar dx, dy, dw, dh;
    int is_frame;
} Fl_Box_Table_Entry;

/* Indexed exactly like enum Fl_Boxtype in Enumerations.h. Entries beyond
 * FL_BORDER_FRAME fall back to fl_flat_box/fl_border_box in fl_draw_box()
 * until their dedicated box-type files are ported (see docs/DESIGN.md);
 * upstream keeps those in separate translation units for the same reason
 * cfltk will: don't force every embedded build to link rounded/plastic/
 * gtk/gleam box art it will never use. */
static const Fl_Box_Table_Entry k_box_table[] = {
    { fl_no_box,          0, 0, 0,  0,  0 }, /* FL_NO_BOX */
    { fl_flat_box,        0, 0, 0,  0,  0 }, /* FL_FLAT_BOX */
    { fl_up_box,         D1,D1,D2, D2, 0 },  /* FL_UP_BOX */
    { fl_down_box,       D1,D1,D2, D2, 0 },  /* FL_DOWN_BOX */
    { fl_up_frame,       D1,D1,D2, D2, 1 },  /* FL_UP_FRAME */
    { fl_down_frame,     D1,D1,D2, D2, 1 },  /* FL_DOWN_FRAME */
    { fl_thin_up_box,     1, 1, 2,  2,  0 }, /* FL_THIN_UP_BOX */
    { fl_thin_down_box,   1, 1, 2,  2,  0 }, /* FL_THIN_DOWN_BOX */
    { fl_thin_up_frame,   1, 1, 2,  2,  1 }, /* FL_THIN_UP_FRAME */
    { fl_thin_down_frame, 1, 1, 2,  2,  1 }, /* FL_THIN_DOWN_FRAME */
    { fl_engraved_box,    2, 2, 4,  4,  0 }, /* FL_ENGRAVED_BOX */
    { fl_embossed_box,    2, 2, 4,  4,  0 }, /* FL_EMBOSSED_BOX */
    { fl_engraved_frame,  2, 2, 4,  4,  1 }, /* FL_ENGRAVED_FRAME */
    { fl_embossed_frame,  2, 2, 4,  4,  1 }, /* FL_EMBOSSED_FRAME */
    { fl_border_box,      1, 1, 2,  2,  0 }, /* FL_BORDER_BOX */
    { fl_border_box,      1, 1, 2,  2,  0 }, /* FL_SHADOW_BOX (falls back) */
    { fl_border_frame,    1, 1, 2,  2,  1 }, /* FL_BORDER_FRAME */
    { fl_border_frame,    1, 1, 2,  2,  1 }, /* FL_SHADOW_FRAME (falls back) */
};
#define K_BOX_TABLE_COUNT (int)(sizeof(k_box_table) / sizeof(k_box_table[0]))

static const Fl_Box_Table_Entry *box_entry(uchar boxtype) {
    if (boxtype < K_BOX_TABLE_COUNT) return &k_box_table[boxtype];
    return &k_box_table[FL_UP_BOX]; /* safe, visible fallback for not-yet-ported types */
}

void fl_draw_box(uchar boxtype, int x, int y, int w, int h, Fl_Color c) {
    box_entry(boxtype)->fn(x, y, w, h, c);
}
int fl_box_dx(uchar boxtype) { return box_entry(boxtype)->dx; }
int fl_box_dy(uchar boxtype) { return box_entry(boxtype)->dy; }
int fl_box_dw(uchar boxtype) { return box_entry(boxtype)->dw; }
int fl_box_dh(uchar boxtype) { return box_entry(boxtype)->dh; }
int fl_box_is_frame(uchar boxtype) { return box_entry(boxtype)->is_frame; }

/* ------------------------------------------------------------------ */
/* Label drawing/measuring (protected Fl_Label::draw()/measure() in
 * src/Fl_Widget.cxx, reduced to the FL_NORMAL_LABEL text+image case --
 * the shadow/engraved/embossed label types are follow-up work, see
 * docs/DESIGN.md). */
/* ------------------------------------------------------------------ */

int fl_draw_shortcut = 0;

/* Strips a single '&' mnemonic marker from label text into out (capped
 * to outcap-1 bytes): "&&" collapses to one literal '&'; a lone '&'
 * before any other character is removed and that character's byte
 * offset in `out` is returned via *underline (-1 if no marker, or if
 * fl_draw_shortcut is unset -- text is then copied through unchanged). */
static int strip_shortcut_marker(const char *in, char *out, int outcap, int *underline) {
    int oi = 0;
    *underline = -1;
    if (!in) { out[0] = '\0'; return 0; }
    if (!fl_draw_shortcut) {
        int n = (int)strlen(in);
        if (n > outcap - 1) n = outcap - 1;
        memcpy(out, in, (size_t)n);
        out[n] = '\0';
        return n;
    }
    while (*in && oi < outcap - 1) {
        if (*in == '&') {
            if (in[1] == '&') { out[oi++] = '&'; in += 2; continue; }
            in++;
            if (*in && *underline < 0) *underline = oi;
            continue;
        }
        out[oi++] = *in++;
    }
    out[oi] = '\0';
    return oi;
}

void fl_label_measure(const Fl_Label *label, int *w, int *h) {
    char buf[512];
    int underline;
    *w = 0;
    *h = 0;
    if (label->value && label->value[0]) {
        fl_font(label->font, label->size);
        strip_shortcut_marker(label->value, buf, (int)sizeof(buf), &underline);
        *w = (int)fl_width_str(buf);
        *h = fl_height();
    }
    if (label->image) {
        int iw = Fl_Image_w(label->image), ih = Fl_Image_h(label->image);
        if (iw > *w) *w = iw;
        *h += ih;
    }
}

/* Combined text+image label drawing, scoped down from upstream's
 * fl_draw(str,x,y,w,h,align,img,draw_symbols) (see docs/DESIGN.md):
 * stacks the image above or below the text depending on
 * FL_ALIGN_TEXT_OVER_IMAGE, or draws image-only / text-only when the
 * other is absent -- covering the overwhelming majority of real
 * widget labels (icon-only toolbar buttons, and text-with-icon
 * buttons/menu items). Known differences: no FL_ALIGN_IMAGE_NEXT_TO_TEXT
 * side-by-side layout, no '@'-prefixed inline symbol glyphs in labels
 * (fl_draw_symbol()'s mini-language -- distinct from Fl_Browser's own
 * '@'-format codes, which ARE ported, see docs/DESIGN.md), no
 * multi-line wrap (pre-existing text-only limitation, unchanged). */
void fl_label_draw(const Fl_Label *label, int x, int y, int w, int h, Fl_Align align) {
    int lw = 0, lh = 0, lx = 0, ly, underline = -1, dn = 0, baseline;
    int imgw = 0, imgh = 0, total_h, top;
    char buf[512];
    int has_text = label->value && label->value[0] ? 1 : 0;
    Fl_Image *img = (align & FL_ALIGN_IMAGE_BACKDROP) ? NULL : label->image;
    int text_over_image = (align & FL_ALIGN_TEXT_OVER_IMAGE) ? 1 : 0;

    if (label->type == FL_NO_LABEL) return;
    if (!has_text && !img) return;

    fl_font(label->font, label->size);
    fl_color(label->color);

    if (has_text) {
        dn = strip_shortcut_marker(label->value, buf, (int)sizeof(buf), &underline);
        lw = (int)fl_width_str(buf);
        lh = fl_height();
    }
    if (img) { imgw = Fl_Image_w(img); imgh = Fl_Image_h(img); }

    if (align & FL_ALIGN_CLIP) fl_push_clip(x, y, w, h);

    total_h = lh + imgh;
    if (align & FL_ALIGN_BOTTOM) top = y + h - total_h;
    else if (align & FL_ALIGN_TOP) top = y;
    else top = y + (h - total_h) / 2;

    if (img && !text_over_image) {
        if (align & FL_ALIGN_LEFT) lx = x;
        else if (align & FL_ALIGN_RIGHT) lx = x + w - imgw;
        else lx = x + (w - imgw) / 2;
        Fl_Image_draw_at(img, lx, top);
        top += imgh;
    }

    if (has_text) {
        if (align & FL_ALIGN_LEFT) lx = x;
        else if (align & FL_ALIGN_RIGHT) lx = x + w - lw;
        else lx = x + (w - lw) / 2;
        ly = top;
        baseline = ly + fl_height() - fl_descent();
        fl_draw_text(buf, dn, lx, baseline);
        if (underline >= 0) {
            int ux = lx + (int)fl_width(buf, underline);
            int uw = (int)fl_width(buf + underline, 1);
            if (uw < 1) uw = 1;
            fl_line(ux, baseline + 2, ux + uw - 1, baseline + 2);
        }
        top += lh;
    }

    if (img && text_over_image) {
        if (align & FL_ALIGN_LEFT) lx = x;
        else if (align & FL_ALIGN_RIGHT) lx = x + w - imgw;
        else lx = x + (w - imgw) / 2;
        Fl_Image_draw_at(img, lx, top);
    }

    if (align & FL_ALIGN_CLIP) fl_pop_clip();
}
