#include <stdlib.h>

#include "../util/where.h"
#include "../util/alloc.h"

#include "cc1_where.h"
#include "sue.h"
#include "decl.h"
#include "expr.h"

#include "macros.h"

#include "attribute.h"

attribute *attribute_new(enum attribute_type t)
{
	attribute *da = umalloc(sizeof *da);
	where_cc1_current(&da->where);
	da->type = t;
	return da;
}

void attribute_append(attribute **loc, attribute *new)
{
	/* may be appending from a prototype to a function def. */
	while(*loc)
		loc = &(*loc)->next;

	/* we can just link up, since pointers aren't rewritten now */
	*loc = /*attribute_copy(*/new/*)*/;
}

attribute *attr_present(attribute *da, enum attribute_type t)
{
	for(; da; da = da->next)
		if(da->type == t)
			return da;
	return NULL;
}

attribute *type_attr_present(type *r, enum attribute_type t)
{
	/*
	 * attributes can be on:
	 *
	 * decl (spel)
	 * type (specifically the type, pointer or func, etc)
	 * sue (struct A {} __attribute((packed)))
	 * type (__attribute((section("data"))) int a)
	 *
	 * this means typedefs carry attributes too
	 */

	while(r){
		attribute *da;

		if((da = attr_present(r->attr, t)))
			return da;

		switch(r->type){
			case type_btype:
			{
				struct_union_enum_st *sue = r->bits.type->sue;
				return sue ? attr_present(sue->attr, t) : NULL;
			}

			case type_tdef:
			{
				decl *d = r->bits.tdef.decl;

				if(d && (da = attr_present(d->attr, t)))
					return da;

				return expr_attr_present(r->bits.tdef.type_of, t);
			}

			case type_ptr:
			case type_block:
			case type_func:
			case type_array:
			case type_cast:
				r = r->ref;
				break;
		}
	}
	return NULL;
}

attribute *attribute_present(decl *d, enum attribute_type t)
{
	/* check the attr on the decl _and_ its type */
	attribute *da;
	if((da = attr_present(d->attr, t)))
		return da;
	if((da = type_attr_present(d->ref, t)))
		return da;

	return d->proto ? attribute_present(d->proto, t) : NULL;
}

attribute *expr_attr_present(expr *e, enum attribute_type t)
{
	attribute *da;

	if(expr_kind(e, cast)){
		da = expr_attr_present(e->expr, t);
		if(da)
			return da;
	}

	if(expr_kind(e, identifier)){
		sym *s = e->bits.ident.sym;
		if(s){
			da = attribute_present(s->decl, t);
			if(da)
				return da;
		}
	}

	return type_attr_present(e->tree_type, t);
}

const char *attribute_to_str(attribute *da)
{
	switch(da->type){
		CASE_STR_PREFIX(attr, format);
		CASE_STR_PREFIX(attr, unused);
		CASE_STR_PREFIX(attr, warn_unused);
		CASE_STR_PREFIX(attr, section);
		CASE_STR_PREFIX(attr, enum_bitmask);
		CASE_STR_PREFIX(attr, noreturn);
		CASE_STR_PREFIX(attr, noderef);
		CASE_STR_PREFIX(attr, nonnull);
		CASE_STR_PREFIX(attr, packed);
		CASE_STR_PREFIX(attr, sentinel);
		CASE_STR_PREFIX(attr, aligned);

		case attr_call_conv:
			switch(da->bits.conv){
				case conv_x64_sysv: return "x64 SYSV";
				case conv_x64_ms:   return "x64 MS";
				CASE_STR_PREFIX(conv, cdecl);
				CASE_STR_PREFIX(conv, stdcall);
				CASE_STR_PREFIX(conv, fastcall);
			}

		case attr_LAST:
			break;
	}
	return NULL;
}

void attribute_free(attribute *a)
{
	if(!a)
		return;

	attribute_free(a->next);
	free(a);
}
