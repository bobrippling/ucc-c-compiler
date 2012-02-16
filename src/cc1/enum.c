#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "../util/dynarray.h"
#include "tree.h"
#include "enum.h"

void enum_vals_add(enum_st *en, char *sp, expr *e)
{
	enum_member *mem = umalloc(sizeof *mem);
	if(!e)
		e = (expr *)-1;

	mem->spel = sp;
	mem->val  = e;

	dynarray_add((void ***)&en->members, mem);
}

enum_st *enum_add( enum_st ***ens, char *spel, enum_st *en)
{
	en->spel = spel;
	dynarray_add((void ***)ens, en);
	return en;
}

enum_st *enum_find(enum_st **ens, const char *spel)
{
	enum_st **i;
	for(i = ens; i && *i; i++){
		enum_st *e = *i;
		if(!strcmp(spel, e->spel))
			return e;
	}
	return NULL;
}

enum_st *enum_st_new(void)
{
	return umalloc(sizeof *enum_st_new());
}
