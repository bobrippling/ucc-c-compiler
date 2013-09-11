#ifndef UTIL_H
#define UTIL_H

#define ucc_unreach(optional) do{ ICE("unreachable"); return optional; }while(0)

extern int warning_count;

void warn_colour(int on, int err);

#include "where.h"

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

void ice(const char *f, int line, const char *fn, const char *fmt, ...) ucc_printflike(4, 5) ucc_dead;
void icw(const char *f, int line, const char *fn, const char *fmt, ...) ucc_printflike(4, 5);
#define UCC_ASSERT(b, ...) do if(!(b)) ICE(__VA_ARGS__); while(0)
#define ICE(...) ice(__FILE__, __LINE__, __func__, __VA_ARGS__)
#define ICW(...) icw(__FILE__, __LINE__, __func__, __VA_ARGS__)

char *terminating_quote(char *);

#endif
