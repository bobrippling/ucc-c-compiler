#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "assert.h"

static void ice_msg(const char *pre,
		const char *f, int line, const char *fn, const char *fmt, va_list l)
{
	const struct where *w = default_where(NULL);

	fprintf(stderr, "%s: %s %s:%d (%s): ",
			where_str(w), pre, f, line, fn);

	vfprintf(stderr, fmt, l);
	fputc('\n', stderr);
}

void ice(const char *f, int line, const char *fn, const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	ice_msg("ICE", f, line, fn, fmt, l);
	va_end(l);
	abort();
}

void icw(const char *f, int line, const char *fn, const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	ice_msg("ICW", f, line, fn, fmt, l);
	va_end(l);
}
