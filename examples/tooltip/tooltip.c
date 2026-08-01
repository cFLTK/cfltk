/*
 * cfltk example: tooltip
 * Exercises Fl_Tooltip: a plain tooltip, a multi-line tooltip (explicit
 * '\n'), and a widget with no tooltip that falls back to its parent
 * group's tooltip.
 */
#include "cfltk/Fl.h"
#include "cfltk/Fl_Window.h"
#include "cfltk/Fl_Button.h"
#include "cfltk/Fl_Group.h"
#include "cfltk/Fl_Tooltip.h"

int main(void) {
    Fl_Window *window = Fl_Window_new(0, 0, 320, 160, "cfltk tooltip");
    Fl_Button *b1, *b2, *b3;
    Fl_Group *g;

    /* Short delay so the example is easy to drive interactively/from a
     * test script without waiting a full second. */
    Fl_Tooltip_set_delay(0.3f);

    b1 = Fl_Button_new(10, 10, 130, 30, "One-line");
    Fl_Widget_set_tooltip(&b1->widget, "A simple tooltip.");

    b2 = Fl_Button_new(150, 10, 160, 30, "Multi-line");
    Fl_Widget_set_tooltip(&b2->widget, "Line one\nLine two\nLine three");

    g = Fl_Group_new(10, 60, 300, 60, NULL);
    Fl_Widget_set_tooltip(&g->widget, "Inherited from the group.");
    b3 = Fl_Button_new(20, 75, 130, 30, "Inherits");
    Fl_Group_end(g);

    Fl_Group_end(&window->group);
    Fl_Widget_show(FL_WIDGET(window));

    (void)b3;
    return Fl_run();
}
