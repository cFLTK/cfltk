/*
 * cfltk - fl_file_chooser.c
 * See include/cfltk/fl_file_chooser.h for the scope/design notes.
 */
#define _XOPEN_SOURCE 700 /* for realpath(), see canon_dir() below */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cfltk/fl_file_chooser.h"
#include "cfltk/Fl.h"
#include "cfltk/Fl_Window.h"
#include "cfltk/Fl_File_Browser.h"
#include "cfltk/Fl_Input.h"
#include "cfltk/Fl_Return_Button.h"
#include "cfltk/fl_filename.h"

typedef struct {
    Fl_Window *window;
    Fl_Input *dirInput;
    Fl_File_Browser *browser;
    Fl_Input *nameInput;
    char dir[FL_PATH_MAX];   /* canonical, no trailing slash (except "/" itself) */
    int accepted;
} FileChooserCtx;

/* Resolves `dir` to an absolute, symlink-free path when possible
 * (matches dillo's own Dialog_canon_dir()); falls back to a plain
 * copy if realpath() fails (e.g. the directory doesn't exist yet). */
static void canon_dir(const char *dir, char *out, size_t outsz) {
    if (!realpath(dir, out)) {
        strncpy(out, dir, outsz - 1);
        out[outsz - 1] = '\0';
    }
}

static void reload(FileChooserCtx *ctx, const char *pattern) {
    Fl_Input_set_value_str(ctx->dirInput, ctx->dir);
    Fl_File_Browser_set_filter(ctx->browser, pattern);
    Fl_File_Browser_load(ctx->browser, ctx->dir, NULL);
}

static void navigate(FileChooserCtx *ctx, const char *newDir, const char *pattern) {
    canon_dir(newDir, ctx->dir, sizeof(ctx->dir));
    reload(ctx, pattern);
    Fl_Input_set_value_str(ctx->nameInput, "");
}

static void dirInput_cb(Fl_Widget *w, void *v) {
    FileChooserCtx *ctx = (FileChooserCtx *)v;
    const char *val = Fl_Input_value((Fl_Input *)w);
    struct stat st;
    if (stat(val, &st) == 0 && S_ISDIR(st.st_mode))
        navigate(ctx, val, Fl_File_Browser_filter(ctx->browser));
}

static void browser_cb(Fl_Widget *w, void *v) {
    FileChooserCtx *ctx = (FileChooserCtx *)v;
    Fl_Browser *br = (Fl_Browser *)w;
    int line = Fl_Browser_value(br);
    const char *text;
    size_t len;
    char full[FL_PATH_MAX];

    if (line < 1) return;
    text = Fl_Browser_text(br, line);
    if (!text) return;
    len = strlen(text);

    if (len > 0 && text[len - 1] == '/') {
        /* Directory entry (Fl_File_Browser_load() appends the
         * trailing '/' itself via fl_filename_list(), matching
         * upstream - see Fl_File_Browser.c). */
        if (Fl_event_clicks() > 0) {
            snprintf(full, sizeof(full), "%s/%.*s", ctx->dir, (int)(len - 1), text);
            navigate(ctx, full, Fl_File_Browser_filter(ctx->browser));
        }
    } else {
        Fl_Input_set_value_str(ctx->nameInput, text);
        if (Fl_event_clicks() > 0) {
            ctx->accepted = 1;
            Fl_Window_hide(FL_WIDGET(ctx->window));
        }
    }
}

static void ok_cb(Fl_Widget *w, void *v) {
    FileChooserCtx *ctx = (FileChooserCtx *)v;
    const char *name = Fl_Input_value(ctx->nameInput);
    char full[FL_PATH_MAX];
    struct stat st;
    (void)w;

    if (!name || !*name) return;

    snprintf(full, sizeof(full), "%s/%s", ctx->dir, name);
    if (stat(full, &st) == 0 && S_ISDIR(st.st_mode)) {
        navigate(ctx, full, Fl_File_Browser_filter(ctx->browser));
        return;
    }
    ctx->accepted = 1;
    Fl_Window_hide(FL_WIDGET(ctx->window));
}

static void cancel_cb(Fl_Widget *w, void *v) {
    FileChooserCtx *ctx = (FileChooserCtx *)v;
    (void)w;
    ctx->accepted = 0;
    Fl_Window_hide(FL_WIDGET(ctx->window));
}

const char *fl_file_chooser(const char *message, const char *pattern, const char *fname) {
    static char result[FL_PATH_MAX];
    static char last_dir[FL_PATH_MAX];
    static int have_last_dir = 0;

    FileChooserCtx ctx;
    int ww = 520, wh = 480, gap = 10, bh = 30, bw = 80;
    int dirInputY, browserY, nameInputY, buttonY, browserH;
    char init_dir[FL_PATH_MAX], init_base[FL_PATH_MAX];

    init_dir[0] = '\0';
    init_base[0] = '\0';

    if (fname && *fname) {
        const char *slash = strrchr(fname, '/');
        if (slash) {
            size_t dlen = (size_t)(slash - fname);
            if (dlen == 0) { init_dir[0] = '/'; init_dir[1] = '\0'; }
            else {
                if (dlen >= sizeof(init_dir)) dlen = sizeof(init_dir) - 1;
                memcpy(init_dir, fname, dlen);
                init_dir[dlen] = '\0';
            }
            strncpy(init_base, slash + 1, sizeof(init_base) - 1);
        } else {
            strncpy(init_base, fname, sizeof(init_base) - 1);
        }
    }
    if (!init_dir[0]) {
        const char *home;
        if (have_last_dir) strncpy(init_dir, last_dir, sizeof(init_dir) - 1);
        else if ((home = getenv("HOME")) != NULL) strncpy(init_dir, home, sizeof(init_dir) - 1);
        else { init_dir[0] = '/'; init_dir[1] = '\0'; }
    }

    canon_dir(init_dir, ctx.dir, sizeof(ctx.dir));
    ctx.accepted = 0;

    dirInputY = gap;
    browserY = dirInputY + 24 + gap;
    buttonY = wh - gap - bh;
    nameInputY = buttonY - gap - 24;
    browserH = nameInputY - gap - browserY;

    ctx.window = Fl_Window_new(0, 0, ww, wh, (message && *message) ? message : "File");
    Fl_Group_begin(&ctx.window->group);

    ctx.dirInput = Fl_Input_new(gap, dirInputY, ww - 2 * gap, 24, NULL);
    Fl_Widget_set_when(FL_WIDGET(ctx.dirInput), FL_WHEN_ENTER_KEY);
    Fl_Widget_set_callback(FL_WIDGET(ctx.dirInput), dirInput_cb, &ctx);

    ctx.browser = Fl_File_Browser_new(gap, browserY, ww - 2 * gap, browserH, NULL);
    /* Fl_Browser_new()/Fl_File_Browser_new() default to
     * FL_NORMAL_BROWSER, which does not select lines on click at all
     * (Fl_Browser_.c's FL_PUSH/FL_RELEASE handling is a no-op for that
     * type - only FL_SELECT_BROWSER/FL_HOLD_BROWSER/FL_MULTI_BROWSER
     * do). Matches upstream Fl_File_Chooser's own browser, which sets
     * this explicitly for the same reason. */
    Fl_Widget_set_type(FL_WIDGET(&ctx.browser->browser), FL_HOLD_BROWSER);
    /* FL_WHEN_ENTER_KEY here does NOT mean the Enter key - on a
     * browser it's the flag Fl_Browser_'s own FL_RELEASE handler
     * checks to decide whether a double-click (Fl_event_clicks() > 0)
     * should invoke the callback at all; without it double-clicks are
     * silently swallowed. Matches examples/file_browser/file_browser.c,
     * the one place in this codebase double-click-to-navigate was
     * already built and interactively verified. */
    Fl_Widget_set_when(FL_WIDGET(&ctx.browser->browser), (uchar)(FL_WHEN_RELEASE | FL_WHEN_ENTER_KEY));
    Fl_Widget_set_callback(FL_WIDGET(&ctx.browser->browser), browser_cb, &ctx);
    Fl_Group_set_resizable(&ctx.window->group, FL_WIDGET(&ctx.browser->browser));

    ctx.nameInput = Fl_Input_new(gap, nameInputY, ww - 2 * gap, 24, NULL);

    Fl_Button *ok = Fl_Return_Button_new(ww - 2 * (gap + bw), buttonY, bw, bh, "OK");
    Fl_Widget_set_callback(FL_WIDGET(ok), ok_cb, &ctx);

    Fl_Button *cancel = Fl_Button_new(ww - (gap + bw), buttonY, bw, bh, "Cancel");
    Fl_Widget_set_callback(FL_WIDGET(cancel), cancel_cb, &ctx);

    Fl_Group_end(&ctx.window->group);

    reload(&ctx, (pattern && *pattern) ? pattern : NULL);
    if (init_base[0]) Fl_Input_set_value_str(ctx.nameInput, init_base);

    Fl_Window_set_modal(ctx.window);
    Fl_Widget_show(FL_WIDGET(ctx.window));
    while (Fl_Window_shown(ctx.window))
        Fl_wait();

    if (ctx.accepted) {
        snprintf(result, sizeof(result), "%s/%s", ctx.dir, Fl_Input_value(ctx.nameInput));
        strncpy(last_dir, ctx.dir, sizeof(last_dir) - 1);
        last_dir[sizeof(last_dir) - 1] = '\0';
        have_last_dir = 1;
    }

    Fl_Widget_delete(FL_WIDGET(ctx.window));

    return ctx.accepted ? result : NULL;
}
