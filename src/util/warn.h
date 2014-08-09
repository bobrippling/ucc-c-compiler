#ifndef WARN_H
#define WARN_H

#include "where.h"

extern void include_bt(FILE *); /* implemented by the program */

extern int warning_count;
extern int warning_length; /* -fmessage-length */

/* used by the *_had_error notification+continue code */
void warn_at_print_error(const struct where *, const char *fmt, ...);

void warn_at(const struct where *, const char *, ...) ucc_printflike(2, 3);
void die_at(const struct where *, const char *, ...) ucc_printflike(2, 3) ucc_dead;
void vwarn(const struct where *w, int err,  const char *fmt, va_list l);
void vdie(const struct where *, const char *, va_list) ucc_dead;
void die(const char *fmt, ...) ucc_printflike(1, 2) ucc_dead;

void warn_colour(int on, int err);

#endif
