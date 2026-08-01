/*
 * cfltk example: tabs_scroll
 *
 * Not a direct upstream port -- exercises Fl_Tabs (three card tabs,
 * switched by click/keyboard, reported via callback) and Fl_Scroll
 * (a viewport onto a grid of boxes much larger than its own size, both
 * scrollbars appearing automatically).
 */
#include <stdio.h>

#include "cfltk/Fl.h"
#include "cfltk/Fl_Window.h"
#include "cfltk/Fl_Box.h"
#include "cfltk/Fl_Button.h"
#include "cfltk/Fl_Output.h"
#include "cfltk/Fl_Tabs.h"
#include "cfltk/Fl_Scroll.h"

static Fl_Input *status;

static void report(const char *what) {
    Fl_Input_set_value_str(status, what);
}

static void tabs_cb(Fl_Widget *w, void *data) {
    Fl_Tabs *tabs = (Fl_Tabs *)w;
    Fl_Widget *v = Fl_Tabs_value(tabs);
    char buf[128];
    (void)data;
    snprintf(buf, sizeof(buf), "tab changed to: %s", v ? Fl_Widget_label(v) : "(none)");
    report(buf);
}

static void button_cb(Fl_Widget *w, void *data) {
    (void)w;
    report((const char *)data);
}

int main(void) {
    Fl_Window *window = Fl_Window_new(0, 0, 520, 420, "cfltk tabs + scroll");

    Fl_Tabs *tabs = Fl_Tabs_new(10, 10, 500, 360, NULL);
    Fl_Widget_set_callback(&tabs->group.widget, tabs_cb, NULL);

    {
        Fl_Group *tab_a = Fl_Group_new(10, 35, 500, 335, "Scroll");
        Fl_Scroll *scroll = Fl_Scroll_new(20, 45, 480, 315, NULL);
        int row, col;
        for (row = 0; row < 12; row++) {
            for (col = 0; col < 8; col++) {
                char label[16];
                Fl_Box *b;
                snprintf(label, sizeof(label), "%d,%d", row, col);
                b = Fl_Box_new(20 + col * 100, 45 + row * 40, 90, 30, NULL);
                Fl_Widget_set_box(&b->widget, FL_UP_BOX);
                Fl_Widget_copy_label(&b->widget, label);
            }
        }
        Fl_Group_end(&scroll->group);
        Fl_Group_end(tab_a);
    }
    {
        Fl_Group *tab_b = Fl_Group_new(10, 35, 500, 335, "Buttons");
        Fl_Button *b1 = Fl_Button_new(40, 70, 150, 30, "Click me");
        Fl_Widget_set_callback(&b1->widget, button_cb, (void *)"clicked button in Tab B");
        Fl_Group_end(tab_b);
    }
    {
        Fl_Group *tab_c = Fl_Group_new(10, 35, 500, 335, "Info");
        Fl_Box *lbl = Fl_Box_new(40, 70, 400, 30, "This is the third tab.");
        Fl_Widget_set_align(&lbl->widget, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
        Fl_Group_end(tab_c);
    }

    Fl_Group_end(&tabs->group);

    status = Fl_Output_new(10, 385, 500, 25, NULL);
    Fl_Input_set_value_str(status, "Click a tab or drag the scrollbars");

    Fl_Group_end(&window->group);
    Fl_Widget_show(FL_WIDGET(window));

    return Fl_run();
}
