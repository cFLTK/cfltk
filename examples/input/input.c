/*
 * cfltk example: input
 *
 * Not an upstream port -- a regression check exercising Fl_Input: a
 * normal single-line field, a secret (password) field, a multiline
 * field, and an Fl_Int_Input, wired to a status Fl_Output that reports
 * value-changed callbacks (FL_WHEN_CHANGED) to make edits observable
 * without reading pixels.
 */
#include <stdio.h>

#include "cfltk/Fl.h"
#include "cfltk/Fl_Window.h"
#include "cfltk/Fl_Box.h"
#include "cfltk/Fl_Input.h"
#include "cfltk/Fl_Secret_Input.h"
#include "cfltk/Fl_Multiline_Input.h"
#include "cfltk/Fl_Int_Input.h"
#include "cfltk/Fl_Output.h"

static Fl_Input *status;

static void report(Fl_Widget *w, void *data) {
    static char buf[256];
    char shown[200];
    const char *tag = (const char *)data;
    const char *value = Fl_Input_value((Fl_Input *)w);
    size_t i;

    /* cfltk doesn't expand control characters for display (see
     * docs/DESIGN.md); a single-line status field showing a multiline
     * value needs to fold newlines itself, same as any real app would. */
    for (i = 0; i < sizeof(shown) - 1 && value[i]; i++)
        shown[i] = (value[i] == '\n') ? ' ' : value[i];
    shown[i] = '\0';

    snprintf(buf, sizeof(buf), "%s changed: \"%s\"", tag, shown);
    Fl_Input_set_value_str(status, buf);
}

int main(void) {
    Fl_Window *window = Fl_Window_new(0, 0, 360, 260, NULL);

    Fl_Box *l1 = Fl_Box_new(10, 10, 100, 25, "Name:");
    Fl_Widget_set_align(&l1->widget, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    Fl_Input *name = Fl_Input_new(110, 10, 240, 25, NULL);
    Fl_Widget_set_when(&name->widget, FL_WHEN_CHANGED);
    Fl_Widget_set_callback(&name->widget, report, (void *)"name");

    Fl_Box *l2 = Fl_Box_new(10, 45, 100, 25, "Password:");
    Fl_Widget_set_align(&l2->widget, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    Fl_Input *pw = Fl_Secret_Input_new(110, 45, 240, 25, NULL);
    Fl_Widget_set_when(&pw->widget, FL_WHEN_CHANGED);
    Fl_Widget_set_callback(&pw->widget, report, (void *)"password");

    Fl_Box *l3 = Fl_Box_new(10, 80, 100, 25, "Age:");
    Fl_Widget_set_align(&l3->widget, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    Fl_Input *age = Fl_Int_Input_new(110, 80, 100, 25, NULL);
    Fl_Widget_set_when(&age->widget, FL_WHEN_CHANGED);
    Fl_Widget_set_callback(&age->widget, report, (void *)"age");

    Fl_Box *l4 = Fl_Box_new(10, 115, 100, 25, "Notes:");
    Fl_Widget_set_align(&l4->widget, FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    Fl_Input *notes = Fl_Multiline_Input_new(110, 115, 240, 80, NULL);
    Fl_Widget_set_when(&notes->widget, FL_WHEN_CHANGED);
    Fl_Widget_set_callback(&notes->widget, report, (void *)"notes");

    status = Fl_Output_new(10, 210, 340, 25, NULL);

    Fl_Group_end(&window->group);
    Fl_Widget_show(FL_WIDGET(window));

    return Fl_run();
}
