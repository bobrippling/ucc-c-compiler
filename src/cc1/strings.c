#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "strings.h"

#include "../util/dynmap.h"
#include "../util/alloc.h"

#include "out/lbl.h"

stringlit *strings_lookup(
		dynmap **plit_tbl, char *s, size_t len, int wide)
{
	stringlit *lit;
	dynmap *lit_tbl;

	if(!*plit_tbl)
		*plit_tbl = dynmap_new((dynmap_cmp_f *)strcmp);
	lit_tbl = *plit_tbl;

	lit = dynmap_get(char *, stringlit *, lit_tbl, s);

	if(!lit){
		lit = umalloc(sizeof *lit);
		lit->str = s;
		lit->len = len;
		lit->wide = wide;
		dynmap_set(char *, stringlit *, lit_tbl, s, lit);
	}

	return lit;
}

void stringlit_use(stringlit *s)
{
	if(s->use_cnt++ == 0)
		s->lbl = out_label_data_store(1);
}
