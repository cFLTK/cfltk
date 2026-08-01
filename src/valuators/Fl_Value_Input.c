/*
 * cfltk - Fl_Value_Input.c
 * See include/cfltk/Fl_Value_Input.h for the class-conversion notes,
 * especially the embedded-Fl_Input "kludge" and why it's safe here.
 * Translated from src/Fl_Value_Input.cxx.
 */
#include <math.h>
#include <stdlib.h>

#include "cfltk/Fl_Value_Input.h"
#include "cfltk/Fl.h"
#include "cfltk/Fl_Group.h"

const Fl_WidgetOps fl_value_input_ops = {
    Fl_Value_Input_draw,
    Fl_Value_Input_handle,
    Fl_Value_Input_resize,
    NULL, NULL,
    Fl_Value_Input_destroy,
    NULL, NULL
};

static void value_damage_cb(Fl_Valuator *v) {
    Fl_Value_Input *self = (Fl_Value_Input *)v;
    char buf[128];
    Fl_Valuator_format(v, buf);
    Fl_Input_set_value_str(&self->input, buf);
    Fl_Input_set_mark(&self->input, Fl_Input_position(&self->input));
}

static void input_cb(Fl_Widget *w, void *data) {
    Fl_Value_Input *self = (Fl_Value_Input *)data;
    Fl_Valuator *v = &self->valuator;
    double step = Fl_Valuator_step(v);
    double nv;
    (void)w;
    if ((step - floor(step)) > 0.0 || step == 0.0) nv = strtod(Fl_Input_value(&self->input), NULL);
    else nv = (double)strtol(Fl_Input_value(&self->input), NULL, 0);
    if (nv != Fl_Valuator_value(v) || (Fl_Widget_when(&v->widget) & FL_WHEN_NOT_CHANGED)) {
        Fl_Valuator_set_value(v, nv);
        Fl_Widget_set_changed(&v->widget);
        if (Fl_Widget_when(&v->widget)) Fl_Widget_do_callback(&v->widget);
    }
}

void Fl_Value_Input_init(Fl_Value_Input *self, int x, int y, int w, int h, const char *label) {
    Fl_Valuator_init(&self->valuator, &fl_value_input_ops, x, y, w, h, label);
    self->valuator.value_damage = value_damage_cb;

    Fl_Input_init(&self->input, x, y, w, h, NULL);
    /* Defeat the automatic add-to-current-group Fl_Widget_init() just did,
     * then force-set parent to this (not a real Fl_Group -- see header). */
    if (self->input.widget.parent) Fl_Group_remove(self->input.widget.parent, &self->input.widget);
    self->input.widget.parent = (Fl_Group *)self;
    Fl_Widget_set_callback(&self->input.widget, input_cb, self);
    Fl_Widget_set_when(&self->input.widget, FL_WHEN_CHANGED);

    self->soft_ = 0;
    Fl_Widget_set_box(&self->valuator.widget, Fl_Widget_box(&self->input.widget));
    Fl_Widget_set_color(&self->valuator.widget, Fl_Widget_color(&self->input.widget));
    Fl_Widget_set_selection_color(&self->valuator.widget, Fl_Widget_selection_color(&self->input.widget));
    Fl_Widget_set_align(&self->valuator.widget, FL_ALIGN_LEFT);
    value_damage_cb(&self->valuator);
    self->valuator.widget.flags |= FL_WIDGET_SHORTCUT_LABEL;
}

Fl_Value_Input *Fl_Value_Input_new(int x, int y, int w, int h, const char *label) {
    Fl_Value_Input *self = (Fl_Value_Input *)malloc(sizeof(Fl_Value_Input));
    Fl_Value_Input_init(self, x, y, w, h, label);
    return self;
}

void Fl_Value_Input_destroy(Fl_Widget *self_w) {
    Fl_Value_Input *self = (Fl_Value_Input *)self_w;
    /* Un-kludge before Fl_Input_destroy()'s Fl_Widget_base_destroy() call,
     * which would otherwise call Fl_Group_remove() on our fake parent
     * pointer and misinterpret Fl_Valuator's fields as Fl_Group's. */
    if (self->input.widget.parent == (Fl_Group *)self) self->input.widget.parent = NULL;
    Fl_Input_destroy(&self->input.widget);
    Fl_Widget_base_destroy(self_w);
}

void Fl_Value_Input_draw(Fl_Widget *self_w) {
    Fl_Value_Input *self = (Fl_Value_Input *)self_w;
    if (self_w->damage & (uchar)~FL_DAMAGE_CHILD) Fl_Widget_set_damage(&self->input.widget, FL_DAMAGE_ALL);
    Fl_Widget_set_box(&self->input.widget, self_w->box);
    Fl_Widget_set_colors(&self->input.widget, self_w->color, Fl_Widget_selection_color(self_w));
    Fl_Input_draw(&self->input.widget);
    self->input.widget.damage = 0;
}

void Fl_Value_Input_resize(Fl_Widget *self_w, int x, int y, int w, int h) {
    Fl_Value_Input *self = (Fl_Value_Input *)self_w;
    Fl_Widget_default_resize(self_w, x, y, w, h);
    Fl_Input_resize(&self->input.widget, x, y, w, h);
}

int Fl_Value_Input_handle(Fl_Widget *self_w, int event) {
    Fl_Value_Input *self = (Fl_Value_Input *)self_w;
    Fl_Valuator *v = &self->valuator;
    double vv;
    int delta;
    int mx = Fl_event_x_root();
    static int ix, drag;

    Fl_Widget_set_when(&self->input.widget, Fl_Widget_when(self_w));

    switch (event) {
        case FL_PUSH:
            if (!Fl_Valuator_step(v)) goto DEFAULT;
            ix = mx;
            drag = Fl_event_button();
            Fl_Valuator_handle_push(v);
            return 1;
        case FL_DRAG:
            if (!Fl_Valuator_step(v)) goto DEFAULT;
            delta = mx - ix;
            if (delta > 5) delta -= 5;
            else if (delta < -5) delta += 5;
            else delta = 0;
            switch (drag) {
                case 3: vv = Fl_Valuator_increment(v, Fl_Valuator_previous_value(v), delta * 100); break;
                case 2: vv = Fl_Valuator_increment(v, Fl_Valuator_previous_value(v), delta * 10); break;
                default: vv = Fl_Valuator_increment(v, Fl_Valuator_previous_value(v), delta); break;
            }
            vv = Fl_Valuator_round(v, vv);
            Fl_Valuator_handle_drag(v, self->soft_ ? Fl_Valuator_softclamp(v, vv) : Fl_Valuator_clamp(v, vv));
            return 1;
        case FL_RELEASE:
            if (!Fl_Valuator_step(v)) goto DEFAULT;
            if (Fl_Valuator_value(v) != Fl_Valuator_previous_value(v) || !Fl_event_is_click())
                Fl_Valuator_handle_release(v);
            else {
                Fl_Widget_Tracker wp;
                Fl_Widget_Tracker_watch(&wp, &self->input.widget);
                Fl_Input_handle(&self->input.widget, FL_PUSH);
                if (Fl_Widget_Tracker_exists(&wp)) Fl_Input_handle(&self->input.widget, FL_RELEASE);
                Fl_Widget_Tracker_release(&wp);
            }
            return 1;
        case FL_FOCUS:
            return Fl_Widget_take_focus(&self->input.widget);
        case FL_SHORTCUT:
            return Fl_Input_handle(&self->input.widget, event);
        default:
        DEFAULT:
            {
                double step = Fl_Valuator_step(v);
                Fl_Input_set_input_type(&self->input, ((step - floor(step)) > 0.0 || step == 0.0) ? FL_FLOAT_INPUT : FL_INT_INPUT);
            }
            return Fl_Input_handle(&self->input.widget, event);
    }
}
