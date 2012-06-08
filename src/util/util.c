#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "util.h"
#include "alloc.h"

#define WHERE_FMT "%s:%d:%d"
#define WHERE_ARGS w->fname, w->line, w->chr + 1

const char *where_str(const struct where *w)
{
	static char buf[WHERE_BUF_SIZ];
	snprintf(buf, sizeof buf, WHERE_FMT, WHERE_ARGS);
	return buf;
}

struct where *default_where(struct where *w)
{
	if(!w){
		extern const char *current_fname;
		extern int current_line, current_chr;
		static struct where instead;

		w = &instead;

		instead.fname = current_fname;
		instead.line  = current_line;
		instead.chr   = current_chr;
	}

	return w;
}

void vwarn(struct where *w, int err, const char *fmt, va_list l)
{
	w = default_where(w);

	fprintf(stderr, "%s: %s: ", where_str(w), err ? "error" : "warning");
	vfprintf(stderr, fmt, l);

	if(fmt[strlen(fmt)-1] == ':'){
		fputc(' ', stderr);
		perror(NULL);
	}else{
		fputc('\n', stderr);
	}
}

void vdie(struct where *w, const char *fmt, va_list l)
{
	vwarn(w, 1, fmt, l);
	exit(1);
}

void warn_at(struct where *w, const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	vwarn(w, 0, fmt, l);
	va_end(l);
}

void die_at(struct where *w, const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	vdie(w, fmt, l);
	va_end(l);
	/* unreachable */
}

void die(const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	vdie(NULL, fmt, l); /* FIXME: this is called before current_fname etc is init'd */
	va_end(l);
	/* unreachable */
}

#define ICE_STR(s)  \
	va_list l; \
	struct where *w = default_where(NULL); \
	fprintf(stderr, WHERE_FMT ": " s " %s:%d (%s): ", WHERE_ARGS, f, line, fn); \
	va_start(l, fmt); \
	vfprintf(stderr, fmt, l); \
	fputc('\n', stderr)

void ice(const char *f, int line, const char *fn, const char *fmt, ...)
{
	ICE_STR("ICE");
	abort();
}

void icw(const char *f, int line, const char *fn, const char *fmt, ...)
{
	ICE_STR("ICW");
}

char *fline(FILE *f)
{
	int c, pos, len;
	char *line;

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
			len *= 2;
			line = urealloc(line, len);
			line[pos] = '\0';
		}

		if(c == '\n'){
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
