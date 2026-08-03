/*
 * cfltk - fl_file_chooser.h
 *
 * C translation of FLTK 1.3's `fl_file_chooser()` convenience function
 * (FL/Fl_File_Chooser.H's free-function entry point) -- NOT the full
 * Fl_File_Chooser class (favorites list, file-type icons, preview
 * pane, new-folder button, multi-select). This is the single call
 * every simple FLTK app (and dillo's own already-working, independent
 * Dialog_file_chooser() in src/dialog.c) actually needs: a modal
 * "pick one file" dialog. See docs/DESIGN.md for the scope note on why
 * the full class isn't ported.
 *
 * Original function : const char *fl_file_chooser(const char *message,
 *                      const char *pattern, const char *fname,
 *                      int relative = 0) -- the `relative` parameter
 *                      (return a path relative to fname's directory
 *                      instead of absolute) has no caller anywhere in
 *                      dillo (grep-confirmed) and is dropped here.
 * New C structure   : none exported -- internal state lives in a
 *                      file-local struct on the stack for the
 *                      duration of the (modal, blocking) call.
 * Built from         : Fl_File_Browser (already ported, directory
 *                      listing/sorting/filtering) + Fl_Input (path and
 *                      filename fields) + Fl_Return_Button/Fl_Button
 *                      (OK/Cancel) + Fl_Window_set_modal() (real modal
 *                      input blocking, see Fl_Window.h) -- the same
 *                      shape dillo's own Dialog_file_chooser() already
 *                      hand-built against lower-level primitives
 *                      before this existed, just backed by
 *                      Fl_File_Browser instead of a hand-rolled
 *                      opendir()/readdir() loop.
 */
#ifndef CFLTK_FL_FILE_CHOOSER_H
#define CFLTK_FL_FILE_CHOOSER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Blocks until the user picks a file (returns a pointer to an
 * internal static buffer, valid until the next call - matches
 * upstream's own documented contract) or cancels (returns NULL).
 * `pattern` is an optional shell-glob filter passed to
 * Fl_File_Browser_set_filter() (NULL/"" = show every file).  `fname`
 * optionally pre-fills the starting directory and/or filename: a path
 * containing '/' splits into directory+basename; a bare name with no
 * '/' just pre-fills the filename field, starting in the last
 * directory used by a previous call (or $HOME on the first call ever,
 * matching upstream). `message` is used as the window title. */
const char *fl_file_chooser(const char *message, const char *pattern, const char *fname);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_FILE_CHOOSER_H */
