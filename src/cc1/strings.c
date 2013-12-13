#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "strings.h"

#include "../util/dynmap.h"
#include "../util/alloc.h"

#include "out/lbl.h"

struct string_key
{
	char *str;
	int is_wide;
};

static int strings_key_eq(void *a, void *b)
{
	const struct string_key *ka = a, *kb = b;

	if(ka->is_wide != kb->is_wide)
		return 1;
	return strcmp(ka->str, kb->str);
}

stringlit *strings_lookup(
		dynmap **plit_tbl, char *s, size_t len, int wide)
{
	stringlit *lit;
	dynmap *lit_tbl;
	struct string_key key = { s, wide };

	if(!*plit_tbl)
		*plit_tbl = dynmap_new(strings_key_eq);
	lit_tbl = *plit_tbl;

	lit = dynmap_get(struct string_key *, stringlit *, lit_tbl, &key);

	if(!lit){
		struct string_key *alloc_key;

		lit = umalloc(sizeof *lit);
		lit->str = s;
		lit->len = len;
		lit->wide = wide;

		alloc_key = umalloc(sizeof *alloc_key);
		*alloc_key = key;
		dynmap_set(struct string_key *, stringlit *, lit_tbl, alloc_key, lit);
	}

	return lit;
}

void stringlit_use(stringlit *s)
{
	if(s->use_cnt++ == 0)
		s->lbl = out_label_data_store(s->wide ? STORE_P_WCHAR : STORE_P_CHAR);
}
