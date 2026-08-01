/*
 * cfltk example: menus
 *
 * Not a direct upstream port -- exercises Fl_Menu_Bar (with a nested
 * submenu and a radio group), Fl_Menu_Button (right-click context
 * menu), and Fl_Choice, all wired to a status Fl_Output so picks are
 * observable without reading pixels.
 */
#include <stdio.h>

#include "cfltk/Fl.h"
#include "cfltk/Fl_Window.h"
#include "cfltk/Fl_Box.h"
#include "cfltk/Fl_Input.h"
#include "cfltk/Fl_Output.h"
#include "cfltk/Fl_Menu_Bar.h"
#include "cfltk/Fl_Menu_Button.h"
#include "cfltk/Fl_Choice.h"

static Fl_Input *status;

static void report(const char *what) {
    Fl_Input_set_value_str(status, what);
}

static void menu_cb(Fl_Widget *w, void *data) {
    Fl_Menu_ *m = (Fl_Menu_ *)w;
    const Fl_Menu_Item *item = Fl_Menu_mvalue(m);
    (void)data;
    report(item ? Fl_Menu_Item_label(item) : "(none)");
}

static Fl_Menu_Item menubar_items[] = {
    {"&File", 0, NULL, NULL, FL_SUBMENU},
        {"&New",  FL_CTRL + 'n', menu_cb},
        {"&Open", FL_CTRL + 'o', menu_cb},
        {"&Save", FL_CTRL + 's', menu_cb, NULL, FL_MENU_DIVIDER},
        {"E&xit", FL_CTRL + 'q', menu_cb},
        {0},
    {"&Edit", 0, NULL, NULL, FL_SUBMENU},
        {"&Copy",  FL_CTRL + 'c', menu_cb},
        {"&Paste", FL_CTRL + 'v', menu_cb, NULL, FL_MENU_DIVIDER},
        {"&Preferences...", 0, NULL, NULL, FL_SUBMENU},
            {"Small font",  0, menu_cb, NULL, FL_MENU_RADIO | FL_MENU_VALUE},
            {"Medium font", 0, menu_cb, NULL, FL_MENU_RADIO},
            {"Large font",  0, menu_cb, NULL, FL_MENU_RADIO},
            {0},
        {0},
    {"&View", 0, NULL, NULL, FL_SUBMENU},
        {"Show &Toolbar", 0, menu_cb, NULL, FL_MENU_TOGGLE | FL_MENU_VALUE},
        {"Show &Status Bar", 0, menu_cb, NULL, FL_MENU_TOGGLE},
        {0},
    {0}
};

static Fl_Menu_Item context_items[] = {
    {"Cut",   0, menu_cb},
    {"Copy",  0, menu_cb},
    {"Paste", 0, menu_cb, NULL, FL_MENU_DIVIDER},
    {"Delete", 0, menu_cb, NULL, FL_MENU_INACTIVE},
    {0}
};

static Fl_Menu_Item choice_items[] = {
    {"Red", 0, menu_cb},
    {"Green", 0, menu_cb},
    {"Blue", 0, menu_cb},
    {0}
};

int main(void) {
    Fl_Window *window = Fl_Window_new(0, 0, 400, 220, NULL);

    Fl_Menu_ *bar = Fl_Menu_Bar_new(0, 0, 400, 25, NULL);
    Fl_Menu_set_menu(bar, menubar_items);

    Fl_Menu_ *ctx = Fl_Menu_Button_new(20, 50, 150, 30, "Right-click me");
    Fl_Widget_set_type(&ctx->widget, 0); /* box set -> left click pulldown for this demo */
    Fl_Menu_set_menu(ctx, context_items);
    Fl_Widget_set_callback(&ctx->widget, menu_cb, NULL);

    Fl_Menu_ *choice = Fl_Choice_new(20, 90, 150, 25, NULL);
    Fl_Menu_set_menu(choice, choice_items);
    Fl_Widget_set_callback(&choice->widget, menu_cb, NULL);
    Fl_Choice_set_value(choice, 0);

    status = Fl_Output_new(20, 150, 360, 25, NULL);
    Fl_Input_set_value_str(status, "Pick a menu item");

    Fl_Group_end(&window->group);
    Fl_Widget_show(FL_WIDGET(window));

    return Fl_run();
}
