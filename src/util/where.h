#ifndef WHERE_H
#define WHERE_H

#include "compiler.h"

struct loc
{
	unsigned line, chr;
};

typedef struct where
{
	const char *fname, *line_str;
	unsigned short line, chr, len;
} where;
#define WHERE_INIT(fnam, lstr, n, c) { fnam, lstr, n, c, 0 }

#define WHERE_BUF_SIZ 128
const char *where_str(const struct where *w);
const char *where_str_r(char buf[ucc_static_param WHERE_BUF_SIZ], const struct where *w);

void where_current(where *);

int where_equal(where *, where *);

where *default_where(where *w);

#endif
