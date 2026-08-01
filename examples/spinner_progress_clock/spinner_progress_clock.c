/*
 * cfltk example: spinner_progress_clock
 * Exercises Fl_Spinner, Fl_Progress, Fl_Clock (square + round).
 */
#include "cfltk/Fl.h"
#include "cfltk/Fl_Window.h"
#include "cfltk/Fl_Box.h"
#include "cfltk/Fl_Spinner.h"
#include "cfltk/Fl_Progress.h"
#include "cfltk/Fl_Clock.h"

static Fl_Progress *progress;

static void spinner_cb(Fl_Widget *w, void *data) {
    (void)data;
    Fl_Spinner *sp = (Fl_Spinner *)w;
    Fl_Progress_set_value(progress, (float)Fl_Spinner_value(sp));
}

int main(void) {
    Fl_Window *window = Fl_Window_new(0, 0, 420, 260, "cfltk spinner/progress/clock");

    Fl_Box_new(10, 10, 120, 20, "Fl_Spinner:");
    Fl_Spinner *sp = Fl_Spinner_new(140, 10, 80, 24, NULL);
    Fl_Spinner_range(sp, 0, 100);
    Fl_Spinner_set_value(sp, 0);
    Fl_Widget_set_callback(&sp->group.widget, spinner_cb, NULL);

    Fl_Box_new(10, 50, 120, 20, "Fl_Progress:");
    progress = Fl_Progress_new(140, 50, 260, 24, NULL);
    Fl_Progress_set_minimum(progress, 0);
    Fl_Progress_set_maximum(progress, 100);
    Fl_Progress_set_value(progress, 0);

    Fl_Box_new(10, 100, 150, 20, "Fl_Clock (square):");
    Fl_Clock_new(10, 130, 120, 120, NULL);

    Fl_Box_new(150, 100, 150, 20, "Fl_Round_Clock:");
    Fl_Round_Clock_new(150, 130, 120, 120, NULL);

    Fl_Group_end(&window->group);
    Fl_Widget_show(FL_WIDGET(window));

    return Fl_run();
}
