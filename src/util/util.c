#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "util.h"
#include "alloc.h"

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

char *udirname(const char *f)
{
	const char *fin;

	fin = strrchr(f, '/');

	if(fin){
		char *dup = ustrdup(f);
		dup[fin - f] = '\0';
		return dup;
	}else{
		return ustrdup("./");
	}
}

char *ext_replace(const char *str, const char *ext)
{
	char *dot;
	dot = strrchr(str, '.');

	if(dot){
		char *dup;

		dup = umalloc(strlen(ext) + strlen(str));

		strcpy(dup, str);
		sprintf(dup + (dot - str), ".%s", ext);

		return dup;
	}else{
		return ustrdup(str);
	}
}
