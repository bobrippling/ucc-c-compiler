#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../util/where.h"
#include "../util/alloc.h"
#include "../util/warn.h"
#include "../util/dynarray.h"

#include "cc1_where.h"
#include "sue.h"
#include "decl.h"
#include "expr.h"
#include "warn.h"

#include "type_is.h"

#include "ops/expr_cast.h"
#include "ops/expr_identifier.h"

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

attribute *attr_present(attribute **attrs, enum attribute_type t)
{
	for(; attrs && *attrs; attrs++)
		if((*attrs)->type == t)
			return *attrs;

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
#define NAME(x, typrop, cat) case attr_ ## x: return #x;
#define ALIAS(s, x, typrop, cat) case attr_ ## x: return s;
#define EXTRA_ALIAS(s, x, cat)
		ATTRIBUTES
#undef NAME
#undef ALIAS
#undef EXTRA_ALIAS
		case attr_LAST:
			break;
	}
	return NULL;
}

void attribute_free(attribute *a)
{
	switch(a->type){
		case attr_LAST:
			assert(0);

		case attr_alias:
			free(a->bits.alias);
			break;

		case attr_format:
		case attr_cleanup:
		case attr_section:
		case attr_call_conv:
		case attr_nonnull:
		case attr_sentinel:
		case attr_aligned:
		case attr_constructor:
		case attr_destructor:
		case attr_visibility:
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
		case attr_no_stack_protector:
		case attr_stack_protect:
			break;
	}

	free(a);
}

void attribute_array_release(attribute ***array)
{
	attribute **i;

	for(i = *array; i && *i; i++)
		RELEASE(*i);

	dynarray_free(attribute **, *array, NULL);
}

struct attribute **attribute_array_retain(struct attribute **attrs)
{
	attribute **i;

	for(i = attrs; i && *i; i++)
		(void)RETAIN(*i);

	return attrs;
}

int attribute_equal(attribute *a, attribute *b)
{
	if(a->type != b->type)
		return 0;

	switch(a->type){
		case attr_LAST:
			assert(0);
		case attr_format:
#define NEQ(bit, memb) (a->bits.bit.memb != b->bits.bit.memb)
			if(NEQ(format, fmt_func)
			|| NEQ(format, fmt_idx)
			|| NEQ(format, var_idx))
			{
				return 0;
			}
#undef NEQ
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

		case attr_constructor:
		case attr_destructor:
			if(a->bits.priority != b->bits.priority)
				return 0;
			break;

		case attr_visibility:
			if(a->bits.visibility != b->bits.visibility)
				return 0;
			break;

		case attr_unused:
		case attr_warn_unused:
		case attr_enum_bitmask:
		case attr_noreturn:
		case attr_noderef:
		case attr_packed:
		case attr_weak:
		case attr_alias:
		case attr_desig_init:
		case attr_ucc_debug:
		case attr_always_inline:
		case attr_noinline:
		case attr_no_stack_protector:
		case attr_stack_protect:
			/* equal */
			break;
	}

	return 1;
}

int attribute_is_typrop(attribute *attr)
{
	switch(attr->type){
#define NAME(nam, typrop, cat) case attr_ ## nam: return typrop;
#define ALIAS(str, nam, typrop, cat) case attr_ ## nam: return typrop;
#define EXTRA_ALIAS(str, nam, cat)
		ATTRIBUTES
#undef NAME
#undef ALIAS
#undef EXTRA_ALIAS
		case attr_LAST:
			break;
	}
	assert(0);
}

void attribute_debug_check(struct attribute **attrs)
{
	for(; attrs && *attrs; attrs++){
		attribute *attr = *attrs;

		if(attr->type == attr_ucc_debug && !attr->bits.ucc_debugged){
			attr->bits.ucc_debugged = 1;

			warn_at(&attr->where, "debug attribute handled");
		}
	}
}

static enum attribute_category attribute_category(attribute *attr)
{
	switch(attr->type){
#define NAME(nam, typrop, cat) case attr_ ## nam: return cat;
#define ALIAS(spel, nam, typrop, cat) NAME(nam, typrop, cat)
#define EXTRA_ALIAS(spel, nam, cat)
		ATTRIBUTES
#undef NAME
#undef ALIAS
#undef EXTRA_ALIAS

		case attr_LAST:
			break;
	}
	assert(0);
	return attribute_cat_any;
}

static void append(char *buf, const char *str)
{
	if(*buf)
		strcat(buf, "/");
	strcat(buf, str);
}

static const char *attribute_category_to_str_r(int bufno, enum attribute_category mask)
{
	static char bufs[2][32];
	char *buf = bufs[bufno];

	*buf = '\0';

	if(mask & attribute_cat_type_funconly){
		append(buf, "function type");
	}else if(mask & attribute_cat_type_enumonly){
		append(buf, "enum");
	}else if(mask & attribute_cat_type_enumentonly){
		append(buf, "enum member");
	}else if(mask & attribute_cat_type_ptronly){
		append(buf, "pointer type");
	}else if(mask & attribute_cat_type_structonly){
		append(buf, "struct");
	}else if(mask & attribute_cat_type){
		append(buf, "type");
	}

	if(*buf)
		return buf;

	if(mask & attribute_cat_decl_funconly){
		append(buf, "function");
	}else if(mask & attribute_cat_decl_varonly){
		append(buf, "variable");
	}else if(mask & attribute_cat_decl){
		append(buf, "declaration");
	}

	if(mask & attribute_cat_label)
		append(buf, "label");

	return buf;
}

int attribute_verify_cat(
		enum attribute_category current,
		enum attribute_category constraint,
		where *loc)
{
	int is_okay = !!(current & constraint);

	if(!is_okay){
		cc1_warn_at(loc, ignored_attribute,
				"ignoring attribute - expected %s attribute, got %s attribute",
				attribute_category_to_str_r(0, constraint),
				attribute_category_to_str_r(1, current));
	}

	return is_okay;
}

static void attribute_verify(attribute ***attrs, enum attribute_category constraint)
{
	size_t i;
	attribute *attr;

	if(!*attrs)
		return;

	for(i = 0; *attrs && (attr = (*attrs)[i]); ){
		int is_okay = attribute_verify_cat(attribute_category(attr), constraint, &attr->where);

		if(!is_okay){
			dynarray_rm(attrs, attr);
			RELEASE(attr);
		}else{
			i++;
		}
	}
}

static enum attribute_category attribute_constraints_for_type(type *ty)
{
	if(type_is(ty, type_func))
		return attribute_cat_type_funconly;
	if(type_is_s_or_u(ty))
		return attribute_cat_type_structonly;
	if(type_is_ptr(ty))
		return attribute_cat_type_ptronly;
	if(type_is_enum(ty))
		return attribute_cat_type_enumonly;

	return attribute_cat_type;
}

static enum attribute_category attribute_constraints_for_decl(decl *d)
{
	if(type_is(d->ref, type_func))
		return attribute_cat_decl_funconly;
	return attribute_cat_decl_varonly;
}

void attribute_verify_type(attribute ***attrs, type *ty)
{
	attribute_verify(attrs, attribute_constraints_for_type(ty));
}

void attribute_verify_decl(attribute ***attrs, decl *d)
{
	attribute_verify(attrs, attribute_constraints_for_type(d->ref) | attribute_constraints_for_decl(d));
}
