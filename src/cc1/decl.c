#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "../util/platform.h"
#include "data_structs.h"

decl_desc *decl_desc_new(enum decl_desc_type t)
{
	decl_desc *dp = umalloc(sizeof *dp);
	where_new(&dp->where);
	dp->type = t;
	return dp;
}

void decl_desc_free(decl_desc *dp)
{
	if(dp->type == decl_desc_func){
		funcargs_free(dp->bits.func, 1);
	}
	free(dp);
}

decl_desc *decl_desc_ptr_new()
{
	return decl_desc_new(decl_desc_ptr);
}

decl_desc *decl_desc_func_new()
{
	return decl_desc_new(decl_desc_func);
}

decl_desc *decl_desc_array_new()
{
	return decl_desc_new(decl_desc_array);
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
	switch(dp->type){
		case decl_desc_ptr:
			if(dp->child)
				ret->child = decl_desc_copy(dp->child);
		default:
			break;
	}
	return ret;
}

decl *decl_copy(decl *d)
{
	decl *ret = umalloc(sizeof *ret);
	memcpy(ret, d, sizeof *ret);
	ret->type = type_copy(d->type);
	if(d->desc)
		ret->desc = decl_desc_copy(d->desc);
	/*ret->spel = NULL;*/
	return ret;
}

int decl_desc_size(decl_desc *dp)
{
	switch(dp->type){
		case decl_desc_ptr:
			return platform_word_size();

		case decl_desc_func:
			ICE("can't return decl size for function");

		case decl_desc_array:
		{
			ICE("TODO: array size");
#if 0
			const int siz = type_size(dp->type);
			decl_desc *dp;
			int ret = 0;

			for(dp = d->desc; dp; dp = dp->child)
				if(dp->array_size){
					/* should've been folded fully */
					long v = dp->array_size->val.iv.val;
					if(!v)
						v = platform_word_size(); /* int x[0] - the 0 is a sentinel */
					ret += v * siz;
				}

			return ret;
#endif
		}
	}

	ICE("hi");
	return 0;
}

int decl_size(decl *d)
{
	if(d->desc) /* pointer */
		return decl_desc_size(d->desc);

	if(d->field_width)
		return d->field_width;

	return type_size(d->type);
}

int decl_desc_equal(decl_desc *dpa, decl_desc *dpb)
{
	/* if we are assigning from const, target must be const */
	if(dpb->type != dpb->type)
		return 0;

	if(dpb->type == decl_desc_ptr){
		if(dpb->bits.ptr.qual & qual_const ? dpa->bits.ptr.qual & qual_const : 0)
			return 0;
	}

	if(dpa->child)
		return dpb->child && decl_desc_equal(dpa->child, dpb->child);

	return !dpb->child;
}

int decl_equal(decl *a, decl *b, enum decl_cmp mode)
{
#define VOID_PTR(d) (                   \
			d->type->primitive == type_void   \
			&&  d->desc                   \
			&& !d->desc->child            \
		)

	if((mode & DECL_CMP_ALLOW_VOID_PTR) && (VOID_PTR(a) || VOID_PTR(b)))
		return 1; /* one side is void * */

	if(!type_equal(a->type, b->type, mode & DECL_CMP_STRICT_PRIMITIVE))
		return 0;

	if(a->desc){
		if(b->desc)
			return decl_desc_equal(a->desc, b->desc);
	}else if(!b->desc){
		return 1;
	}

	return 0;
}

int decl_ptr_depth(decl *d)
{
	decl_desc *dp;
	int i = 0;

	for(dp = d->desc; dp; dp = dp->child)
		i++;

	return i;
}

decl_desc **decl_leaf(decl *d)
{
	decl_desc **dp;
	UCC_ASSERT(d, "null decl param");
	for(dp = &d->desc; *dp; dp = &(*dp)->child);
	return dp;
}

int decl_is_struct_or_union(decl *d)
{
	return d->type->primitive == type_struct || d->type->primitive == type_union;
}

int decl_is_const(decl *d)
{
	/* const char *x is not const. char *const x is */
	decl_desc *dp = *decl_leaf(d);
	if(dp)
		return dp->type == decl_desc_ptr && dp->bits.ptr.qual & qual_const;
	return d->type->qual & qual_const;
}

decl *decl_desc_depth_inc(decl *d)
{
	*decl_leaf(d) = decl_desc_ptr_new();
	return d;
}

decl *decl_desc_depth_dec(decl *d)
{
	/* if we are derefing a function pointer, move its func args up to the decl */
	decl_desc *ultimate, *prev;

	/*desc_desc_last_two(d, &penultimate, &ultimate);*/
	for(ultimate = d->desc, prev = NULL; ultimate->child; prev = ultimate, ultimate = ultimate->child);

	UCC_ASSERT(ultimate && ultimate->type == decl_desc_ptr, "trying to deref non-ptr");

	if(prev)
		prev->child = NULL;
	else
		d->desc = NULL;

	decl_desc_free(ultimate);

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

int decl_has_array(decl *d)
{
	decl_desc *dp;
	for(dp = d->desc; dp; dp = dp->child)
		if(dp->type == decl_desc_array)
			return 1;
	return 0;
}

void decl_func_deref(decl *d, funcargs **pfuncargs)
{
	decl_desc **pdp = decl_leaf(d);
	decl_desc *dp;

	dp = *pdp;
	UCC_ASSERT(dp, "can't call nil decl desc");

	if(dp->type == decl_desc_func){
		*pdp = NULL;

		*pfuncargs = dp->bits.func;

		decl_desc_free(dp);
	}else if(dp->type == decl_desc_ptr){
		/* look one up for func ptr */
		decl_desc *pre, **before;

		for(before = &d->desc, pre = d->desc;
				pre->child != dp;
				before = &pre->child, pre = pre->child);

		if(pre->type == decl_desc_func){
			*before = NULL; /* snip */

			*pfuncargs = dp->bits.func;

			decl_desc_free(dp);
			decl_desc_free(pre);
		}else{
			goto cant;
		}
	}else{
cant:
		ICE("can't func-deref non func decl desc");
	}
}

const char *decl_to_str(decl *d)
{
	static char buf[DECL_STATIC_BUFSIZ];
	char *bufp = buf;
	decl_desc *dp;

#define BUF_ADD(...) \
	bufp += snprintf(bufp, sizeof(buf) - (bufp - buf), __VA_ARGS__)


	BUF_ADD("%s%s", type_to_str(d->type), d->desc ? " " : "");

	for(dp = d->desc; dp; dp = dp->child)
		BUF_ADD("TODO");

	return buf;
}
