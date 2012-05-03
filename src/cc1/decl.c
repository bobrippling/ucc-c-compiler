#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "../util/platform.h"
#include "data_structs.h"
#include "macros.h"

decl_desc *decl_desc_new(enum decl_desc_type t, decl *dparent, decl_desc *parent)
{
	decl_desc *dp = umalloc(sizeof *dp);
	where_new(&dp->where);
	dp->type = t;
	dp->parent_decl = dparent;
	dp->parent_desc = parent;
	return dp;
}

void decl_desc_free(decl_desc *dp)
{
	free(dp);
}

decl_desc *decl_desc_ptr_new(decl *dparent, decl_desc *parent)
{
	return decl_desc_new(decl_desc_ptr, dparent, parent);
}

decl_desc *decl_desc_func_new(decl *dparent, decl_desc *parent)
{
	return decl_desc_new(decl_desc_func, dparent, parent);
}

decl_desc *decl_desc_array_new(decl *dparent, decl_desc *parent)
{
	return decl_desc_new(decl_desc_array, dparent, parent);
}

void decl_desc_append(decl_desc **pparent, decl_desc *child)
{
	decl_desc *parent = *pparent;

	if(!parent){
		*pparent = child;
		return;
	}

	for(; parent->child; parent = parent->child);
	parent->child = child;
}

decl_desc *decl_desc_tail(decl *d)
{
	decl_desc *i;
	for(i = d->desc; i && i->child; i = i->child);
	return i;
}

decl *decl_new()
{
	decl *d = umalloc(sizeof *d);
	where_new(&d->where);
	d->type = type_new();
	return d;
}

array_decl *array_decl_new()
{
	array_decl *ad = umalloc(sizeof *ad);
	return ad;
}

decl_attr *decl_attr_new(enum decl_attr_type t)
{
	decl_attr *da = umalloc(sizeof *da);
	where_new(&da->where);
	da->type = t;
	return da;
}

int decl_attr_present(decl_attr *da, enum decl_attr_type t)
{
	for(; da; da = da->next)
		if(da->type == t)
			return 1;
	return 0;
}

decl_desc *decl_desc_copy(decl_desc *dp)
{
	decl_desc *ret = umalloc(sizeof *ret);
	memcpy(ret, dp, sizeof *ret);
	if(dp->child){
		ret->child = decl_desc_copy(dp->child);
		ret->child->parent_desc = ret;
	}
	return ret;
}

decl *decl_copy(decl *d)
{
	decl *ret = umalloc(sizeof *ret);
	memcpy(ret, d, sizeof *ret);
	ret->type = type_copy(d->type);
	if(d->desc){
		ret->desc = decl_desc_copy(d->desc);
		ret->desc->parent_decl = ret;
	}
	return ret;
}

int decl_size(decl *d)
{
	int mul = 1;

	if(d->field_width)
		return d->field_width;

	if(d->desc){
		/* find the lowest, start working our way up */
		decl_desc *dp;
		int had_ptr = 0;

		for(dp = decl_desc_tail(d); dp; dp = dp->parent_desc)
			switch(dp->type){
				case decl_desc_ptr:
					had_ptr = 1;
					break;

				case decl_desc_func:
					break;

				case decl_desc_array:
					UCC_ASSERT(expr_kind(dp->bits.array_size, val), "decl array size not constant");
					mul *= dp->bits.array_size->val.iv.val;
					break;
			}

		/* pointer to a type, the size is the size of a pointer, not the type */
		if(had_ptr)
			return mul * platform_word_size();
	}

	return mul * type_size(d->type);
}

int decl_desc_equal(decl_desc *a, decl_desc *b)
{
	/* if we are assigning from const, target must be const */
	if(a->type != b->type){
		/* can assign to int * from int [] */
		if(a->type != decl_desc_ptr || b->type != decl_desc_array)
			return 0;
	}

	/* allow a to be "type (*)()" and b to be "type ()" */
	if(a->type == decl_desc_func && a->child && a->child->type == decl_desc_ptr){
		if(b->type == decl_desc_func){
			if(a->child->child && b->child)
				return decl_desc_equal(a->child->child, b->child);
			return 1;
		}
	}

	if(b->type == decl_desc_ptr){
		if(a->type != decl_desc_ptr || (b->bits.qual & qual_const ? a->bits.qual & qual_const : 0))
			return 0;
	}

	if(a->child)
		return b->child && decl_desc_equal(a->child, b->child);

	return !b->child;
}

int decl_is_void_ptr(decl *d)
{
	return d->type->primitive == type_void
		&& d->desc
		&& d->desc->type == decl_desc_ptr
		&& !d->desc->child;
}

int decl_equal(decl *a, decl *b, enum decl_cmp mode)
{
	if((mode & DECL_CMP_ALLOW_VOID_PTR) && (decl_is_void_ptr(a) || decl_is_void_ptr(b)))
		return 1; /* one side is void * */

	if(!type_equal(a->type, b->type, mode & DECL_CMP_STRICT_PRIMITIVE))
		return 0;

	return a->desc ? b->desc && decl_desc_equal(a->desc, b->desc) : !b->desc;
}

int decl_ptr_depth(decl *d)
{
	decl_desc *dp;
	int i = 0;

	for(dp = d->desc; dp; dp = dp->child)
		if(dp->type == decl_desc_ptr)
			i++;

	return i;
}

int decl_desc_depth(decl *d)
{
	decl_desc *dp;
	int i = 0;
	for(dp = d->desc; dp; dp = dp->child)
		i++;
	return i;
}

decl_desc *decl_leaf(decl *d)
{
	decl_desc *dp;

	if(!d->desc)
		return NULL;

	for(dp = d->desc; dp->child; dp = dp->child);

	return dp;
}

funcargs *decl_funcargs(decl *d)
{
	decl_desc *dp;
	for(dp = d->desc; dp->type != decl_desc_func && dp->child; dp = dp->child);
	return dp->bits.func;
}

int decl_is_struct_or_union(decl *d)
{
	return d->type->primitive == type_struct || d->type->primitive == type_union;
}

int decl_is_const(decl *d)
{
	/* const char *x is not const. char *const x is */
	decl_desc *dp = decl_leaf(d);
	if(dp)
		return dp->type == decl_desc_ptr && dp->bits.qual & qual_const;
	return d->type->qual & qual_const;
}

decl *decl_ptr_depth_inc(decl *d)
{
	decl_desc **p, *prev;

	for(prev = NULL, p = &d->desc; *p; prev = *p, p = &(*p)->child);

	*p = decl_desc_ptr_new(d, prev);

	return d;
}

decl *decl_ptr_depth_dec(decl *d, where *from)
{
	/* if we are derefing a function pointer, move its func args up to the decl */
	decl_desc *last;

	for(last = d->desc; last && last->child; last = last->child);

	if(!last || (last->type != decl_desc_ptr && last->type != decl_desc_array)){
		die_at(from,
			"trying to dereference non-pointer%s%s%s",
			last ? " (" : "",
			last ? decl_desc_str(last) : "",
			last ? ")"  : "");
	}

	if(last->parent_desc)
		last->parent_desc->child = NULL;
	else
		last->parent_decl->desc = NULL;

	decl_desc_free(last);

	return d;
}

int decl_is_callable(decl *d)
{
	decl_desc *dp, *pre;

	for(pre = NULL, dp = d->desc; dp && dp->child; pre = dp, dp = dp->child);

	if(!dp)
		return 0;

	if(dp->type == decl_desc_func)
		return 1;

	if(dp->type == decl_desc_ptr)
		return pre && pre->type == decl_desc_func; /* ptr to func */

	return 0;
}

int decl_is_func(decl *d)
{
	decl_desc *dp;
	for(dp = d->desc; dp && dp->child; dp = dp->child);
	return dp && dp->type == decl_desc_func;
}

int decl_is_fptr(decl *d)
{
	decl_desc *dp, *prev;

	for(prev = NULL, dp = d->desc;
			dp && dp->child;
			prev = dp, dp = dp->child);

	return dp && prev && dp->type == decl_desc_ptr && prev->type == decl_desc_func;
}

int decl_has_array(decl *d)
{
	decl_desc *dp;
	for(dp = d->desc; dp; dp = dp->child)
		if(dp->type == decl_desc_array)
			return 1;
	return 0;
}

void decl_desc_cut_loose(decl_desc *dp)
{
	if(dp->parent_desc)
		dp->parent_desc->child = NULL;
	else
		dp->parent_decl->desc = NULL;
}

decl *decl_func_deref(decl *d, funcargs **pfuncargs)
{
	decl_desc *dp;

	for(dp = d->desc; dp->child; dp = dp->child);

	/* should've been caught by is_callable() */
	UCC_ASSERT(dp, "can't call non-function");

	if(dp->type == decl_desc_func){
		*pfuncargs = dp->bits.func;

		decl_desc_cut_loose(dp);

		decl_desc_free(dp);
	}else if(dp->type == decl_desc_ptr){
		decl_desc *const func = dp->parent_desc;
		UCC_ASSERT(func, "no parent desc for func-ptr call");

		if(func->type == decl_desc_func){
			*pfuncargs = func->bits.func;

			decl_desc_cut_loose(func);

			decl_desc_free(dp);
			decl_desc_free(func);
		}else{
			goto cant;
		}
	}else{
cant:
		ICE("can't func-deref non func decl desc");
	}

	return d;
}

char *decl_spel(decl *d)
{
#if 0
	decl_desc *dp;

	if(!d->desc)
		return NULL;

	for(dp = d->desc; dp->child; dp = dp->child);

	return NULL;
#endif
	return d->spel;
}

void decl_set_spel(decl *d, char *sp)
{
#if 0
	decl_desc **new, *prev;

	if(d->desc){
		decl_desc *p;
		for(p = d->desc; p->child; p = p->child);

		if(p->type == decl_desc_spel){
			free(p->bits.spel);
			p->bits.spel = sp;
			return;
		}

		prev = p;
		new = &p->child;
	}else{
		prev = NULL;
		new = &d->desc;
	}

	*new = decl_desc_spel_new(d, prev, sp);
#endif
	free(d->spel);
	d->spel = sp;
}

void decl_desc_link(decl *d)
{
	decl_desc *dp, *prev;

	for(dp = d->desc, prev = NULL; dp; prev = dp, dp = dp->child){
		dp->parent_decl = d;
		dp->parent_desc = prev;
	}
}

const char *decl_desc_str(decl_desc *dp)
{
	switch(dp->type){
		CASE_STR_PREFIX(decl_desc, ptr);
		CASE_STR_PREFIX(decl_desc, array);
		CASE_STR_PREFIX(decl_desc, func);
	}
	return NULL;
}

void decl_debug(decl *d)
{
	decl_desc *i;

	fprintf(stderr, "decl %s:\n", d->spel);

	for(i = d->desc; i; i = i->child)
		fprintf(stderr, "\t%s\n", decl_desc_str(i));
}

void decl_desc_add_str(decl_desc *dp, char **bufp, int sz)
{
#define BUF_ADD(...) \
	do{ int n = snprintf(*bufp, sz, __VA_ARGS__); *bufp += n, sz -= n; }while(0)

	switch(dp->type){
		case decl_desc_ptr:
			BUF_ADD("%s*%s",
					dp->parent_desc ? "(" : "",
					type_qual_to_str(dp->bits.qual));
		default:
			break;
	}

	if(dp->child)
		decl_desc_add_str(dp->child, bufp, sz);

	switch(dp->type){
		case decl_desc_ptr:
			if(dp->parent_desc)
				BUF_ADD(")");
			break;
		case decl_desc_func:
			BUF_ADD("()");
			break;
		case decl_desc_array:
			BUF_ADD("[]");
			break;
	}
#undef BUF_ADD
}

const char *decl_to_str(decl *d)
{
	static char buf[DECL_STATIC_BUFSIZ];
	char *bufp = buf;

#define BUF_ADD(...) \
	bufp += snprintf(bufp, sizeof(buf) - (bufp - buf), __VA_ARGS__)

	BUF_ADD("%s%s", type_to_str(d->type), d->desc ? " " : "");

	if(d->desc)
		decl_desc_add_str(d->desc, &bufp, sizeof(buf) - (bufp - buf));

	return buf;
#undef BUF_ADD
}
