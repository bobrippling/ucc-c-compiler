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

int struct_union_size(struct_union_st *st)
{
	decl **i;
	int total, max;

	total = max = 0;

	for(i = st->members; i && *i; i++){
		const int sz = decl_size(*i);
		total += sz;
		if(sz > max)
			max = sz;
	}

	return st->is_union ? max : total;
}

/* TODO: merge with similar code in enum.c -> struct_enum.c */
struct_union_st *struct_union_find(symtable *stab, const char *spel)
{
	for(; stab; stab = stab->parent){
		struct_union_st **i;

		for(i = stab->structs; i && *i; i++){
			struct_union_st *st = *i;
			if(st->spel && !strcmp(st->spel, spel))
				return st;
		}
	}
	return NULL;
}

struct_union_st *struct_union_add(symtable *const stab, char *spel, decl **members, int is_union)
{
	struct_union_st *struct_union_st;
	decl **iter;

	if(spel && (struct_union_st = struct_union_find(stab, spel))){
		char buf[WHERE_BUF_SIZ];
		strcpy(buf, where_str(&members[0]->where));
		die_at(&struct_union_st->members[0]->where, "duplicate struct from %s", buf);
	}

	struct_union_st = umalloc(sizeof *struct_union_st);

	struct_union_st->is_union = is_union;

	for(iter = members; iter && *iter; iter++){
		decl *d = *iter;
		if(d->init)
			die_at(&d->init->where, "struct member %s is initialised", d->spel);
	}

	struct_union_st->anon = !spel;

	st_en_un_set_spel(&struct_union_st->spel, spel, struct_union_str(struct_union_st));

	struct_union_st->members = members;

	dynarray_add((void ***)&stab->structs, struct_union_st);

	return struct_union_st;
}

decl *struct_union_member_find(struct_union_st *st, const char *spel, where *die_where)
{
	decl **i;

	for(i = st->members; i && *i; i++)
		if(!strcmp((*i)->spel, spel))
			return *i;

	die_at(die_where, "struct %s has no member named \"%s\"", st->spel, spel);
	return NULL;
}
