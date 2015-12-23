#ifndef STRBUF_FIXED_H
#define STRBUF_FIXED_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

#include "macros.h"

typedef struct {
	char *str;
	size_t current;
	size_t max;
} strbuf_fixed;

#define STRBUF_FIXED_INIT(str, len) { str, 0, len }
#define STRBUF_FIXED_INIT_ARRAY(str) STRBUF_FIXED_INIT(str, sizeof(str))

attr_printf(2, 3)
bool strbuf_fixed_printf(strbuf_fixed *, const char *, ...);

attr_printf(2, 0)
bool strbuf_fixed_vprintf(strbuf_fixed *, const char *, va_list);

char *strbuf_fixed_detach(strbuf_fixed *);

#endif
