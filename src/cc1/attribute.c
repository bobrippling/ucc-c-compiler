#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../util/where.h"
#include "../util/alloc.h"
#include "../util/warn.h"

#include "cc1_where.h"
#include "sue.h"
#include "decl.h"
#include "expr.h"

#include "macros.h"

#include "attribute.h"

attribute *attribute_new(enum attribute_type t)
{
	attribute *da = umalloc(sizeof *da);
	RETAIN_INIT(da, attribute_free);
	where_cc1_current(&da->where);
	da->type = t;
	return da;
}

void attribute_append(attribute **loc, attribute *new)
{
	/* may be appending from a prototype to a function def. */
	while(*loc)
		loc = &(*loc)->next;

	*loc = RETAIN(new);
}

attribute *attribute_copy(attribute *attr)
{
	attribute *ret;

	if(!attr)
		return NULL;

	ret = umalloc(sizeof *ret);

	memcpy(ret, attr, sizeof *ret);
	ret->rc.retains = 1;

	switch(ret->type){
		case attr_section:
			ret->bits.section = ustrdup(attr->bits.section);
			break;

		case attr_destructor:
			ret->bits.priority = attr->bits.priority; /* TODO: retain expr */
			break;

		case attr_format:
		case attr_unused:
		case attr_warn_unused:
		case attr_enum_bitmask:
		case attr_noreturn:
		case attr_noderef:
		case attr_nonnull:
		case attr_packed:
		case attr_sentinel:
		case attr_aligned:
		case attr_weak:
		case attr_cleanup:
		case attr_always_inline:
		case attr_noinline:
		case attr_desig_init:
		case attr_ucc_debug:
		case attr_call_conv:
		case attr_constructor:
			break;
		case attr_LAST:
			assert(0);
	}

	ret->next = attribute_copy(attr->next);

	return ret;
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

		switch(r->type){
			case type_auto:
				assert(0 && "__auto_type");

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

			case type_attr:
				if((da = attr_present(r->bits.attr, t)))
					return da;

			case type_ptr:
			case type_block:
			case type_func:
			case type_array:
			case type_cast:
			case type_where:
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
		sym *s = e->bits.ident.bits.ident.sym;
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
		CASE_STR_PREFIX(attr, weak);
		CASE_STR_PREFIX(attr, cleanup);
		CASE_STR_PREFIX(attr, desig_init);
		CASE_STR_PREFIX(attr, always_inline);
		CASE_STR_PREFIX(attr, noinline);
		CASE_STR_PREFIX(attr, constructor);
		CASE_STR_PREFIX(attr, destructor);
		CASE_STR_PREFIX(attr, ucc_debug);

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
	attribute *i;
	if(!a)
		return;

	for(i = a->next; i; i = i->next)
		RELEASE(i);

	free(a);
}

int attribute_equal(attribute *a, attribute *b)
{
#define NEQ(bit, memb) (a->bits.bit.memb != b->bits.bit.memb)

		for(; a && b; a = a->next, b = b->next){
			if(a->type != b->type)
				return 0;
			switch(a->type){
				case attr_LAST:
					assert(0);
				case attr_format:
					if(NEQ(format, fmt_func)
					|| NEQ(format, fmt_idx)
					|| NEQ(format, var_idx))
					{
						return 0;
					}
					if(a->bits.format.fmt_idx != b->bits.format.fmt_idx
					|| a->bits.format.var_idx != b->bits.format.var_idx)
					{
						return 0;
					}
					break;

				case attr_cleanup:
					/* since a cleanup must be a global function,
					 * we can just strcmp */
					return !strcmp(a->bits.cleanup->spel, b->bits.cleanup->spel);

				case attr_section:
					if(strcmp(a->bits.section, b->bits.section))
						return 0;
					break;

				case attr_call_conv:
					if(a->bits.conv != b->bits.conv)
						return 0;
					break;

				case attr_nonnull:
					if(a->bits.nonnull_args != b->bits.nonnull_args)
						return 0;
					break;

				case attr_constructor:
				case attr_destructor:
					if(a->bits.priority != b->bits.priority)
						return 0;
					break;

				case attr_sentinel:
					/* lazy solution for now */
					if(a->bits.sentinel != b->bits.sentinel)
						return 0;
					break;

				case attr_aligned:
					/* same as sentinel */
					if(a->bits.align != b->bits.align)
						return 0;
					break;

				case attr_unused:
				case attr_warn_unused:
				case attr_enum_bitmask:
				case attr_noreturn:
				case attr_noderef:
				case attr_packed:
				case attr_weak:
				case attr_desig_init:
				case attr_ucc_debug:
				case attr_always_inline:
				case attr_noinline:
					/* equal */
					break;
			}
		}

		/* both null? */
		return a == b;
}

int attribute_is_typrop(attribute *attr)
{
	switch(attr->type){
#define NAME(nam, typrop) case attr_ ## nam: return typrop;
#define ALIAS(str, nam, typrop) case attr_ ## nam: return typrop;
#define EXTRA_ALIAS(str, nam)
		ATTRIBUTES
#undef NAME
#undef ALIAS
#undef EXTRA_ALIAS
		case attr_LAST:
			break;
	}
	assert(0);
}

void attribute_debug_check(struct attribute *attr)
{
	for(; attr; attr = attr->next){
		if(attr->type == attr_ucc_debug && !attr->bits.ucc_debugged){
			attr->bits.ucc_debugged = 1;

			warn_at(&attr->where, "debug attribute handled");
		}
	}
}
