/*
 * cfltk example: layout
 *
 * Exercises Fl_Pack (a column of buttons packed tight, then the whole
 * pack resized to fit), Fl_Tile (two boxes with a draggable shared
 * border), and Fl_Wizard (two panes switched by Next/Prev buttons).
 */
#include "cfltk/Fl.h"
#include "cfltk/Fl_Window.h"
#include "cfltk/Fl_Box.h"
#include "cfltk/Fl_Button.h"
#include "cfltk/Fl_Pack.h"
#include "cfltk/Fl_Tile.h"
#include "cfltk/Fl_Wizard.h"

static Fl_Wizard *wiz;

static void next_cb(Fl_Widget *w, void *data) { (void)w; (void)data; Fl_Wizard_next(wiz); }
static void prev_cb(Fl_Widget *w, void *data) { (void)w; (void)data; Fl_Wizard_prev(wiz); }

int main(void) {
    Fl_Window *window = Fl_Window_new(0, 0, 640, 340, "cfltk layout");
    Fl_Box *lbl_pack, *lbl_tile, *lbl_wiz;
    Fl_Pack *pack;
    Fl_Button *b1, *b2, *b3;
    Fl_Tile *tile;
    Fl_Box *tile_left, *tile_right;
    Fl_Group *page1, *page2;
    Fl_Box *p1lbl, *p2lbl;
    Fl_Button *next_btn, *prev_btn;

    lbl_pack = Fl_Box_new(20, 10, 180, 20, "Fl_Pack (vertical)");
    Fl_Widget_set_align(&lbl_pack->widget, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    pack = Fl_Pack_new(20, 35, 160, 200, NULL);
    Fl_Pack_set_spacing(pack, 4);
    b1 = Fl_Button_new(0, 0, 160, 30, "One");
    b2 = Fl_Button_new(0, 0, 160, 30, "Two");
    b3 = Fl_Button_new(0, 0, 160, 30, "Three");
    (void)b1; (void)b2; (void)b3;
    Fl_Group_end(&pack->group);

    lbl_tile = Fl_Box_new(220, 10, 200, 20, "Fl_Tile (drag the border)");
    Fl_Widget_set_align(&lbl_tile->widget, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    tile = Fl_Tile_new(220, 35, 200, 200, NULL);
    tile_left = Fl_Box_new(220, 35, 100, 200, "Left");
    Fl_Widget_set_box(&tile_left->widget, FL_DOWN_BOX);
    Fl_Widget_set_color(&tile_left->widget, FL_LIGHT2);
    tile_right = Fl_Box_new(320, 35, 100, 200, "Right");
    Fl_Widget_set_box(&tile_right->widget, FL_DOWN_BOX);
    Fl_Widget_set_color(&tile_right->widget, FL_LIGHT3);
    Fl_Group_end(&tile->group);

    lbl_wiz = Fl_Box_new(440, 10, 180, 20, "Fl_Wizard");
    Fl_Widget_set_align(&lbl_wiz->widget, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    wiz = Fl_Wizard_new(440, 35, 180, 160, NULL);

    page1 = Fl_Group_new(440, 35, 180, 160, NULL);
    p1lbl = Fl_Box_new(440, 100, 180, 30, "Page 1");
    (void)p1lbl;
    Fl_Group_end(page1);

    page2 = Fl_Group_new(440, 35, 180, 160, NULL);
    p2lbl = Fl_Box_new(440, 100, 180, 30, "Page 2");
    Fl_Widget_set_labelcolor(&p2lbl->widget, FL_BLUE);
    Fl_Group_end(page2);

    Fl_Group_end(&wiz->group);

    prev_btn = Fl_Button_new(440, 200, 85, 25, "@< Prev");
    Fl_Widget_set_callback(&prev_btn->widget, prev_cb, NULL);
    next_btn = Fl_Button_new(535, 200, 85, 25, "Next @>");
    Fl_Widget_set_callback(&next_btn->widget, next_cb, NULL);

    Fl_Group_end(&window->group);
    Fl_Widget_show(FL_WIDGET(window));

    return Fl_run();
}
