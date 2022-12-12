#ifndef UTIL_STR_H
#define UTIL_STR_H

#include <stddef.h> /* size_t */

#include "compiler.h"

char *str_quotefin2(char *s, char q);

char *str_spc_skip(const char *);
char *str_quotefin(char *);
char *char_quotefin(char *);

int str_endswith(const char *, const char *);

ucc_printflike(3, 4)
int xsnprintf(char *, size_t, const char *, ...);

char *str_split(char *haystack, char on, char **state);

#endif
