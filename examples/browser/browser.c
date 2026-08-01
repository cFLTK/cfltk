/*
 * cfltk example: browser
 *
 * Not a direct upstream port -- exercises Fl_Hold_Browser (single-line
 * select, keyboard navigation) and Fl_Multi_Browser (multi-select via
 * Ctrl/Shift-click), including tab-separated columns and a few '@'
 * inline format codes (bold, color, alignment), wired to a status
 * Fl_Output so selections are observable without reading pixels.
 */
#include <stdio.h>

#include "cfltk/Fl.h"
#include "cfltk/Fl_Window.h"
#include "cfltk/Fl_Box.h"
#include "cfltk/Fl_Output.h"
#include "cfltk/Fl_Hold_Browser.h"
#include "cfltk/Fl_Multi_Browser.h"

static Fl_Input *status;

static void report(const char *what) {
    Fl_Input_set_value_str(status, what);
}

static void hold_cb(Fl_Widget *w, void *data) {
    Fl_Browser *b = (Fl_Browser *)w;
    char buf[128];
    int line = Fl_Browser_value(b);
    (void)data;
    snprintf(buf, sizeof(buf), "hold: line %d = \"%s\"", line, line ? Fl_Browser_text(b, line) : "(none)");
    report(buf);
}

static void multi_cb(Fl_Widget *w, void *data) {
    Fl_Browser *b = (Fl_Browser *)w;
    char buf[256];
    int i, n = 0, pos = 0;
    (void)data;
    pos += snprintf(buf + pos, sizeof(buf) - (size_t)pos, "multi selected:");
    for (i = 1; i <= Fl_Browser_size(b); i++) {
        if (Fl_Browser_selected(b, i)) {
            pos += snprintf(buf + pos, sizeof(buf) - (size_t)pos, " %d", i);
            n++;
        }
    }
    if (!n) snprintf(buf + pos, sizeof(buf) - (size_t)pos, " (none)");
    report(buf);
}

int main(void) {
    Fl_Window *window = Fl_Window_new(0, 0, 520, 420, "cfltk browser");
    static int widths[] = { 60, 60, 0 };

    Fl_Box *b1 = Fl_Box_new(20, 10, 200, 20, "Hold browser (columns + @ format codes)");
    Fl_Widget_set_align(&b1->widget, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    Fl_Browser *hold = (Fl_Browser *)Fl_Hold_Browser_new(20, 35, 480, 150, NULL);
    Fl_Browser_set_column_widths(hold, widths);
    Fl_Browser_add(hold, "@bName\t@bScore\t@bNote", NULL);
    Fl_Browser_add(hold, "Alice\t92\t@C1normal", NULL);
    Fl_Browser_add(hold, "Bob\t78\t@i@C4italic blue", NULL);
    Fl_Browser_add(hold, "Carol\t85\t@r@brightbold", NULL);
    Fl_Browser_add(hold, "Dave\t60\t@-strikethrough", NULL);
    Fl_Widget_set_callback(&hold->browser_.group.widget, hold_cb, NULL);

    Fl_Box *b2 = Fl_Box_new(20, 200, 200, 20, "Multi browser (Ctrl/Shift-click)");
    Fl_Widget_set_align(&b2->widget, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    Fl_Browser *multi = (Fl_Browser *)Fl_Multi_Browser_new(20, 225, 480, 150, NULL);
    Fl_Browser_add(multi, "Apples", NULL);
    Fl_Browser_add(multi, "Bananas", NULL);
    Fl_Browser_add(multi, "Cherries", NULL);
    Fl_Browser_add(multi, "Dates", NULL);
    Fl_Browser_add(multi, "Elderberries", NULL);
    Fl_Browser_add(multi, "Figs", NULL);
    Fl_Widget_set_callback(&multi->browser_.group.widget, multi_cb, NULL);

    status = Fl_Output_new(20, 385, 480, 25, NULL);
    Fl_Input_set_value_str(status, "Click a line in either browser");

    Fl_Group_end(&window->group);
    Fl_Widget_show(FL_WIDGET(window));

    return Fl_run();
}
