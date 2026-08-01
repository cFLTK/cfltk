/*
 * cfltk - Fl_Help_Dialog.c
 * See include/cfltk/Fl_Help_Dialog.h for the class-conversion notes.
 * Translated from src/Fl_Help_Dialog.cxx.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cfltk/Fl_Help_Dialog.h"
#include "cfltk/Fl.h"
#include "cfltk/Fl_Box.h"

static void cb_back(Fl_Widget *o, void *v) {
    Fl_Help_Dialog *self = (Fl_Help_Dialog *)v;
    int l;
    (void)o;

    if (self->index_ > 0) self->index_--;

    if (self->index_ == 0) Fl_Widget_deactivate(FL_WIDGET(self->back_));
    Fl_Widget_activate(FL_WIDGET(self->forward_));

    l = self->line_[self->index_];

    if (!Fl_Help_View_filename(self->view_) || strcmp(Fl_Help_View_filename(self->view_), self->file_[self->index_]) != 0)
        Fl_Help_View_load(self->view_, self->file_[self->index_]);

    Fl_Help_View_set_topline(self->view_, l);
}

static void cb_forward(Fl_Widget *o, void *v) {
    Fl_Help_Dialog *self = (Fl_Help_Dialog *)v;
    int l;
    (void)o;

    if (self->index_ < self->max_) self->index_++;

    if (self->index_ >= self->max_) Fl_Widget_deactivate(FL_WIDGET(self->forward_));
    Fl_Widget_activate(FL_WIDGET(self->back_));

    l = Fl_Help_View_topline(self->view_);

    if (!Fl_Help_View_filename(self->view_) || strcmp(Fl_Help_View_filename(self->view_), self->file_[self->index_]) != 0)
        Fl_Help_View_load(self->view_, self->file_[self->index_]);

    Fl_Help_View_set_topline(self->view_, l);
}

static void cb_smaller(Fl_Widget *o, void *v) {
    Fl_Help_Dialog *self = (Fl_Help_Dialog *)v;
    (void)o;

    if (Fl_Help_View_textsize(self->view_) > 8) Fl_Help_View_set_textsize(self->view_, Fl_Help_View_textsize(self->view_) - 2);

    if (Fl_Help_View_textsize(self->view_) <= 8) Fl_Widget_deactivate(FL_WIDGET(self->smaller_));
    Fl_Widget_activate(FL_WIDGET(self->larger_));
}

static void cb_larger(Fl_Widget *o, void *v) {
    Fl_Help_Dialog *self = (Fl_Help_Dialog *)v;
    (void)o;

    if (Fl_Help_View_textsize(self->view_) < 18) Fl_Help_View_set_textsize(self->view_, Fl_Help_View_textsize(self->view_) + 2);

    if (Fl_Help_View_textsize(self->view_) >= 18) Fl_Widget_deactivate(FL_WIDGET(self->larger_));
    Fl_Widget_activate(FL_WIDGET(self->smaller_));
}

static void cb_find(Fl_Widget *o, void *v) {
    Fl_Help_Dialog *self = (Fl_Help_Dialog *)v;
    (void)o;
    self->find_pos_ = Fl_Help_View_find(self->view_, Fl_Input_value(self->find_), self->find_pos_);
}

static void cb_view(Fl_Widget *o, void *v) {
    Fl_Help_Dialog *self = (Fl_Help_Dialog *)v;
    (void)o;

    if (Fl_Help_View_filename(self->view_)) {
        if (Fl_Widget_changed(&self->view_->group.widget)) {
            self->index_++;

            if (self->index_ >= CFLTK_HELP_DIALOG_HISTORY) {
                memmove(self->line_, self->line_ + 10, sizeof(self->line_[0]) * (CFLTK_HELP_DIALOG_HISTORY - 10));
                memmove(self->file_, self->file_ + 10, sizeof(self->file_[0]) * (CFLTK_HELP_DIALOG_HISTORY - 10));
                self->index_ -= 10;
            }

            self->max_ = self->index_;

            snprintf(self->file_[self->index_], sizeof(self->file_[0]), "%s", Fl_Help_View_filename(self->view_));
            self->line_[self->index_] = Fl_Help_View_topline(self->view_);

            if (self->index_ > 0) Fl_Widget_activate(FL_WIDGET(self->back_));
            else Fl_Widget_deactivate(FL_WIDGET(self->back_));

            Fl_Widget_deactivate(FL_WIDGET(self->forward_));
            Fl_Widget_set_label(FL_WIDGET(self->window_), Fl_Help_View_title(self->view_));
        } else {
            snprintf(self->file_[self->index_], sizeof(self->file_[0]), "%s", Fl_Help_View_filename(self->view_));
            self->line_[self->index_] = Fl_Help_View_topline(self->view_);
        }
    } else {
        self->index_ = 0;
        self->file_[self->index_][0] = '\0';
        self->line_[self->index_] = Fl_Help_View_topline(self->view_);
        Fl_Widget_deactivate(FL_WIDGET(self->back_));
        Fl_Widget_deactivate(FL_WIDGET(self->forward_));
    }
}

Fl_Help_Dialog *Fl_Help_Dialog_new(void) {
    Fl_Help_Dialog *self = (Fl_Help_Dialog *)malloc(sizeof(Fl_Help_Dialog));
    Fl_Group *toolbar_group;
    Fl_Group *find_group;
    Fl_Box *spacer;

    self->window_ = Fl_Double_Window_new(0, 0, 530, 385, "Help Dialog");

    toolbar_group = Fl_Group_new(10, 10, 511, 25, NULL);

    self->back_ = Fl_Button_new(10, 10, 25, 25, "@<-");
    Fl_Widget_set_tooltip(FL_WIDGET(self->back_), "Show the previous help page.");
    Fl_Button_set_shortcut(self->back_, FL_Left);
    Fl_Widget_set_labelcolor(FL_WIDGET(self->back_), (Fl_Color)2);
    Fl_Widget_set_callback(FL_WIDGET(self->back_), cb_back, self);

    self->forward_ = Fl_Button_new(45, 10, 25, 25, "@->");
    Fl_Widget_set_tooltip(FL_WIDGET(self->forward_), "Show the next help page.");
    Fl_Button_set_shortcut(self->forward_, FL_Right);
    Fl_Widget_set_labelcolor(FL_WIDGET(self->forward_), (Fl_Color)2);
    Fl_Widget_set_callback(FL_WIDGET(self->forward_), cb_forward, self);

    self->smaller_ = Fl_Button_new(80, 10, 25, 25, "F");
    Fl_Widget_set_tooltip(FL_WIDGET(self->smaller_), "Make the help text smaller.");
    Fl_Widget_set_labelfont(FL_WIDGET(self->smaller_), FL_HELVETICA_BOLD);
    Fl_Widget_set_labelsize(FL_WIDGET(self->smaller_), 10);
    Fl_Widget_set_callback(FL_WIDGET(self->smaller_), cb_smaller, self);

    self->larger_ = Fl_Button_new(115, 10, 25, 25, "F");
    Fl_Widget_set_tooltip(FL_WIDGET(self->larger_), "Make the help text larger.");
    Fl_Widget_set_labelfont(FL_WIDGET(self->larger_), FL_HELVETICA_BOLD);
    Fl_Widget_set_labelsize(FL_WIDGET(self->larger_), 16);
    Fl_Widget_set_callback(FL_WIDGET(self->larger_), cb_larger, self);

    find_group = Fl_Group_new(350, 10, 171, 25, NULL);
    Fl_Widget_set_box(FL_WIDGET(find_group), FL_DOWN_BOX);
    Fl_Widget_set_color(FL_WIDGET(find_group), FL_BACKGROUND2_COLOR);

    self->find_ = Fl_Input_new(375, 12, 143, 21, "@search");
    Fl_Widget_set_tooltip(FL_WIDGET(self->find_), "find text in document");
    Fl_Widget_set_box(FL_WIDGET(self->find_), FL_FLAT_BOX);
    Fl_Widget_set_labelsize(FL_WIDGET(self->find_), 13);
    Fl_Input_set_textfont(self->find_, FL_COURIER);
    Fl_Widget_set_callback(FL_WIDGET(self->find_), cb_find, self);
    Fl_Widget_set_when(FL_WIDGET(self->find_), FL_WHEN_ENTER_KEY_ALWAYS);

    Fl_Group_end(find_group);

    spacer = Fl_Box_new(150, 10, 190, 25, NULL);
    Fl_Group_set_resizable(toolbar_group, FL_WIDGET(spacer));

    Fl_Group_end(toolbar_group);

    self->view_ = Fl_Help_View_new(10, 45, 510, 330, NULL);
    Fl_Widget_set_box(FL_WIDGET(self->view_), FL_DOWN_BOX);
    Fl_Widget_set_callback(FL_WIDGET(self->view_), cb_view, self);
    Fl_Group_set_resizable(&self->window_->window.group, FL_WIDGET(self->view_));

    Fl_Group_end(&self->window_->window.group);

    Fl_Widget_deactivate(FL_WIDGET(self->back_));
    Fl_Widget_deactivate(FL_WIDGET(self->forward_));

    self->index_ = -1;
    self->max_ = 0;
    self->find_pos_ = 0;

    return self;
}

void Fl_Help_Dialog_delete(Fl_Help_Dialog *self) {
    if (!self) return;
    Fl_Widget_delete(FL_WIDGET(self->window_));
    free(self);
}

int Fl_Help_Dialog_h(const Fl_Help_Dialog *self) { return FL_WIDGET(self->window_)->h; }
int Fl_Help_Dialog_w(const Fl_Help_Dialog *self) { return FL_WIDGET(self->window_)->w; }
int Fl_Help_Dialog_x(const Fl_Help_Dialog *self) { return FL_WIDGET(self->window_)->x; }
int Fl_Help_Dialog_y(const Fl_Help_Dialog *self) { return FL_WIDGET(self->window_)->y; }
int Fl_Help_Dialog_visible(const Fl_Help_Dialog *self) { return Fl_Widget_visible(FL_WIDGET(self->window_)); }

void Fl_Help_Dialog_show(Fl_Help_Dialog *self) { Fl_Widget_show(FL_WIDGET(self->window_)); }
void Fl_Help_Dialog_hide(Fl_Help_Dialog *self) { Fl_Widget_hide(FL_WIDGET(self->window_)); }
void Fl_Help_Dialog_position(Fl_Help_Dialog *self, int x, int y) { Fl_Widget_position(FL_WIDGET(self->window_), x, y); }
void Fl_Help_Dialog_resize(Fl_Help_Dialog *self, int x, int y, int w, int h) { Fl_Widget_resize(FL_WIDGET(self->window_), x, y, w, h); }

void Fl_Help_Dialog_load(Fl_Help_Dialog *self, const char *f) {
    Fl_Widget_set_changed(&self->view_->group.widget);
    Fl_Help_View_load(self->view_, f);
    Fl_Widget_set_label(FL_WIDGET(self->window_), Fl_Help_View_title(self->view_));
}

void Fl_Help_Dialog_set_value(Fl_Help_Dialog *self, const char *f) {
    Fl_Widget_set_changed(&self->view_->group.widget);
    Fl_Help_View_set_value(self->view_, f);
    Fl_Widget_set_label(FL_WIDGET(self->window_), Fl_Help_View_title(self->view_));
}

const char *Fl_Help_Dialog_value(const Fl_Help_Dialog *self) { return Fl_Help_View_value(self->view_); }

void Fl_Help_Dialog_set_textsize(Fl_Help_Dialog *self, Fl_Fontsize s) {
    Fl_Help_View_set_textsize(self->view_, s);

    if (s <= 8) Fl_Widget_deactivate(FL_WIDGET(self->smaller_));
    else Fl_Widget_activate(FL_WIDGET(self->smaller_));

    if (s >= 18) Fl_Widget_deactivate(FL_WIDGET(self->larger_));
    else Fl_Widget_activate(FL_WIDGET(self->larger_));
}

Fl_Fontsize Fl_Help_Dialog_textsize(const Fl_Help_Dialog *self) { return Fl_Help_View_textsize(self->view_); }

void Fl_Help_Dialog_set_topline(Fl_Help_Dialog *self, int n) { Fl_Help_View_set_topline(self->view_, n); }
void Fl_Help_Dialog_set_topline_target(Fl_Help_Dialog *self, const char *n) { Fl_Help_View_set_topline_target(self->view_, n); }
