/*
 * cfltk - Enumerations.h
 *
 * C translation of FLTK 1.3 FL/Enumerations.H.
 * Values are preserved exactly as in the original so that resource files,
 * saved layouts, and any code relying on numeric constants remain valid.
 *
 * Box types that FLTK 1.3 registers lazily at runtime (the ones prefixed
 * with an underscore in the original, e.g. _FL_ROUND_UP_BOX) are compiled
 * in unconditionally here instead of being deferred through
 * fl_define_FL_ROUND_UP_BOX()-style registration functions. Embedded targets
 * do not need that startup-footprint trick, and it removes a class of
 * hidden global initialization the contract asks us to avoid. This is a
 * documented behavioral difference from upstream FLTK, not a numbering
 * change: every enumerator keeps its original integer value.
 */
#ifndef CFLTK_ENUMERATIONS_H
#define CFLTK_ENUMERATIONS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char uchar;

/* ------------------------------------------------------------------ */
/* Basic scalar types                                                  */
/* ------------------------------------------------------------------ */

typedef unsigned int Fl_Color;
typedef unsigned int Fl_Align;
typedef int Fl_Font;
typedef int Fl_Fontsize;
typedef uchar Fl_Boxtype_t;   /* stored form, matches Fl_Widget::box_ */
typedef unsigned int Fl_Shortcut; /* key | shift-flag bits, see FL_SHIFT etc. */

/* ------------------------------------------------------------------ */
/* Events (FL/Enumerations.H enum Fl_Event)                            */
/* ------------------------------------------------------------------ */

enum Fl_Event {
    FL_NO_EVENT                     = 0,
    FL_PUSH                         = 1,
    FL_RELEASE                      = 2,
    FL_ENTER                        = 3,
    FL_LEAVE                        = 4,
    FL_DRAG                         = 5,
    FL_FOCUS                        = 6,
    FL_UNFOCUS                      = 7,
    FL_KEYDOWN                      = 8,
    FL_KEYBOARD                     = 8,
    FL_KEYUP                        = 9,
    FL_CLOSE                        = 10,
    FL_MOVE                         = 11,
    FL_SHORTCUT                     = 12,
    FL_DEACTIVATE                   = 13,
    FL_ACTIVATE                     = 14,
    FL_HIDE                         = 15,
    FL_SHOW                         = 16,
    FL_PASTE                        = 17,
    FL_SELECTIONCLEAR               = 18,
    FL_MOUSEWHEEL                   = 19,
    FL_DND_ENTER                    = 20,
    FL_DND_DRAG                     = 21,
    FL_DND_LEAVE                    = 22,
    FL_DND_RELEASE                  = 23,
    FL_SCREEN_CONFIGURATION_CHANGED = 24,
    FL_FULLSCREEN                   = 25,
    FL_ZOOM_GESTURE                 = 26,
    FL_ZOOM_EVENT                   = 27
};

/* ------------------------------------------------------------------ */
/* Fl_Widget::when() flags                                             */
/* ------------------------------------------------------------------ */

enum Fl_When {
    FL_WHEN_NEVER             = 0,
    FL_WHEN_CHANGED           = 1,
    FL_WHEN_NOT_CHANGED       = 2,
    FL_WHEN_RELEASE           = 4,
    FL_WHEN_RELEASE_ALWAYS    = 6,
    FL_WHEN_ENTER_KEY         = 8,
    FL_WHEN_ENTER_KEY_ALWAYS  = 10,
    FL_WHEN_ENTER_KEY_CHANGED = 11
};

/* ------------------------------------------------------------------ */
/* Label alignment                                                     */
/* ------------------------------------------------------------------ */

#define FL_ALIGN_CENTER            ((Fl_Align)0)
#define FL_ALIGN_TOP               ((Fl_Align)1)
#define FL_ALIGN_BOTTOM            ((Fl_Align)2)
#define FL_ALIGN_LEFT              ((Fl_Align)4)
#define FL_ALIGN_RIGHT             ((Fl_Align)8)
#define FL_ALIGN_INSIDE            ((Fl_Align)16)
#define FL_ALIGN_TEXT_OVER_IMAGE   ((Fl_Align)0x0020)
#define FL_ALIGN_IMAGE_OVER_TEXT   ((Fl_Align)0x0000)
#define FL_ALIGN_CLIP              ((Fl_Align)64)
#define FL_ALIGN_WRAP              ((Fl_Align)128)
#define FL_ALIGN_IMAGE_NEXT_TO_TEXT ((Fl_Align)0x0100)
#define FL_ALIGN_TEXT_NEXT_TO_IMAGE ((Fl_Align)0x0120)
#define FL_ALIGN_IMAGE_BACKDROP    ((Fl_Align)0x0200)
#define FL_ALIGN_TOP_LEFT          (FL_ALIGN_TOP | FL_ALIGN_LEFT)
#define FL_ALIGN_TOP_RIGHT         (FL_ALIGN_TOP | FL_ALIGN_RIGHT)
#define FL_ALIGN_BOTTOM_LEFT       (FL_ALIGN_BOTTOM | FL_ALIGN_LEFT)
#define FL_ALIGN_BOTTOM_RIGHT      (FL_ALIGN_BOTTOM | FL_ALIGN_RIGHT)
#define FL_ALIGN_LEFT_TOP          ((Fl_Align)0x0007)
#define FL_ALIGN_RIGHT_TOP         ((Fl_Align)0x000b)
#define FL_ALIGN_LEFT_BOTTOM       ((Fl_Align)0x000d)
#define FL_ALIGN_RIGHT_BOTTOM      ((Fl_Align)0x000e)
#define FL_ALIGN_NOWRAP            ((Fl_Align)0)
#define FL_ALIGN_POSITION_MASK     ((Fl_Align)0x000f)
#define FL_ALIGN_IMAGE_MASK        ((Fl_Align)0x0320)

/* ------------------------------------------------------------------ */
/* Label types                                                         */
/* ------------------------------------------------------------------ */

enum Fl_Labeltype {
    FL_NORMAL_LABEL   = 0,
    FL_NO_LABEL       = 1,
    FL_SHADOW_LABEL   = 2,
    FL_ENGRAVED_LABEL = 3,
    FL_EMBOSSED_LABEL = 4,
    FL_MULTI_LABEL    = 5,
    FL_ICON_LABEL     = 6,
    FL_IMAGE_LABEL    = 7,
    FL_FREE_LABELTYPE = 8
};

/* ------------------------------------------------------------------ */
/* Box types (FL/Enumerations.H enum Fl_Boxtype)                       */
/* ------------------------------------------------------------------ */

enum Fl_Boxtype {
    FL_NO_BOX = 0,
    FL_FLAT_BOX,
    FL_UP_BOX,
    FL_DOWN_BOX,
    FL_UP_FRAME,
    FL_DOWN_FRAME,
    FL_THIN_UP_BOX,
    FL_THIN_DOWN_BOX,
    FL_THIN_UP_FRAME,
    FL_THIN_DOWN_FRAME,
    FL_ENGRAVED_BOX,
    FL_EMBOSSED_BOX,
    FL_ENGRAVED_FRAME,
    FL_EMBOSSED_FRAME,
    FL_BORDER_BOX,
    FL_SHADOW_BOX,
    FL_BORDER_FRAME,
    FL_SHADOW_FRAME,
    FL_ROUNDED_BOX,
    FL_RSHADOW_BOX,
    FL_ROUNDED_FRAME,
    FL_RFLAT_BOX,
    FL_ROUND_UP_BOX,
    FL_ROUND_DOWN_BOX,
    FL_DIAMOND_UP_BOX,
    FL_DIAMOND_DOWN_BOX,
    FL_OVAL_BOX,
    FL_OSHADOW_BOX,
    FL_OVAL_FRAME,
    FL_OFLAT_BOX,
    FL_PLASTIC_UP_BOX,
    FL_PLASTIC_DOWN_BOX,
    FL_PLASTIC_UP_FRAME,
    FL_PLASTIC_DOWN_FRAME,
    FL_PLASTIC_THIN_UP_BOX,
    FL_PLASTIC_THIN_DOWN_BOX,
    FL_PLASTIC_ROUND_UP_BOX,
    FL_PLASTIC_ROUND_DOWN_BOX,
    FL_GTK_UP_BOX,
    FL_GTK_DOWN_BOX,
    FL_GTK_UP_FRAME,
    FL_GTK_DOWN_FRAME,
    FL_GTK_THIN_UP_BOX,
    FL_GTK_THIN_DOWN_BOX,
    FL_GTK_THIN_UP_FRAME,
    FL_GTK_THIN_DOWN_FRAME,
    FL_GTK_ROUND_UP_BOX,
    FL_GTK_ROUND_DOWN_BOX,
    FL_GLEAM_UP_BOX,
    FL_GLEAM_DOWN_BOX,
    FL_GLEAM_UP_FRAME,
    FL_GLEAM_DOWN_FRAME,
    FL_GLEAM_THIN_UP_BOX,
    FL_GLEAM_THIN_DOWN_BOX,
    FL_GLEAM_ROUND_UP_BOX,
    FL_GLEAM_ROUND_DOWN_BOX,
    FL_FREE_BOXTYPE
};

/* Box-type family navigation, translated from the inline fl_box()/
 * fl_down()/fl_frame() helpers in FL/Enumerations.H. Every box family
 * occupies 4 consecutive enumerators: up-box, down-box, up-frame,
 * down-frame -- these just walk between them. Undefined (upstream:
 * "some random box or frame is returned") if the boxtype has no such
 * variant. */
static inline uchar fl_box(uchar b) { return (uchar)((b < FL_UP_BOX || b % 4 > 1) ? b : (b - 2)); }
static inline uchar fl_down(uchar b) { return (uchar)((b < FL_UP_BOX) ? b : (b | 1)); }
static inline uchar fl_frame(uchar b) { return (uchar)((b % 4 < 2) ? b : (b + 2)); }

#define FL_FRAME FL_ENGRAVED_FRAME
#define FL_FRAME_BOX FL_ENGRAVED_BOX
#define FL_CIRCLE_BOX FL_ROUND_DOWN_BOX
#define FL_DIAMOND_BOX FL_DIAMOND_DOWN_BOX

/* ------------------------------------------------------------------ */
/* Colors (FL/Enumerations.H color constants)                          */
/* ------------------------------------------------------------------ */

#define FL_FOREGROUND_COLOR  ((Fl_Color)0)
#define FL_BACKGROUND2_COLOR ((Fl_Color)7)
#define FL_INACTIVE_COLOR    ((Fl_Color)8)
#define FL_SELECTION_COLOR   ((Fl_Color)15)

#define FL_GRAY0             ((Fl_Color)32)
#define FL_DARK3             ((Fl_Color)39)
#define FL_DARK2             ((Fl_Color)45)
#define FL_DARK1             ((Fl_Color)47)
#define FL_BACKGROUND_COLOR  ((Fl_Color)49)
#define FL_LIGHT1            ((Fl_Color)50)
#define FL_LIGHT2            ((Fl_Color)52)
#define FL_LIGHT3            ((Fl_Color)54)

#define FL_BLACK             ((Fl_Color)56)
#define FL_RED               ((Fl_Color)88)
#define FL_GREEN             ((Fl_Color)63)
#define FL_YELLOW            ((Fl_Color)95)
#define FL_BLUE              ((Fl_Color)216)
#define FL_MAGENTA           ((Fl_Color)248)
#define FL_CYAN              ((Fl_Color)223)
#define FL_DARK_RED          ((Fl_Color)72)

#define FL_DARK_GREEN        ((Fl_Color)60)
#define FL_DARK_YELLOW       ((Fl_Color)76)
#define FL_DARK_BLUE         ((Fl_Color)136)
#define FL_DARK_MAGENTA      ((Fl_Color)152)
#define FL_DARK_CYAN         ((Fl_Color)140)

#define FL_WHITE             ((Fl_Color)255)

#define FL_FREE_COLOR      ((Fl_Color)16)
#define FL_NUM_FREE_COLOR  16
#define FL_GRAY_RAMP       ((Fl_Color)32)
#define FL_NUM_GRAY        24
#define FL_GRAY            FL_BACKGROUND_COLOR
#define FL_COLOR_CUBE      ((Fl_Color)56)
#define FL_NUM_RED         5
#define FL_NUM_GREEN       8
#define FL_NUM_BLUE        5

/** Packs 8-bit r,g,b into a 24-bit-plus-flag Fl_Color exactly like fl_rgb_color(). */
static inline Fl_Color fl_rgb_color(uchar r, uchar g, uchar b) {
    if (!r && !g && !b) return FL_BLACK;
    return (Fl_Color)(((((r << 8) | g) << 8) | b) << 8);
}

/** Index (0..FL_NUM_GRAY-1) into the reserved gray ramp; matches upstream's public fl_gray_ramp(int). */
static inline Fl_Color fl_gray_ramp(int i) { return (Fl_Color)(i + FL_GRAY_RAMP); }

/** Index into the reserved 5x8x5 color cube (r in 0..FL_NUM_RED-1, etc.); matches upstream's public fl_color_cube(int,int,int). */
static inline Fl_Color fl_color_cube(int r, int g, int b) {
    return (Fl_Color)((b * FL_NUM_RED + r) * FL_NUM_GREEN + g + FL_COLOR_CUBE);
}

static inline Fl_Color fl_gray_color(uchar g) {
    if (!g) return FL_BLACK;
    if (g == 0xff) return FL_WHITE;
    return (Fl_Color)(((((g << 8) | g) << 8) | g) << 8);
}

/* ------------------------------------------------------------------ */
/* Fonts                                                               */
/* ------------------------------------------------------------------ */

#define FL_HELVETICA               0
#define FL_HELVETICA_BOLD          1
#define FL_HELVETICA_ITALIC        2
#define FL_HELVETICA_BOLD_ITALIC   3
#define FL_COURIER                 4
#define FL_COURIER_BOLD            5
#define FL_COURIER_ITALIC          6
#define FL_COURIER_BOLD_ITALIC     7
#define FL_TIMES                   8
#define FL_TIMES_BOLD              9
#define FL_TIMES_ITALIC            10
#define FL_TIMES_BOLD_ITALIC       11
#define FL_SYMBOL                  12
#define FL_SCREEN                  13
#define FL_SCREEN_BOLD             14
#define FL_ZAPF_DINGBATS           15
#define FL_FREE_FONT               16
#define FL_BOLD                    1
#define FL_ITALIC                  2
#define FL_NORMAL_SIZE             14

/* ------------------------------------------------------------------ */
/* Common keysyms subset (enough for text/menu shortcut handling)      */
/* ------------------------------------------------------------------ */

#define FL_Escape        0xff1b
#define FL_BackSpace     0xff08
#define FL_Tab           0xff09
#define FL_Enter         0xff0d
#define FL_Pause         0xff13
#define FL_Insert        0xff63
#define FL_Home          0xff50
#define FL_Left          0xff51
#define FL_Up            0xff52
#define FL_Right         0xff53
#define FL_Down          0xff54
#define FL_Page_Up       0xff55
#define FL_Page_Down     0xff56
#define FL_End           0xff57
#define FL_Delete        0xffff
#define FL_KP            0xff80  /**< One of the keypad numbers; use FL_KP + 'n' for digit n. */
#define FL_KP_Enter      0xff8d  /**< The enter key on the keypad. */
#define FL_KP_Last       0xffbd  /**< The last keypad key; use to range-check keypad. */
#define FL_Shift_L       0xffe1
#define FL_Shift_R       0xffe2
#define FL_Control_L     0xffe3
#define FL_Control_R     0xffe4
#define FL_CapsLock      0xffe5
#define FL_Alt_L         0xffe9
#define FL_Alt_R         0xffea

/* Mouse buttons, as returned by Fl_event_button(). */
#define FL_LEFT_MOUSE    1
#define FL_MIDDLE_MOUSE  2
#define FL_RIGHT_MOUSE   3

/* Fl::event_state() bit masks */
#define FL_SHIFT     0x00010000
#define FL_CAPS_LOCK 0x00020000
#define FL_CTRL      0x00040000
#define FL_ALT       0x00080000
#define FL_NUM_LOCK  0x00100000
#define FL_META      0x00400000
#define FL_SCROLL_LOCK 0x00800000
#define FL_BUTTON1   0x01000000
#define FL_BUTTON2   0x02000000
#define FL_BUTTON3   0x04000000
#define FL_BUTTONS   0x7f000000
#define FL_KEY_MASK  0x0000ffff
/* Upstream aliases FL_COMMAND to FL_META on macOS, FL_CTRL everywhere
 * else (WIN32/X11) -- cfltk only targets the latter. */
#define FL_COMMAND   FL_CTRL

/* Standard cursor shapes, for Fl_Window_set_cursor() (see Fl_Window.h).
 * Same names/values as upstream FLTK's Fl_Cursor enum, mapped to X11's
 * standard cursor font glyphs (XC_* in <X11/cursorfont.h>) in the X11
 * backend (fl_x11_window.c) - not a 1:1 numeric mapping to XC_* indices,
 * just upstream's own arbitrary enumeration, kept for source
 * compatibility with code written against real FLTK. */
typedef enum {
    FL_CURSOR_DEFAULT = 0,
    FL_CURSOR_ARROW   = 35,
    FL_CURSOR_CROSS   = 66,
    FL_CURSOR_WAIT    = 76,
    FL_CURSOR_INSERT  = 77,
    FL_CURSOR_HAND    = 31,
    FL_CURSOR_HELP    = 47,
    FL_CURSOR_MOVE    = 27,
    FL_CURSOR_NS      = 78,
    FL_CURSOR_WE      = 79,
    FL_CURSOR_NWSE    = 80,
    FL_CURSOR_NESW    = 81,
    FL_CURSOR_N       = 70,
    FL_CURSOR_NE      = 69,
    FL_CURSOR_E       = 49,
    FL_CURSOR_SE      = 8,
    FL_CURSOR_S       = 9,
    FL_CURSOR_SW      = 7,
    FL_CURSOR_W       = 36,
    FL_CURSOR_NW      = 68,
    FL_CURSOR_NONE    = 255
} Fl_Cursor;

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_ENUMERATIONS_H */
