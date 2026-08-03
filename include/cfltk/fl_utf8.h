/*
 * cfltk - fl_utf8.h
 *
 * C translation of FLTK 1.3 FL/fl_utf8.h -- new infrastructure, not a
 * translation of an existing cfltk header. Fl_Text_Buffer's data model
 * is UTF-8 native by design (every position is a byte offset that must
 * land on a character boundary; char_at() returns a decoded UCS-4
 * codepoint), so implementing it faithfully requires real UTF-8
 * decode/encode, not the ASCII-only treatment cfltk uses elsewhere
 * (Fl_Widget shortcuts, Fl_Input, event text -- see docs/DESIGN.md).
 *
 * Scope is deliberately narrow: byte-level UTF-8 correctness (decode/
 * encode/sequence-length), which is what Fl_Text_Buffer's position
 * bookkeeping actually depends on. fl_tolower()/fl_toupper() (single
 * codepoint) only case-fold ASCII, kept that way deliberately (a
 * guaranteed-locale-independent fast path with no per-script table);
 * fl_utf_tolower()/fl_utf_toupper() (whole strings, added later - see
 * their own doc comments below) case-fold via the C library's
 * towlower()/towupper() instead, for callers that want real Unicode
 * case conversion and can accept it being locale-dependent.
 *
 * Known differences: no fl_utf8toUtf16()/fl_utf8fromUtf16() (Windows-only
 * upstream, irrelevant to the X11/NuttX targets this port cares about),
 * no fl_utf8fwd()/fl_utf8back() (not needed by anything ported so far).
 * fl_utf8test()/fl_utf_nb_char() are now implemented (added for a
 * downstream embedder that needed UTF-8 validity testing and byte
 * counting beyond decode/encode).
 */
#ifndef CFLTK_FL_UTF8_H
#define CFLTK_FL_UTF8_H

#ifdef __cplusplus
extern "C" {
#endif

/* Byte length (1-4) of the UTF-8 sequence starting with byte c, or 1 if
 * c is not a valid leading byte (so callers can still advance). */
int fl_utf8len1(char c);

/* Decodes one UTF-8 sequence at p (not reading past end, if end != NULL)
 * into a UCS-4 codepoint, and sets *len to the number of bytes consumed.
 * Invalid sequences decode byte-for-byte as CP1252 (matching upstream's
 * ERRORS_TO_CP1252 default, which lets stray Latin-1/CP1252 text that
 * ended up somewhere UTF-8 was expected still show up as something
 * reasonable instead of a wall of replacement characters), and set
 * *len = 1. */
unsigned fl_utf8decode(const char *p, const char *end, int *len);

/* Encodes ucs (0-0x10ffff) as UTF-8 into buf (up to 4 bytes, buf must
 * have room) and returns the number of bytes written. */
int fl_utf8encode(unsigned ucs, char *buf);

/* ASCII-only case folding -- see header note above. */
int fl_tolower(unsigned int ucs);
int fl_toupper(unsigned int ucs);

/* Examines the first srclen bytes of src. Returns 0 if not legal UTF-8,
 * 1 if all ASCII, 2 if all below 0x800, 3 if all below 0x10000, and 4
 * otherwise. Rejects truncated sequences, invalid continuation bytes,
 * overlong encodings and UTF-16 surrogate halves. */
int fl_utf8test(const char *src, unsigned int srclen);

/* Counts the number of UTF-8 characters (i.e. non-continuation bytes)
 * in the first len bytes of str. */
int fl_utf_nb_char(const unsigned char *str, int len);

/* Case-converts the first len bytes of str (UTF-8) into buf (caller-
 * allocated, must have room for up to 4 bytes per input character -
 * worst case, every character's case mapping happens to encode longer
 * than the original), returning the number of bytes written. Unlike
 * fl_tolower()/fl_toupper() above (deliberately ASCII-only, see this
 * header's own note), these go through the C library's towlower()/
 * towupper() per decoded codepoint, so correctness for non-ASCII
 * scripts depends on the process's locale being UTF-8-aware (LC_CTYPE
 * set via setlocale(), typically already the case on modern Linux via
 * the environment's LANG/LC_ALL - cfltk does not call setlocale()
 * itself, that's an application-level decision). Falls back to
 * leaving a character unchanged if the "C" locale's tables don't know
 * its case mapping, same as towlower()/towupper() themselves do. */
int fl_utf_tolower(const unsigned char *str, int len, char *buf);
int fl_utf_toupper(const unsigned char *str, int len, char *buf);

#ifdef __cplusplus
}
#endif

#endif /* CFLTK_FL_UTF8_H */
