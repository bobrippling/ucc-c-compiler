#ifndef STRINGS_H
#define STRINGS_H

/* used for caching string literals
 * may be used for static const compound literals
 */

#include "../util/dynmap.h"

typedef struct stringlit stringlit;

struct stringlit
{
	char *lbl;
	const char *str;
	size_t len;
	int wide;
	unsigned use_cnt;
};

stringlit *strings_lookup(
		dynmap **lit_tbl,
		char *, size_t len, int wide);

#define stringlit_use(sl) ++((sl)->use_cnt)

#endif
