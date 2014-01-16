#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "../util/platform.h"
#include "../util/dynarray.h"

#include "cc1_where.h"

#include "macros.h"
#include "sue.h"
#include "const.h"
#include "cc1.h"
#include "fold.h"
#include "funcargs.h"
#include "defs.h"

#include "type_is.h"

decl *decl_new_w(const where *w)
{
	decl *d = umalloc(sizeof *d);
	memcpy_safe(&d->where, w);
	return d;
}

decl *decl_new()
{
	where wtmp;
	where_cc1_current(&wtmp);
	return decl_new_w(&wtmp);
}

decl *decl_new_ty_sp(type *ty, char *sp)
{
	decl *d = decl_new();
	d->ref = ty;
	d->spel = sp;
	return d;
}

void decl_replace_with(decl *to, decl *from)
{
	/* XXX: memleak of .ref */
	memcpy_safe(&to->where, &from->where);
	to->ref      = from->ref;
	to->attr     = from->attr;
	to->spel_asm = from->spel_asm;
	/* no point copying bitfield stuff */
	to->align    = from->align;
}

const char *decl_asm_spel(decl *d)
{
	if(!d->spel_asm){
		/* apply underscore prefixes, name mangling, etc */
		type *rf = DECL_IS_FUNC(d);
		char *pre, suff[8];

		pre = fopt_mode & FOPT_LEADING_UNDERSCORE ? "_" : "";
		*suff = '\0';

		if(rf){
			funcargs *fa = type_funcargs(rf);

			switch(fa->conv){
				case conv_fastcall:
					pre = "@";

				case conv_stdcall:
					snprintf(suff, sizeof suff,
							"@%d",
							dynarray_count(fa->arglist) * platform_word_size());

				case conv_x64_sysv:
				case conv_x64_ms:
				case conv_cdecl:
					break;
			}
		}

		if(*pre || *suff)
			d->spel_asm = ustrprintf(
					"%s%s%s", pre, d->spel, suff);


		if(!d->spel_asm)
			d->spel_asm = d->spel;
	}

	return d->spel_asm;
}

void decl_free(decl *d)
{
	if(!d)
		return;

	expr_free(d->field_width); /* XXX: bad? */

	free(d);
}

decl_attr *decl_attr_new(enum decl_attr_type t)
{
	decl_attr *da = umalloc(sizeof *da);
	where_cc1_current(&da->where);
	da->type = t;
	return da;
}

void decl_attr_append(decl_attr **loc, decl_attr *new)
{
	/* may be appending from a prototype to a function def. */
	while(*loc)
		loc = &(*loc)->next;

	/* we can just link up, since pointers aren't rewritten now */
	*loc = /*decl_attr_copy(*/new/*)*/;
}

decl_attr *attr_present(decl_attr *da, enum decl_attr_type t)
{
	for(; da; da = da->next)
		if(da->type == t)
			return da;
	return NULL;
}

decl_attr *type_attr_present(type *r, enum decl_attr_type t)
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
		decl_attr *da;

		if((da = attr_present(r->attr, t)))
			return da;

		switch(r->type){
			case type_btype:
			{
				struct_union_enum_st *sue = r->bits.type->sue;
				if((da = attr_present(r->bits.type->attr, t)))
					return da;
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

decl_attr *decl_attr_present(decl *d, enum decl_attr_type t)
{
	/* check the attr on the decl _and_ its type */
	decl_attr *da;
	if((da = attr_present(d->attr, t)))
		return da;
	if((da = type_attr_present(d->ref, t)))
		return da;

	return d->proto ? decl_attr_present(d->proto, t) : NULL;
}

decl_attr *expr_attr_present(expr *e, enum decl_attr_type t)
{
	decl_attr *da;

	if(expr_kind(e, cast)){
		da = expr_attr_present(e->expr, t);
		if(da)
			return da;
	}

	if(expr_kind(e, identifier)){
		sym *s = e->bits.ident.sym;
		if(s){
			da = decl_attr_present(s->decl, t);
			if(da)
				return da;
		}
	}

	return type_attr_present(e->tree_type, t);
}

const char *decl_attr_to_str(decl_attr *da)
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

const char *decl_store_to_str(const enum decl_storage s)
{
	static char buf[16]; /* "inline register" is the longest - just a fit */

	if(s & STORE_MASK_EXTRA){
		*buf = '\0';

		if((s & STORE_MASK_EXTRA) == store_inline)
			strcpy(buf, "inline ");

		strcpy(buf + strlen(buf), decl_store_to_str(s & STORE_MASK_STORE));
		return buf;
	}

	switch(s){
		case store_inline:
			ICE("inline");
		case store_default:
			return "";
		CASE_STR_PREFIX(store, auto);
		CASE_STR_PREFIX(store, static);
		CASE_STR_PREFIX(store, extern);
		CASE_STR_PREFIX(store, register);
		CASE_STR_PREFIX(store, typedef);
	}
	return NULL;
}

void decl_attr_free(decl_attr *a)
{
	if(!a)
		return;

	decl_attr_free(a->next);
	free(a);
}

unsigned decl_size(decl *d)
{
	if(type_is_void(d->ref))
		die_at(&d->where, "%s is void", d->spel);

	if(d->field_width)
		die_at(&d->where, "can't take size of a bitfield");

	return type_size(d->ref, &d->where);
}

unsigned decl_align(decl *d)
{
	unsigned al = 0;

	if(d->align)
		al = d->align->resolved;

	return al ? al : type_align(d->ref, &d->where);
}

enum type_cmp decl_cmp(decl *a, decl *b, enum type_cmp_opts opts)
{
	enum type_cmp cmp = type_cmp(a->ref, b->ref, opts);
	enum decl_storage sa = a->store & STORE_MASK_STORE,
	                  sb = b->store & STORE_MASK_STORE;

	if(cmp & TYPE_EQUAL_ANY && sa != sb){
		/* types are equal but there's a store mismatch
		 * only return convertible if it's a typedef or static mismatch
		 */
#define STORE_INCOMPAT(st) ((st) == store_typedef || (st) == store_static)

		if(STORE_INCOMPAT(sa) || STORE_INCOMPAT(sb))
			return TYPE_CONVERTIBLE_IMPLICIT;
	}

	return cmp;
}

int decl_conv_array_func_to_ptr(decl *d)
{
	type *old = d->ref;

	d->ref = type_decay(d->ref);

	return old != d->ref;
}

type *decl_is_decayed_array(decl *d)
{
	return type_is_decayed_array(d->ref);
}

int decl_store_static_or_extern(enum decl_storage s)
{
	switch((enum decl_storage)(s & STORE_MASK_STORE)){
		case store_static:
		case store_extern:
			return 1;
		default:
			return 0;
	}
}

const char *decl_to_str_r(char buf[DECL_STATIC_BUFSIZ], decl *d)
{
	char *bufp = buf;

	if(d->store)
		bufp += snprintf(bufp, DECL_STATIC_BUFSIZ, "%s ", decl_store_to_str(d->store));

	type_to_str_r_spel(bufp, d->ref, d->spel);

	return buf;
}

const char *decl_to_str(decl *d)
{
	static char buf[DECL_STATIC_BUFSIZ];
	return decl_to_str_r(buf, d);
}
