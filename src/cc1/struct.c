#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "../util/util.h"
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

	for(i = structs; i && *i; i++)
		if(!strcmp((*i)->spel, spel))
			return *i;

	return NULL;
}
