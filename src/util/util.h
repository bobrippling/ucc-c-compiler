#ifndef UTIL_H
#define UTIL_H

#define memcpy_safe(a, b) (*(a) = *(b))

#define ucc_unreach(optional) do{ ICE("unreachable"); return optional; }while(0)

extern int warning_count;

void warn_colour(int on, int err);

#include "where.h"
#include "assert.h"

/* used by the *_had_error notification+continue code */
void warn_at_print_error(struct where *, const char *fmt, ...);

void warn_at(struct where *, const char *, ...) ucc_printflike(2, 3);
void die_at(struct where *, const char *, ...) ucc_printflike(2, 3) ucc_dead;
void vwarn(struct where *w, int err,  const char *fmt, va_list l);
void vdie(struct where *, const char *, va_list) ucc_dead;
void die(const char *fmt, ...) ucc_printflike(1, 2) ucc_dead;

extern void include_bt(FILE *); /* implemented by the program */

char *fline(FILE *f);
char *udirname(const char *);
char *ext_replace(const char *str, const char *ext);

char *terminating_quote(char *);

#endif
