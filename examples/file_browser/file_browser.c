/*
 * cfltk example: file_browser
 *
 * Exercises Fl_File_Browser: load()-ing a real directory (with
 * directories sorted first and shown in bold), double-click
 * navigation into a directory, an "Up" button to go to the parent,
 * and load("")'s mounted-filesystems listing.
 */
#include <stdio.h>
#include <string.h>

#include "cfltk/Fl.h"
#include "cfltk/Fl_Window.h"
#include "cfltk/Fl_Box.h"
#include "cfltk/Fl_Button.h"
#include "cfltk/Fl_Input.h"
#include "cfltk/Fl_File_Browser.h"

static Fl_File_Browser *browser;
static Fl_Input *path_input;
static Fl_Input *status;
static char current_dir[4096] = "/tmp";

static void report(const char *fmt_prefix, const char *name) {
    char buf[256];
    snprintf(buf, sizeof(buf), "%s%s", fmt_prefix, name ? name : "");
    Fl_Input_set_value_str(status, buf);
}

static void load_dir(const char *dir) {
    int n;
    strncpy(current_dir, dir, sizeof(current_dir) - 1);
    current_dir[sizeof(current_dir) - 1] = '\0';
    n = Fl_File_Browser_load(browser, current_dir, NULL);
    Fl_Input_set_value_str(path_input, current_dir);
    report("loaded: ", current_dir);
    (void)n;
}

static void browser_cb(Fl_Widget *w, void *data) {
    Fl_File_Browser *b = (Fl_File_Browser *)w;
    int line = Fl_Browser_value(&b->browser);
    const char *text;
    (void)data;
    if (!line) return;
    text = Fl_Browser_text(&b->browser, line);
    if (!text) return;

    if (Fl_event_clicks() && text[strlen(text) - 1] == '/') {
        /* Double-click on a directory entry: descend into it. */
        char newdir[8192];
        if (strcmp(current_dir, "/") == 0) snprintf(newdir, sizeof(newdir), "/%s", text);
        else snprintf(newdir, sizeof(newdir), "%s/%s", current_dir, text);
        newdir[strlen(newdir) - 1] = '\0'; /* drop trailing '/' Fl_File_Browser adds */
        load_dir(newdir);
    } else {
        report("selected: ", text);
    }
}

static void up_cb(Fl_Widget *w, void *data) {
    char *slash;
    (void)w; (void)data;
    if (strcmp(current_dir, "/") == 0) return;
    slash = strrchr(current_dir, '/');
    if (slash == current_dir) slash[1] = '\0';
    else if (slash) *slash = '\0';
    load_dir(current_dir);
}

static void mounts_cb(Fl_Widget *w, void *data) {
    (void)w; (void)data;
    Fl_File_Browser_load(browser, "", NULL);
    Fl_Input_set_value_str(path_input, "(mounted filesystems)");
    report("loaded: ", "mounted filesystems");
}

int main(void) {
    Fl_Window *window = Fl_Window_new(0, 0, 480, 420, "cfltk file_browser");
    Fl_Button *up_btn, *mounts_btn;

    path_input = Fl_Input_new(20, 10, 340, 25, NULL);
    Fl_Input_set_readonly(path_input, 1);

    up_btn = Fl_Button_new(370, 10, 40, 25, "Up");
    Fl_Widget_set_callback(&up_btn->widget, up_cb, NULL);

    mounts_btn = Fl_Button_new(20, 45, 120, 25, "Mounts");
    Fl_Widget_set_callback(&mounts_btn->widget, mounts_cb, NULL);

    browser = Fl_File_Browser_new(20, 80, 440, 280, NULL);
    /* Plain Fl_Browser (which Fl_File_Browser doesn't change) defaults
     * to FL_NORMAL_BROWSER, which never selects on click -- that's
     * what FL_HOLD_BROWSER/FL_SELECT_BROWSER/FL_MULTI_BROWSER exist
     * for. Real applications set this explicitly. */
    Fl_Widget_set_type(&browser->browser.browser_.group.widget, FL_HOLD_BROWSER);
    /* FL_WHEN_ENTER_KEY is FLTK's real (if confusingly named) flag for
     * "also fire the callback on a double-click", separate from plain
     * single-click selection changes. */
    Fl_Widget_set_when(&browser->browser.browser_.group.widget, (uchar)(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY));
    Fl_Widget_set_callback(&browser->browser.browser_.group.widget, browser_cb, NULL);

    status = Fl_Input_new(20, 375, 440, 25, NULL);
    Fl_Input_set_readonly(status, 1);

    load_dir("/tmp/cfltk_test_dir");

    Fl_Group_end(&window->group);
    Fl_Widget_show(FL_WIDGET(window));

    return Fl_run();
}
