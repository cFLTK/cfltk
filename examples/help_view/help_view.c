/*
 * cfltk example: help_view
 * Exercises Fl_Help_View embedded directly (with inline HTML covering
 * headings, lists, links, character styling, and named colors) plus
 * Fl_Help_Dialog (a back/forward-navigable popup loading page1.html /
 * page2.html from disk). Run from the repository root so the relative
 * paths resolve, e.g. `./build/help_view`.
 */
#include <stdio.h>
#include "cfltk/Fl.h"
#include "cfltk/Fl_Window.h"
#include "cfltk/Fl_Button.h"
#include "cfltk/Fl_Input.h"
#include "cfltk/Fl_Help_View.h"
#include "cfltk/Fl_Help_Dialog.h"

static const char *sample_html =
    "<HTML><HEAD><TITLE>Embedded Sample</TITLE></HEAD><BODY>"
    "<H1>Fl_Help_View</H1>"
    "<P>A small HTML subset renderer: <B>bold</B>, <I>italic</I>, "
    "<U>underline</U>, and <TT>monospace</TT> text.</P>"
    "<H2>Lists</H2>"
    "<UL>"
    "<LI>Headings H1-H6</LI>"
    "<LI>Ordered and unordered lists</LI>"
    "<LI><A HREF=\"#bottom\">A link to a named anchor below</A></LI>"
    "</UL>"
    "<HR>"
    "<P>Search for the word <TT>needle</TT> using the box below: this "
    "paragraph hides a needle in it for the Find button to locate.</P>"
    "<P><A NAME=\"bottom\">You scrolled to the bottom anchor.</A></P>"
    /* <FONT COLOR> mutates the view's running default text color and
     * only resets on the next format() pass -- not on </FONT> and not
     * between separate draw() calls (see Fl_Help_View.c's note by the
     * FONT tag handler). So this demo puts the colored words last and
     * closes with an explicit reset to black, or every later redraw of
     * this same document would come out navy. */
    "<P>Some named colors: <FONT COLOR=\"red\">red</FONT>, "
    "<FONT COLOR=\"green\">green</FONT>, <FONT COLOR=\"navy\">navy</FONT>.</P>"
    "<FONT COLOR=\"black\">"
    "</BODY></HTML>";

static Fl_Help_View *g_view;
static Fl_Input *g_find_input;
static Fl_Help_Dialog *g_dialog = NULL;

static void find_cb(Fl_Widget *w, void *data) {
    (void)w;
    (void)data;
    int pos = Fl_Help_View_find(g_view, Fl_Input_value(g_find_input), 0);
    printf("find(\"%s\") -> %d\n", Fl_Input_value(g_find_input), pos);
    fflush(stdout);
}

static void dialog_cb(Fl_Widget *w, void *data) {
    (void)w;
    (void)data;
    if (!g_dialog) {
        g_dialog = Fl_Help_Dialog_new();
        Fl_Help_Dialog_load(g_dialog, "examples/help_view/page1.html");
    }
    Fl_Help_Dialog_show(g_dialog);
}

int main(void) {
    Fl_Window *window = Fl_Window_new(0, 0, 640, 480, "cfltk help_view");

    g_view = Fl_Help_View_new(10, 10, 620, 400, NULL);
    Fl_Widget_set_box(FL_WIDGET(g_view), FL_DOWN_BOX);
    Fl_Help_View_set_value(g_view, sample_html);

    g_find_input = Fl_Input_new(10, 420, 300, 25, NULL);
    Fl_Input_set_value_str(g_find_input, "needle");

    Fl_Button *find_btn = Fl_Button_new(320, 420, 90, 25, "Find");
    Fl_Widget_set_callback(&find_btn->widget, find_cb, NULL);

    Fl_Button *dialog_btn = Fl_Button_new(420, 420, 210, 25, "Open Fl_Help_Dialog...");
    Fl_Widget_set_callback(&dialog_btn->widget, dialog_cb, NULL);

    Fl_Group_end(&window->group);
    Fl_Widget_show(FL_WIDGET(window));

    return Fl_run();
}
