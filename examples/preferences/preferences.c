/*
 * cfltk example: preferences
 * Exercises Fl_Preferences: remembers a window's position/size (and a
 * click counter) across runs, stored at ~/.fltk/cfltk/preferences_demo.prefs.
 */
#include <stdio.h>
#include "cfltk/Fl.h"
#include "cfltk/Fl_Window.h"
#include "cfltk/Fl_Button.h"
#include "cfltk/Fl_Preferences.h"

static Fl_Preferences g_prefs;
static Fl_Preferences g_win_prefs;
static int g_clicks = 0;

static void click_cb(Fl_Widget *w, void *data) {
    (void)w; (void)data;
    g_clicks++;
    Fl_Preferences_set_int(&g_prefs, "clicks", g_clicks);
    Fl_Preferences_flush(&g_prefs);
    printf("clicks so far: %d\n", g_clicks);
    fflush(stdout);
}

int main(void) {
    int x, y, w, h;
    Fl_Window *window;
    Fl_Button *b;

    Fl_Preferences_init_root(&g_prefs, FL_PREFERENCES_USER, "cfltk", "preferences_demo");
    Fl_Preferences_init_group(&g_win_prefs, &g_prefs, "MainWindow");

    Fl_Preferences_get_int(&g_win_prefs, "x", &x, 50);
    Fl_Preferences_get_int(&g_win_prefs, "y", &y, 50);
    Fl_Preferences_get_int(&g_win_prefs, "w", &w, 300);
    Fl_Preferences_get_int(&g_win_prefs, "h", &h, 150);
    Fl_Preferences_get_int(&g_prefs, "clicks", &g_clicks, 0);

    printf("starting at %d,%d %dx%d with %d prior clicks\n", x, y, w, h, g_clicks);

    window = Fl_Window_new(x, y, w, h, "cfltk preferences");
    b = Fl_Button_new(20, 20, w - 40, 40, "Click me");
    Fl_Widget_set_callback(&b->widget, click_cb, NULL);
    Fl_Group_end(&window->group);
    Fl_Widget_show(FL_WIDGET(window));

    Fl_run();

    Fl_Preferences_set_int(&g_win_prefs, "x", FL_WIDGET(window)->x);
    Fl_Preferences_set_int(&g_win_prefs, "y", FL_WIDGET(window)->y);
    Fl_Preferences_set_int(&g_win_prefs, "w", FL_WIDGET(window)->w);
    Fl_Preferences_set_int(&g_win_prefs, "h", FL_WIDGET(window)->h);
    Fl_Preferences_destroy(&g_prefs);

    return 0;
}
