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
	struct cstring *cstr;
	unsigned use_cnt;
};

stringlit *strings_lookup(
		dynmap **lit_tbl,
		struct cstring *);

void stringlit_use(stringlit *);

int stringlit_empty(const stringlit *);

#endif
