#ifndef UTIL_H
#define UTIL_H

#define ucc_unreach(optional) do{ ICE("unreachable"); return optional; }while(0)

#include "warn.h"

char *udirname(const char *);
char *ext_replace(const char *str, const char *ext);

void ice(const char *f, int line, const char *fn, const char *fmt, ...) ucc_printflike(4, 5) ucc_dead;
void icw(const char *f, int line, const char *fn, const char *fmt, ...) ucc_printflike(4, 5);
#define UCC_ASSERT(b, ...) do if(!(b)) ICE(__VA_ARGS__); while(0)
#define ICE(...) ice(__FILE__, __LINE__, __func__, __VA_ARGS__)
#define ICW(...) icw(__FILE__, __LINE__, __func__, __VA_ARGS__)

#define ICW_1(...) { static int warned = 0; if(!warned){ warned = 1; ICW(__VA_ARGS__);}}

#endif
