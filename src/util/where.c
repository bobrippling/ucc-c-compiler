#include <stdio.h>
#include "where.h"

#define WHERE_FMT "%s:%d:%d"
#define WHERE_ARGS w->fname, w->line, w->chr + 1

struct where *default_where(struct where *w)
{
	if(!w){
		static struct where instead;

		w = &instead;
		where_current(w);
	}

	return w;
}

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
	extern const char *current_fname, *current_line_str;
	extern int current_line, current_chr;

	w->fname    = current_fname;
	w->line     = current_line;
	w->chr      = current_chr;
	w->line_str = current_line_str;
}
