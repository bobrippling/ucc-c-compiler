#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "strings.h"
#include "str.h"

#include "../util/dynmap.h"
#include "../util/alloc.h"

#include "out/lbl.h"

stringlit *strings_lookup(dynmap **plit_tbl, struct cstring *cstr)
{
	stringlit *lit;
	dynmap *lit_tbl;

	if(!*plit_tbl)
		*plit_tbl = dynmap_new(struct cstring *, cstring_eq, cstring_hash);
	lit_tbl = *plit_tbl;

	lit = dynmap_get(struct cstring *, stringlit *, lit_tbl, cstr);

	if(!lit){
		enum out_label_store store_type;
		stringlit *prev;

		lit = umalloc(sizeof *lit);
		lit->cstr = cstr;

		switch(cstr->type){
			case CSTRING_u16:
			case CSTRING_u32:
			case CSTRING_WIDE:
				store_type = STORE_P_WCHAR;
				break;
			case CSTRING_RAW:
			case CSTRING_u8:
				store_type = STORE_P_CHAR;
				break;
		}

		/* create the label immediately - used in const folding */
		lit->lbl = out_label_data_store(store_type);

		prev = dynmap_set(struct cstring *, stringlit *, lit_tbl, cstr, lit);
		assert(!prev);
	}

	return lit;
}

void stringlit_use(stringlit *s)
{
	s->use_cnt++;
}

int stringlit_empty(const stringlit *str)
{
	struct cstring *cstr = str->cstr;

	switch(cstr->count){
		case 0:
			return 1;
		case 1:
			return cstring_char_at(cstr, 0) == 0;
	}
	return 0;
}
