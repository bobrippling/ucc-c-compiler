#include <stdio.h>
#include <string.h>

#include "where.h"

#define WHERE_FMT "%s:%d:%d"
#define WHERE_ARGS w->fname, w->line, w->chr + 1

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

void where_current(where *w)
{
	extern struct loc loc_tok;
	extern const char *current_fname, *current_line_str;

	w->fname    = current_fname;
	w->line     = loc_tok.line;
	w->chr      = loc_tok.chr;
	w->line_str = current_line_str;
	w->len      = 0;
}

int where_equal(where *a, where *b)
{
	return memcmp(a, b, sizeof *a) == 0;
}

const struct where *default_where(const struct where *w)
{
	if(!w){
		static struct where instead;

		w = &instead;
		where_current(&instead);
	}

	return w;
}
