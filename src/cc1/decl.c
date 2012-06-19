#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "../util/platform.h"
#include "../util/dynarray.h"
#include "data_structs.h"
#include "macros.h"

#define ITER_DESC_TYPE(d, dp, typ)     \
	for(dp = d->desc; dp; dp = dp->child) \
		if(dp->type == typ)

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

decl_desc *decl_desc_block_new(decl *dparent, decl_desc *parent)
{
	return decl_desc_new(decl_desc_block, dparent, parent);
}

decl_desc *decl_desc_func_new(decl *dparent, decl_desc *parent)
{
	return decl_desc_new(decl_desc_func, dparent, parent);
}

decl_desc *decl_desc_array_new(decl *dparent, decl_desc *parent)
{
	return decl_desc_new(decl_desc_array, dparent, parent);
}

void decl_desc_insert(decl *d, decl_desc *new)
{
	UCC_ASSERT(!new->child, "child on insert");
	new->child = d->desc;
	d->desc = new;
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

void decl_free(decl *d)
{
	type_free(d->type);
	decl_free_notype(d);
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

void decl_attr_append(decl_attr **loc, decl_attr *new)
{
	while((*loc))
		loc = &(*loc)->next;
	*loc = new;
}

int decl_attr_present(decl_attr *da, enum decl_attr_type t)
{
	for(; da; da = da->next)
		if(da->type == t)
			return 1;
	return 0;
}

const char *decl_attr_to_str(enum decl_attr_type t)
{
	switch(t){
		CASE_STR_PREFIX(attr, format);
		CASE_STR_PREFIX(attr, unused);
		CASE_STR_PREFIX(attr, warn_unused);
		CASE_STR_PREFIX(attr, section);
		CASE_STR_PREFIX(attr, enum_bitmask);
	}
	return NULL;
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

static void decl_copy_desc_if(decl *to, decl *from)
{
	UCC_ASSERT(!to->desc, "desc for target copy: %s", decl_to_str(to));

	if(from->desc){
		to->desc = decl_desc_copy(from->desc);
		to->desc->parent_decl = to;
	}
}

decl *decl_copy(decl *d)
{
	decl *ret = umalloc(sizeof *ret);
	memcpy(ret, d, sizeof *ret);
	ret->type = type_copy(d->type);
	ret->desc = NULL;
	decl_copy_desc_if(ret, d);
	return ret;
}

void decl_copy_primitive(decl *dto, decl *dfrom)
{
	type *const to = dto->type, *const from = dfrom->type;

	to->primitive = from->primitive;
	to->sue       = from->sue;
	to->typeof    = from->typeof;

	decl_copy_desc_if(dto, dfrom);
}

int decl_size(decl *d)
{
	int mul = 1;

	if(d->field_width){
		ICW("use of struct field width - brace for incorrect code (%s)",
				where_str(&d->where));
		return d->field_width;
	}

	if(d->desc){
		/* find the lowest, start working our way up */
		decl_desc *dp;
		int had_ptr = 0;

		for(dp = decl_desc_tail(d); dp; dp = dp->parent_desc)
			switch(dp->type){
				case decl_desc_ptr:
				case decl_desc_block:
					had_ptr = 1;
					break;

				case decl_desc_func:
					break;

				case decl_desc_array:
				{
					int sz;
					/* don't check dp->bits.array_size - it could be any expr */
					sz = dp->bits.array_size->val.iv.val;
					UCC_ASSERT(sz, "incomplete array size attempt");
					mul *= sz;
					break;
				}
			}

		/* pointer to a type, the size is the size of a pointer, not the type */
		if(had_ptr)
			return mul * platform_word_size();
	}

	return mul * type_size(d->type);
}

int funcargs_equal(funcargs *args_to, funcargs *args_from, int strict_types, int *idx)
{
	const enum decl_cmp flag = DECL_CMP_ALLOW_VOID_PTR | (strict_types ? DECL_CMP_STRICT_PRIMITIVE : 0);
	const int count_to = dynarray_count((void **)args_to->arglist);
	const int count_from = dynarray_count((void **)args_from->arglist);
	int i;

	if((count_to == 0 && !args_to->args_void) || (count_from == 0 && !args_from->args_void)){
		/* a() or b() */
		return 1;
	}else if(!(args_to->variadic ? count_to <= count_from : count_to == count_from)){

#ifdef DEBUG_FUNCARGS
		fprintf(stderr, "variadic %d, count_to %d, count_from %d\n", args_to->variadic, count_to, count_from);
		for(i = 0; args_to->arglist[i]; i++)
			fprintf(stderr, "to [%d] = %s\n", i, decl_to_str(args_to->arglist[i]));
		for(i = 0; args_to->arglist[i]; i++)
			fprintf(stderr, "fr [%d] = %s\n", i, decl_to_str(args_from->arglist[i]));
#endif

		if(idx) *idx = -1;
		return 0;
	}

	if(count_to)
		for(i = 0; args_to->arglist[i]; i++)
			if(!decl_equal(args_to->arglist[i], args_from->arglist[i], flag)){
				if(idx) *idx = i;
				return 0;
			}

	return 1;
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

	if(a->type == decl_desc_func && b->type == decl_desc_func)
		if(!funcargs_equal(a->bits.func, b->bits.func, 1 /* exact match */, NULL))
			return 0;

	if(b->type == decl_desc_ptr){
		/* check const qualifiers */
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
	int strict;

	if((mode & DECL_CMP_ALLOW_VOID_PTR)){
		/* one side is void * */
		if(decl_is_void_ptr(a) && decl_ptr_depth(b))
			return 1;
		if(decl_is_void_ptr(b) && decl_ptr_depth(a))
			return 1;
	}

	/* we are strict if told, or if either are a pointer - types must be equal */
	strict = (mode & DECL_CMP_STRICT_PRIMITIVE) || a->desc || b->desc;

	if(!type_equal(a->type, b->type, strict))
		return 0;

	return a->desc ? b->desc && decl_desc_equal(a->desc, b->desc) : !b->desc;
}

int decl_ptr_depth(decl *d)
{
	decl_desc *dp;
	int i = 0;

	for(dp = d->desc; dp; dp = dp->child)
		switch(dp->type){
			case decl_desc_ptr:
			case decl_desc_array:
				i++;
			case decl_desc_func:
			case decl_desc_block:
				break;
		}

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

int decl_is_struct_or_union_possible_ptr(decl *d)
{
	return d->type->primitive == type_struct || d->type->primitive == type_union;
}

int decl_is_struct_or_union(decl *d)
{
	return decl_is_struct_or_union_possible_ptr(d) && !decl_ptr_depth(d);
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
		DIE_AT(from,
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

int decl_is_integral(decl *d)
{
	if(d->desc)
		return 0;

	switch(d->type->primitive){
		case type_int:
		case type_char:
				return 1;

		case type_unknown:
		case type_void:
		case type_struct:
		case type_union:
		case type_enum:
				break;
	}

	return 0;
}

int decl_is_callable(decl *d)
{
	decl_desc *dp, *pre;

	for(pre = NULL, dp = d->desc; dp && dp->child; pre = dp, dp = dp->child);

	if(!dp)
		return 0;

	switch(dp->type){
		case decl_desc_block:
		case decl_desc_ptr:
			return pre && pre->type == decl_desc_func; /* ptr to func */

		case decl_desc_func:
			return 1;

		default:
			break;
	}

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

	return dp
		&& prev
		&& prev->type == decl_desc_func
		&& (dp->type == decl_desc_ptr || dp->type == decl_desc_block);
}

int decl_is_array(decl *d)
{
	decl_desc *dp;
	for(dp = d->desc; dp && dp->child; dp = dp->child);
	return dp ? dp->type == decl_desc_array : 0;
}

int decl_has_array(decl *d)
{
	decl_desc *dp;

	ITER_DESC_TYPE(d, dp, decl_desc_array)
		return 1;

	return 0;
}

decl_desc *decl_array_first_incomplete(decl *d)
{
	decl_desc *dp;

	ITER_DESC_TYPE(d, dp, decl_desc_array)
		if(!dp->bits.array_size->val.iv.val)
			return dp;

	return NULL;
}

decl_desc *decl_array_first(decl *d)
{
	decl_desc *dp;

	ITER_DESC_TYPE(d, dp, decl_desc_array)
		return dp;

	return NULL;
}

int decl_has_incomplete_array(decl *d)
{
	decl_desc *tail = decl_desc_tail(d);

	return tail
	&& tail->type == decl_desc_array
	&& tail->bits.array_size->val.iv.val == 0;
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
	funcargs *args;

	for(dp = d->desc; dp->child; dp = dp->child);

	/* should've been caught by is_callable() */
	UCC_ASSERT(dp, "can't call non-function");

	switch(dp->type){
		case decl_desc_func:
			args = dp->bits.func;

			decl_desc_cut_loose(dp);

			decl_desc_free(dp);
			break;

		case decl_desc_ptr:
		case decl_desc_block:
		{
			decl_desc *const func = dp->parent_desc;

			UCC_ASSERT(func, "no parent desc for func-ptr call");

			if(func->type == decl_desc_func){
				args = func->bits.func;

				decl_desc_cut_loose(func);

				decl_desc_free(dp);
				decl_desc_free(func);
			}else{
				goto cant;
			}
			break;
		}

		default:
cant:
			ICE("can't func-deref non func decl desc");
	}

	if(pfuncargs)
		*pfuncargs = args;

	return d;
}

void decl_conv_array_ptr(decl *d)
{
	decl_desc *dp;

	ITER_DESC_TYPE(d, dp, decl_desc_array){
		expr_free(dp->bits.array_size);
		dp->type = decl_desc_ptr;
		dp->bits.qual = qual_none;
	}
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
		CASE_STR_PREFIX(decl_desc, block);
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

	const int need_paren = dp->parent_desc && dp->parent_desc->type != dp->type;

	if(need_paren)
		BUF_ADD("(");

	switch(dp->type){
		case decl_desc_ptr:
			BUF_ADD("*%s",
					type_qual_to_str(dp->bits.qual));
			break;
		case decl_desc_block:
			BUF_ADD("^");
			break;
		default:
			break;
	}

	if(dp->child)
		decl_desc_add_str(dp->child, bufp, sz);

	switch(dp->type){
		case decl_desc_block:
		case decl_desc_ptr:
			break;
		case decl_desc_func:
		{
			const char *comma = "";
			decl **i;
			funcargs *args = dp->bits.func;

			BUF_ADD("(");
			for(i = args->arglist; i && *i; i++){
				char tmp_buf[DECL_STATIC_BUFSIZ];
				BUF_ADD("%s%s", comma, decl_to_str_r(tmp_buf, *i));
				comma = ", ";
			}
			BUF_ADD("%s)", args->variadic ? ", ..." : args->args_void ? "void" : "");
			break;
		}
		case decl_desc_array:
			BUF_ADD("[%ld]", dp->bits.array_size->val.iv.val);
			break;
	}

	if(need_paren)
		BUF_ADD(")");
#undef BUF_ADD
}

const char *decl_to_str_r(char buf[DECL_STATIC_BUFSIZ], decl *d)
{
	char *bufp = buf;

#define BUF_ADD(...) \
	bufp += snprintf(bufp, DECL_STATIC_BUFSIZ - (bufp - buf), __VA_ARGS__)

	BUF_ADD("%s%s", type_to_str(d->type), d->desc ? " " : "");

	if(d->desc)
		decl_desc_add_str(d->desc, &bufp, DECL_STATIC_BUFSIZ - (bufp - buf));

	return buf;
#undef BUF_ADD
}

const char *decl_to_str(decl *d)
{
	static char buf[DECL_STATIC_BUFSIZ];
	return decl_to_str_r(buf, d);
}
