#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "util.h"
#include "alloc.h"

#define WHERE_FMT "%s:%d:%d"
#define WHERE_ARGS w->fname, w->line, w->chr + 1

enum
{
	colour_black,
	colour_red,
	colour_green,
	colour_orange,
	colour_blue,
	colour_magenta,
	colour_cyan,
	colour_white,

	colour_err  = colour_red,
	colour_warn = colour_orange
};

static const char *const colour_strs[] = {
	"\033[30m",
	"\033[31m",
	"\033[32m",
	"\033[33m",
	"\033[34m",
	"\033[35m",
	"\033[36m",
	"\033[37m",
};

const char *where_str_r(char buf[WHERE_BUF_SIZ], const struct where *w)
{
	snprintf(buf, WHERE_BUF_SIZ, WHERE_FMT, WHERE_ARGS);
	return buf;
}

const char *where_str(const struct where *w)
{
	static char buf[WHERE_BUF_SIZ];
	return where_str_r(buf, w);
}

struct where *default_where(struct where *w)
{
	if(!w){
		extern const char *current_fname, *current_line_str;
		extern int current_line, current_chr;
		static struct where instead;

		w = &instead;

		instead.fname    = current_fname;
		instead.line     = current_line;
		instead.chr      = current_chr;
		instead.line_str = current_line_str;
	}

	return w;
}

static void warn_show_line(struct where *w)
{
	extern int show_current_line;

	if(show_current_line && w->line_str){
		static int buffed = 0;
		char *line = ustrdup(w->line_str);
		char *p;
		int i;

		if(!buffed){
			setvbuf(stderr, NULL, _IOLBF, 0);
			buffed = 1;
		}

		for(p = line; *p; p++)
			if(*p == '\t')
				*p = ' ';

		fprintf(stderr, "  \"%s\"\n", line);

		for(i = w->chr + 2; i > 0; i--)
			fputc(' ', stderr);
		fputs("^\n", stderr);

		free(line);
	}
}

void vwarn(struct where *w, int err, int show_line, const char *fmt, va_list l)
{
	static enum { f = 0, t = 1, need_init = 2 } is_tty = need_init;

	if(is_tty == need_init)
		is_tty = isatty(2);

	if(is_tty)
		fputs(colour_strs[err ? colour_err : colour_warn], stderr);

	w = default_where(w);

	fprintf(stderr, "%s: %s: ", where_str(w), err ? "error" : "warning");
	vfprintf(stderr, fmt, l);

	if(fmt[strlen(fmt)-1] == ':'){
		fputc(' ', stderr);
		perror(NULL);
	}else{
		fputc('\n', stderr);
	}

	if(is_tty)
		fprintf(stderr, "\033[m");

	if(show_line)
		warn_show_line(w);
}

void vdie(struct where *w, int show_line, const char *fmt, va_list l)
{
	vwarn(w, 1, show_line, fmt, l);
	exit(1);
}

void warn_at(struct where *w, int show_line, const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	vwarn(w, 0, show_line, fmt, l);
	va_end(l);
}

void die_at(struct where *w, int show_line, const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	vdie(w, show_line, fmt, l);
	va_end(l);
	/* unreachable */
}

void die(const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	vdie(NULL, 0, fmt, l); /* FIXME: this is called before current_fname etc is init'd */
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

char *terminating_quote(char *s)
{
	/* accept backslashes properly "\\" */
	char *p;

	for(p = s; *p && *p != '"'; p++)
		if(*p == '\\')
			++p;

	return *p ? p : NULL;
}
