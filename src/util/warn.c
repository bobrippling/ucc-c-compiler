#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <limits.h>

#include "warn.h"
#include "alloc.h"

#ifndef MIN
#  define MIN(x, y) ((x) > (y) ? (y) : (x))
#endif

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
	colour_warn = colour_orange,
	colour_note = colour_blue
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
int warning_length = 80;

static const char *vwarn_colour(enum warn_type ty)
{
	int i = 0;
	switch(ty){
		case VWARN_ERR: i = colour_err; break;
		case VWARN_WARN: i = colour_warn; break;
		case VWARN_NOTE: i = colour_note; break;
	}
	return colour_strs[i];
}

void warn_colour(int on, enum warn_type ty)
{
	static enum { f = 0, t = 1, need_init = 2 } is_tty = need_init;

	if(is_tty == need_init)
		is_tty = isatty(2);

	if(is_tty){
		if(on)
			fputs(vwarn_colour(ty), stderr);
		else
			fprintf(stderr, "\033[m");
	}
}

static void warn_show_line_part(char *line, int pos, unsigned wlen)
{
	int len = strlen(line);
	int i;
	struct { int start, end; } dotdot = { 0, 0 };

	int warning_length_normalise = INT_MAX;
	if(warning_length > 0){
		warning_length_normalise = warning_length - 10;
		if(warning_length_normalise <= 0)
			warning_length_normalise = 1;

		if(len > warning_length_normalise){
			/* is `pos' visible? */
			if(pos >= warning_length_normalise){
				dotdot.start = 1;

				line += pos - warning_length_normalise / 2;
				len -= pos - warning_length_normalise / 2;

				pos = warning_length_normalise / 2;
			}

			if(len > warning_length_normalise){
				dotdot.end = 1;
				line[warning_length_normalise] = '\0';
			}
		}
	}

	fprintf(stderr, "  %s\"%s\"%s\n",
			dotdot.start ? "..." : "",
			line,
			dotdot.end ? "..." : "");

	for(i = pos + 3 + (dotdot.start ? 3 : 0); i > 0; i--)
		fputc(' ', stderr);

	fputc('^', stderr);

	/* don't go over */
	for(i = MIN(wlen, (unsigned)(warning_length_normalise - pos));
			i > 1; /* >1 since we've already put a '^' out */
			i--)
		fputc('~', stderr);

	fputc('\n', stderr);
}

static void warn_show_line(const struct where *w)
{
	extern int show_current_line;

	if(show_current_line && w->line_str){
		static int buffed = 0;
		char *line = ustrdup(w->line_str);
		char *p, *nonblank;

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

		warn_show_line_part(nonblank, w->chr - (nonblank - line), w->len);

		free(line);
	}
}

static const char *vwarn_str(enum warn_type ty)
{
	switch(ty){
		case VWARN_ERR: return "error";
		case VWARN_WARN: return "warning";
		case VWARN_NOTE: return "note";
	}
	return NULL;
}

void vwarn(const struct where *w, enum warn_type ty,
		const char *fmt, va_list l)
{
	include_bt(stderr);

	warn_colour(1, ty);

	w = default_where(w);

	fprintf(stderr, "%s: %s: ", where_str(w), vwarn_str(ty));
	vfprintf(stderr, fmt, l);

	warning_count++;

	if(*fmt && fmt[strlen(fmt)-1] == ':'){
		fputc(' ', stderr);
		perror(NULL);
	}else{
		fputc('\n', stderr);
	}

	warn_colour(0, ty);

	warn_show_line(w);
}

void warn_at_print_error(const struct where *w, const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	vwarn(w, VWARN_ERR, fmt, l);
	va_end(l);
}


void vdie(const struct where *w, const char *fmt, va_list l)
{
	vwarn(w, VWARN_ERR, fmt, l);
	exit(1);
}

void note_at(const struct where *w, const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	vwarn(w, VWARN_NOTE, fmt, l);
	va_end(l);
}

void warn_at(const struct where *w, const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	vwarn(w, VWARN_WARN, fmt, l);
	va_end(l);
}

void die_at(const struct where *w, const char *fmt, ...)
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

