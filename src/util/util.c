#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "util.h"
#include "alloc.h"

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

int warning_count = 0;

static struct where *default_where(struct where *w)
{
	if(!w){
		static struct where instead;

		w = &instead;
		where_current(w);
	}

	return w;
}

static void warn_show_line(const struct where *w)
{
	extern int show_current_line;

	if(show_current_line && w->line_str){
		static int buffed = 0;
		char *line = ustrdup(w->line_str);
		char *p, *nonblank;
		int i;

		if(!buffed){
			/* line buffer stderr since we're outputting chars */
			setvbuf(stderr, NULL, _IOLBF, 0);
			buffed = 1;
		}

		nonblank = NULL;
		for(p = line; *p; p++)
			switch(*p){
				case '\t':
					*p = ' ';
				case ' ':
					break;
				default:
					/* trim initial whitespace */
					if(!nonblank)
						nonblank = p;
			}

		if(!nonblank)
			nonblank = line;

		fprintf(stderr, "  \"%s\"\n", nonblank);

		for(i = w->chr + 2 - (nonblank - line); i > 0; i--)
			fputc(' ', stderr);
		fputs("^\n", stderr);

		free(line);
	}
}

void warn_colour(int on, int err)
{
	static enum { f = 0, t = 1, need_init = 2 } is_tty = need_init;

	if(is_tty == need_init)
		is_tty = isatty(2);

	if(is_tty){
		if(on)
			fputs(colour_strs[err ? colour_err : colour_warn], stderr);
		else
			fprintf(stderr, "\033[m");
	}
}

void vwarn(struct where *w, int err, int show_line, const char *fmt, va_list l)
{
	include_bt(stderr);

	warn_colour(1, err);

	w = default_where(w);

	fprintf(stderr, "%s: %s: ", where_str(w), err ? "error" : "warning");
	vfprintf(stderr, fmt, l);

	warning_count++;

	if(fmt[strlen(fmt)-1] == ':'){
		fputc(' ', stderr);
		perror(NULL);
	}else{
		fputc('\n', stderr);
	}

	warn_colour(0, err);

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
			const size_t old = len;
			len *= 2;
			line = urealloc(line, len, old);
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
