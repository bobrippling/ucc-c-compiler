#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "../util/dynarray.h"
#include "tree.h"
#include "struct.h"

int struct_member_offset(expr *e)
{
	int offset = e->rhs->tree_type->struct_offset;

	UCC_ASSERT(e->type == expr_struct, "not a struct");

	/*
	 * walk down e->lhs->lhs->lhs... until we reach an identifier
	 * adding offsets as we go
	 */

	while(e->lhs->type == expr_struct){
		offset += e->lhs->tree_type->struct_offset;
		e = e->lhs;
	}

	return offset;
}

int struct_size(struc *st)
{
	decl **i;
	int r = 0;
	for(i = st->members; *i; i++)
		r += decl_size(*i);
	return r;
}

struc *struct_find(struc **structs, const char *spel)
{
	struc **i;

	for(i = structs; i && *i; i++){
		struc *st = *structs;
		if(st->spel && !strcmp(st->spel, spel))
			return st;
	}

	return NULL;
}

struc *struct_add(struc ***structs, char *spel, decl **members)
{
	struc *struc;

	if(spel && (struc = struct_find(*structs, spel))){
		char buf[WHERE_BUF_SIZ];
		strcpy(buf, where_str(&members[0]->where));
		die_at(&struc->members[0]->where, "duplicate struct from %s", buf);
	}

	struc          = umalloc(sizeof *struc);
	struc->spel    = spel;
	struc->members = members;

	dynarray_add((void ***)structs, struc);

	return struc;
}
