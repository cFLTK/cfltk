/*
 * cfltk example: shared_image
 *
 * Exercises Fl_Shared_Image's caching/refcounting: get()-ing the same
 * file by name twice returns the identical cached object with its
 * refcount bumped (not a fresh decode), get()-ing it again at a
 * different size adds a second, independently-cached resized entry,
 * and release() drops the refcount back down. Also exercises
 * fl_register_images()'s magic-byte format dispatch (this loads a
 * plain ".bin"-named file that is actually a BMP -- get() has to
 * sniff the content, not trust the extension).
 */
#include <stdio.h>
#include <stdlib.h>

#include "cfltk/Fl.h"
#include "cfltk/Fl_Window.h"
#include "cfltk/Fl_Box.h"
#include "cfltk/Fl_Shared_Image.h"

static void put_le16(FILE *f, unsigned v) { fputc((int)(v & 0xff), f); fputc((int)((v >> 8) & 0xff), f); }
static void put_le32(FILE *f, unsigned v) {
    fputc((int)(v & 0xff), f); fputc((int)((v >> 8) & 0xff), f);
    fputc((int)((v >> 16) & 0xff), f); fputc((int)((v >> 24) & 0xff), f);
}

/* Same 2x2 24-bit BMP generator as examples/loaders, named ".bin" on
 * purpose to prove fl_register_images() dispatches by content, not by
 * file extension. */
static void generate_bmp(const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) return;
    fputc('B', f); fputc('M', f);
    put_le32(f, 70); put_le32(f, 0); put_le32(f, 54);
    put_le32(f, 40); put_le32(f, 2); put_le32(f, 2);
    put_le16(f, 1); put_le16(f, 24);
    put_le32(f, 0); put_le32(f, 0); put_le32(f, 0); put_le32(f, 0); put_le32(f, 0); put_le32(f, 0);
    fputc(255, f); fputc(0, f); fputc(0, f);
    fputc(255, f); fputc(255, f); fputc(255, f);
    fputc(0, f); fputc(0, f);
    fputc(0, f); fputc(0, f); fputc(255, f);
    fputc(0, f); fputc(255, f); fputc(0, f);
    fputc(0, f); fputc(0, f);
    fclose(f);
}

int main(void) {
    Fl_Window *window;
    Fl_Box *box1, *box2, *lbl1, *lbl2;
    Fl_Box *status[4];
    Fl_Shared_Image *img1, *img2, *img3;
    char line[4][128];
    int i;
    const char *path = "/tmp/cfltk_shared_test.bin";

    generate_bmp(path);
    fl_register_images();

    img1 = Fl_Shared_Image_get(path, 0, 0);
    img2 = Fl_Shared_Image_get(path, 0, 0);
    img3 = Fl_Shared_Image_get(path, 40, 40);

    sprintf(line[0], "same object: %s (refcount=%d)", img1 == img2 ? "yes" : "NO", Fl_Shared_Image_refcount(img1));
    sprintf(line[1], "resized copy is different object: %s", img3 != img1 ? "yes" : "NO");
    sprintf(line[2], "cache size before release: %d", Fl_Shared_Image_num_images());
    Fl_Shared_Image_release(img2);
    sprintf(line[3], "after releasing one ref: refcount=%d, cache size=%d", Fl_Shared_Image_refcount(img1), Fl_Shared_Image_num_images());

    window = Fl_Window_new(0, 0, 560, 220, "cfltk shared_image");

    lbl1 = Fl_Box_new(20, 10, 200, 20, "get(path, 0, 0) [original]");
    Fl_Widget_set_align(&lbl1->widget, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    box1 = Fl_Box_new(20, 35, 60, 60, NULL);
    Fl_Widget_set_box(&box1->widget, FL_DOWN_BOX);
    Fl_Widget_set_image(&box1->widget, &img1->image);

    lbl2 = Fl_Box_new(140, 10, 200, 20, "get(path, 40, 40) [resized]");
    Fl_Widget_set_align(&lbl2->widget, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    box2 = Fl_Box_new(140, 35, 60, 60, NULL);
    Fl_Widget_set_box(&box2->widget, FL_DOWN_BOX);
    Fl_Widget_set_image(&box2->widget, &img3->image);

    for (i = 0; i < 4; i++) {
        status[i] = Fl_Box_new(20, 130 + i * 22, 520, 20, NULL);
        Fl_Widget_set_align(&status[i]->widget, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
        Fl_Widget_copy_label(&status[i]->widget, line[i]);
    }

    Fl_Group_end(&window->group);
    Fl_Widget_show(FL_WIDGET(window));

    return Fl_run();
}
