#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "../util/dynarray.h"
#include "data_structs.h"
#include "enum.h"
#include "sym.h"
#include "struct_enum.h"

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
	st_en_un_set_spel(&en->spel, spel, "enum");

	dynarray_add((void ***)ens, en);
	return en;
}

enum_member *enum_find_member(symtable *stab, const char *spel)
{
	for(; stab; stab = stab->parent){
		enum_st **i;

		for(i = stab->enums; i && *i; i++){
			enum_st *e = *i;
			enum_member **mi;
			for(mi = e->members; *mi; mi++){
				enum_member *m = *mi;
				if(!strcmp(spel, m->spel))
					return m;
			}
		}
	}

	return NULL;
}

enum_st *enum_find(symtable *stab, const char *spel)
{
	for(; stab; stab = stab->parent){
		enum_st **i;

		for(i = stab->enums; i && *i; i++){
			enum_st *e = *i;
			if(!strcmp(spel, e->spel))
				return e;
		}
	}

	return NULL;
}

enum_st *enum_st_new(void)
{
	return umalloc(sizeof *enum_st_new());
}

int enum_nentries(enum_st *e)
{
	int n = 0;
	enum_member **i;
	for(i = e->members; *i; i++, n++);
	return n;
}
