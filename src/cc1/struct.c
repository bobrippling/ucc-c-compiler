#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "../util/dynarray.h"
#include "tree.h"
#include "struct.h"

int struct_size(struct_st *st)
{
	decl **i;
	int r = 0;
	for(i = st->members; *i; i++)
		r += decl_size(*i);
	return r;
}

struct_st *struct_find(struct_st **structs, const char *spel)
{
	struct_st **i;

	for(i = structs; i && *i; i++){
		struct_st *st = *structs;
		if(st->spel && !strcmp(st->spel, spel))
			return st;
	}

	return NULL;
}

struct_st *struct_add(struct_st ***structs, char *spel, decl **members)
{
	struct_st *struct_st;
	decl **iter;

	if(spel && (struct_st = struct_find(*structs, spel))){
		char buf[WHERE_BUF_SIZ];
		strcpy(buf, where_str(&members[0]->where));
		die_at(&struct_st->members[0]->where, "duplicate struct from %s", buf);
	}

	struct_st = umalloc(sizeof *struct_st);

	for(iter = members; *iter; iter++){
		decl *d = *iter;
		if(d->init)
			die_at(&d->init->where, "struct member %s is initialised", d->spel);
	}

	if(spel){
		struct_st->spel = spel;
	}else{
		struct_st->spel = umalloc(32);
		snprintf(struct_st->spel, 32, "<anon %p>", (void *)struct_st);
	}

	struct_st->members = members;

	dynarray_add((void ***)structs, struct_st);

	return struct_st;
}
