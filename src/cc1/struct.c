#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "../util/dynarray.h"
#include "data_structs.h"
#include "struct.h"
#include "sym.h"
#include "struct_enum.h"

int struct_size(struct_st *st)
{
	decl **i;
	int r = 0;
	for(i = st->members; i && *i; i++)
		r += decl_size(*i);
	return r;
}

/* TODO: merge with similar code in enum.c -> struct_enum.c */
struct_st *struct_find(symtable *stab, const char *spel)
{
	for(; stab; stab = stab->parent){
		struct_st **i;

		for(i = stab->structs; i && *i; i++){
			struct_st *st = *i;
			if(st->spel && !strcmp(st->spel, spel))
				return st;
		}
	}
	return NULL;
}

struct_st *struct_add(symtable *const stab, char *spel, decl **members)
{
	struct_st *struct_st;
	decl **iter;

	if(spel && (struct_st = struct_find(stab, spel))){
		char buf[WHERE_BUF_SIZ];
		strcpy(buf, where_str(&members[0]->where));
		die_at(&struct_st->members[0]->where, "duplicate struct from %s", buf);
	}

	struct_st = umalloc(sizeof *struct_st);

	for(iter = members; iter && *iter; iter++){
		decl *d = *iter;
		if(d->init)
			die_at(&d->init->where, "struct member %s is initialised", d->spel);
	}

	st_en_set_spel(&struct_st->spel, spel, "struct");

	struct_st->members = members;

	dynarray_add((void ***)&stab->structs, struct_st);

	return struct_st;
}
