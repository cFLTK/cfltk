/*
 * cfltk example: images
 *
 * Exercises the Fl_Image/Fl_RGB_Image/Fl_Pixmap/Fl_Bitmap family:
 *  - an Fl_RGB_Image built from a synthetic RGB gradient, shown as a
 *    box's image() label, plus a desaturate()'d and a color_average()'d
 *    copy side by side to exercise those two Fl_Image virtuals;
 *  - an Fl_Pixmap built from inline XPM data (color table + binary
 *    "None" transparency), shown as a button's image with a text
 *    label underneath (FL_ALIGN_TEXT_OVER_IMAGE combined label);
 *  - an Fl_Bitmap built from inline XBM-style 1-bit data, shown as a
 *    box's image, drawn in the widget's labelcolor().
 */
#include <stdlib.h>

#include "cfltk/Fl.h"
#include "cfltk/Fl_Window.h"
#include "cfltk/Fl_Box.h"
#include "cfltk/Fl_Button.h"
#include "cfltk/Fl_RGB_Image.h"
#include "cfltk/Fl_Pixmap.h"
#include "cfltk/Fl_Bitmap.h"

#define GRAD_W 96
#define GRAD_H 64

static uchar gradient[GRAD_H][GRAD_W][3];

static void make_gradient(void) {
    int x, y;
    for (y = 0; y < GRAD_H; y++) {
        for (x = 0; x < GRAD_W; x++) {
            gradient[y][x][0] = (uchar)(255 * x / (GRAD_W - 1));
            gradient[y][x][1] = (uchar)(255 * y / (GRAD_H - 1));
            gradient[y][x][2] = 128;
        }
    }
}

static const char *diamond_xpm[] = {
    "16 16 3 1",
    "  c None",
    ". c #802000",
    "X c #FF6020",
    "        ..      ",
    "       .XX.     ",
    "      .XXXX.    ",
    "     .XXXXXX.   ",
    "    .XXXXXXXX.  ",
    "   .XXXXXXXXXX. ",
    "  .XXXXXXXXXXXX.",
    " .XXXXXXXXXXXXX.",
    " .XXXXXXXXXXXXX.",
    "  .XXXXXXXXXXXX.",
    "   .XXXXXXXXXX. ",
    "    .XXXXXXXX.  ",
    "     .XXXXXX.   ",
    "      .XXXX.    ",
    "       .XX.     ",
    "        ..      "
};

/* 16x16 1-bit checkerboard, XBM row layout: (16+7)/8 = 2 bytes/row,
 * LSB first. */
static uchar checker_bits[16 * 2];

static void make_checker(void) {
    int x, y;
    for (y = 0; y < 16; y++) {
        for (x = 0; x < 16; x++) {
            int on = ((x / 2) + (y / 2)) & 1;
            if (on) checker_bits[y * 2 + x / 8] |= (uchar)(1 << (x % 8));
        }
    }
}

int main(void) {
    Fl_Window *window = Fl_Window_new(0, 0, 640, 340, "cfltk images");
    Fl_Box *b1, *b2, *b3, *lbl1, *lbl2, *lbl3, *lbl4, *lbl5, *bmbox;
    Fl_Button *btn;
    Fl_RGB_Image *rgb, *rgb_desat, *rgb_avg;
    Fl_Pixmap *pxm;
    Fl_Bitmap *bmp;

    make_gradient();
    make_checker();

    lbl1 = Fl_Box_new(20, 10, 110, 20, "gradient");
    Fl_Widget_set_align(&lbl1->widget, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    b1 = Fl_Box_new(20, 35, GRAD_W, GRAD_H, NULL);
    Fl_Widget_set_box(&b1->widget, FL_DOWN_BOX);
    rgb = Fl_RGB_Image_new(&gradient[0][0][0], GRAD_W, GRAD_H, 3, 0);
    Fl_Widget_set_image(&b1->widget, &rgb->image);

    lbl2 = Fl_Box_new(140, 10, 110, 20, "desaturate()");
    Fl_Widget_set_align(&lbl2->widget, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    b2 = Fl_Box_new(140, 35, GRAD_W, GRAD_H, NULL);
    Fl_Widget_set_box(&b2->widget, FL_DOWN_BOX);
    rgb_desat = (Fl_RGB_Image *)Fl_RGB_Image_copy(rgb, GRAD_W, GRAD_H);
    Fl_RGB_Image_desaturate(rgb_desat);
    Fl_Widget_set_image(&b2->widget, &rgb_desat->image);

    lbl3 = Fl_Box_new(260, 10, 130, 20, "color_avg(RED)");
    Fl_Widget_set_align(&lbl3->widget, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    b3 = Fl_Box_new(260, 35, GRAD_W, GRAD_H, NULL);
    Fl_Widget_set_box(&b3->widget, FL_DOWN_BOX);
    rgb_avg = (Fl_RGB_Image *)Fl_RGB_Image_copy(rgb, GRAD_W, GRAD_H);
    Fl_RGB_Image_color_average(rgb_avg, FL_RED, 0.5f);
    Fl_Widget_set_image(&b3->widget, &rgb_avg->image);

    lbl4 = Fl_Box_new(400, 10, 130, 20, "Fl_Pixmap");
    Fl_Widget_set_align(&lbl4->widget, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    btn = Fl_Button_new(400, 35, 90, 90, "Diamond");
    Fl_Widget_set_align(&btn->widget, FL_ALIGN_TEXT_OVER_IMAGE | FL_ALIGN_INSIDE);
    pxm = Fl_Pixmap_new(diamond_xpm);
    Fl_Widget_set_image(&btn->widget, &pxm->image);

    lbl5 = Fl_Box_new(520, 10, 130, 20, "Fl_Bitmap");
    Fl_Widget_set_align(&lbl5->widget, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    bmbox = Fl_Box_new(520, 35, 60, 60, NULL);
    Fl_Widget_set_box(&bmbox->widget, FL_DOWN_BOX);
    Fl_Widget_set_labelcolor(&bmbox->widget, FL_DARK_BLUE);
    bmp = Fl_Bitmap_new(checker_bits, 16, 16);
    Fl_Widget_set_image(&bmbox->widget, &bmp->image);

    Fl_Group_end(&window->group);
    Fl_Widget_show(FL_WIDGET(window));

    return Fl_run();
}
