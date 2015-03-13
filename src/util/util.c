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

char *fline(FILE *f, int *const newline)
{
	int c, pos, len;
	char *line;

	if(newline)
		*newline = 0;

	if(feof(f) || ferror(f))
		return NULL;

	pos = 0;
	len = 10;
	line = umalloc(len);

	do{
		errno = 0;
		if((c = fgetc(f)) == EOF){
			if(errno == EINTR)
				continue;
			if(pos)
				return line;
			free(line);
			return NULL;
		}

		line[pos++] = c;
		if(pos == len){
			const size_t old = len;
			len *= 2;
			line = urealloc(line, len, old);
			line[pos] = '\0';
		}

		if(c == '\n'){
			if(newline)
				*newline = 1;
			line[pos-1] = '\0';
			return line;
		}
	}while(1);
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
