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
#include "sue.h"
#include "const.h"
#include "cc1.h"
#include "fold.h"

#define ITER_DESC_TYPE(d, dp, typ)     \
	for(dp = d->desc; dp; dp = dp->child) \
		if(dp->type == typ)


void decl_debug(decl *d);

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

decl_desc *decl_desc_tail(const decl *d)
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

decl *decl_new_type(enum type_primitive p)
{
	decl *d = decl_new();
	d->type->primitive = p;

	switch(p){
		case type_ptrdiff_t:
		case type_intptr_t:
			d->type->is_signed = 0;
		default:
			break;
	}

	return d;
}

void decl_free(decl *d)
{
	type_free(d->type);
	decl_free_notype(d);
}

decl_attr *decl_attr_new(enum decl_attr_type t)
{
	decl_attr *da = umalloc(sizeof *da);
	where_new(&da->where);
	da->type = t;
	return da;
}

decl_attr *decl_attr_copy(decl_attr *da)
{
	decl_attr *ret = decl_attr_new(da->type);

	memcpy(ret, da, sizeof *ret);

	ret->next = da->next ? decl_attr_copy(da->next) : NULL;

	return ret;
}

void decl_attr_append(decl_attr **loc, decl_attr *new)
{
	/*
	 * don't link it up, make copies,
	 * so when we adjust others,
	 * things don't get tangled with links
	 */

	if(new)
		*loc = decl_attr_copy(new);
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
		CASE_STR_PREFIX(attr, noreturn);
		CASE_STR_PREFIX(attr, noderef);
	}
	return NULL;
}

decl_desc *decl_desc_copy(const decl_desc *dp)
{
	decl_desc *ret = umalloc(sizeof *ret);
	memcpy(ret, dp, sizeof *ret);

	if(dp->type == decl_desc_array){
		intval iv;
		const_fold_need_val(dp->bits.array_size, &iv);
		ret->bits.array_size = expr_new_val(iv.val);
	}

	if(dp->child){
		ret->child = decl_desc_copy(dp->child);
		ret->child->parent_desc = ret;
	}

	return ret;
}

static decl *decl_copy_array(const decl *d, int decay_first_array)
{
	decl *ret = umalloc(sizeof *ret);

	memcpy(ret, d, sizeof *ret);

	/* null-out what we don't want to pass on */
	ret->init = NULL;
	ret->spel = NULL;
	ret->func_code = NULL;

	ret->type = type_copy(d->type);

	if(d->desc){
		EOF_WHERE(&d->where,
				decl_desc *inner;

				ret->desc = decl_desc_copy(d->desc);
				decl_desc_link(ret);
				inner = decl_desc_tail(ret);

				if(decay_first_array && inner->type == decl_desc_array){
					/* convert to ptr */
					expr_free(inner->bits.array_size);

					inner->type = decl_desc_ptr;
					inner->bits.qual = qual_none;
				}
		);
	}else{
		ret->desc = NULL;
	}

	return ret;
}

decl *decl_copy_keep_array(const decl *d)
{
	return decl_copy_array(d, 0);
}

decl *decl_copy(const decl *d)
{
	return decl_copy_array(d, 1);
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
		const int word_size = platform_word_size();
		decl_desc *dp;

		dp = decl_desc_tail(d);
		switch(dp->type){
			case decl_desc_ptr:
			case decl_desc_block:
				/* pointer to something, e.g. int (*x)[5] */
				return word_size;

			default:
				break;
		}

		for(; dp; dp = dp->parent_desc)
			switch(dp->type){
				case decl_desc_ptr:
				case decl_desc_block:
				case decl_desc_func:
					break;

				case decl_desc_array:
				{
					intval sz;

					const_fold_need_val(dp->bits.array_size, &sz);

					if(sz.val == 0)
							DIE_AT(&dp->bits.array_size->where, "incomplete array size attempt");

					mul *= sz.val;
					break;
				}
			}
	}

	return mul * type_size(d->type);
}

enum funcargs_cmp funcargs_equal(funcargs *args_to, funcargs *args_from,
		int strict_types, const char *fspel)
{
	const int count_to = dynarray_count((void **)args_to->arglist);
	const int count_from = dynarray_count((void **)args_from->arglist);

	if((count_to   == 0 && !args_to->args_void)
	|| (count_from == 0 && !args_from->args_void)){
		/* a() or b() */
		return funcargs_cmp_equal;
	}

	if(!(args_to->variadic ? count_to <= count_from : count_to == count_from))
		return funcargs_cmp_mismatch_count;

	if(count_to){
		const enum decl_cmp flag =
			DECL_CMP_ALLOW_VOID_PTR | (strict_types ? DECL_CMP_EXACT_MATCH : 0);
		int i;

		for(i = 0; args_to->arglist[i]; i++)
			if(!decl_equal(args_to->arglist[i], args_from->arglist[i], flag)){
				if(fspel){
					char buf[DECL_STATIC_BUFSIZ];

					cc1_warn_at(&args_from->where, 0, 1, WARN_ARG_MISMATCH,
							"mismatching argument %d to %s (%s <-- %s)",
							i, fspel,
							decl_to_str_r(buf,   args_to->arglist[i]),
							decl_to_str(       args_from->arglist[i]));
				}

				return funcargs_cmp_mismatch_types;
			}
	}

	return funcargs_cmp_equal;
}

int decl_desc_equal(decl_desc *a, decl_desc *b, enum decl_cmp mode)
{
	/* if we are assigning from const, target must be const */
	if(a->type != b->type){
		/* can assign to int * from int [] */
		if((mode & DECL_CMP_NO_ARRAY)
		|| a->type != decl_desc_ptr
		|| b->type != decl_desc_array)
		{
			return 0;
		}
	}

	/* XXX: this must be before the auto-cast check below */
	if(a->type == decl_desc_func && b->type == decl_desc_func)
		if(funcargs_cmp_equal != funcargs_equal(a->bits.func, b->bits.func, 1 /* exact match */, NULL))
			return 0;

	/* allow a to be "type (*)()" and b to be "type ()" */
	/* note: we don't accept blocks to be assign from type () */
	if(a->type == decl_desc_func && a->child && a->child->type == decl_desc_ptr){
		/* a is ptr-to-func */

		if(b->type == decl_desc_func && (!b->child || b->child->type != decl_desc_ptr)){
			/* b is func, and not a ptr to it */

			/* attempt to compare children, otherwise assume equal */
			if(a->child->child)
				return decl_desc_equal(a->child->child, b->child, mode);
			return 1;
		}
	}

	if(a->type == decl_desc_ptr){ /* (a == ptr) -> (b == ptr || b == array) */
		/* check qualifiers */
		enum type_qualifier qa, qb;

		qa = a->bits.qual;
		qb = b->bits.qual;

		if(b->type == decl_desc_ptr){
			if(mode & DECL_CMP_EXACT_MATCH){
				if(qa != qb)
					return 0;
			}else{
				/* if qb is const and qa isn't, error */
				if((qb & qual_const) && !(qa & qual_const))
					return 0;
			}
		}
		/* else b is an array */
	}

	if(a->child)
		return b->child && decl_desc_equal(a->child, b->child, mode);

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
	const int a_ptr = decl_is_ptr(a);
	const int b_ptr = decl_is_ptr(b);
	enum type_cmp tmode;

	if(mode & DECL_CMP_ALLOW_VOID_PTR){
		/* one side is void * */
		if(decl_is_void_ptr(a) && b_ptr)
			return 1;
		if(decl_is_void_ptr(b) && a_ptr)
			return 1;
	}

	/* we are exact if told, or if either are a pointer - types must be equal */
	tmode = 0;

	if((mode & DECL_CMP_EXACT_MATCH))
		tmode |= TYPE_CMP_EXACT;

	if(a_ptr || b_ptr)
		tmode |= TYPE_CMP_QUAL;

	if(!type_equal(a->type, b->type, tmode))
		return 0;

	return a->desc ? b->desc && decl_desc_equal(a->desc, b->desc, mode) : !b->desc;
}

int decl_ptr_depth(decl *d)
{
	int depth = 0;
	decl_desc *dp;

	for(dp = d->desc; dp; dp = dp->child)
		switch(dp->type){
			case decl_desc_ptr:
			case decl_desc_array:
				depth++;
				break;
			case decl_desc_block:
			case decl_desc_func:
				break;
		}

	return depth;
}

int decl_is_ptr(decl *d)
{
	decl_desc *dp;

	for(dp = d->desc; dp; dp = dp->child)
		switch(dp->type){
			case decl_desc_ptr:
			case decl_desc_array:
				return 1;
			case decl_desc_func:
			case decl_desc_block:
				break;
		}

	return 0;
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

	for(dp = decl_desc_tail(d); dp; dp = dp->parent_desc)
		if(dp->type == decl_desc_func)
			return dp->bits.func;

	return NULL;
}

int decl_variadic_func(decl *d)
{
	return decl_funcargs(d)->variadic;
}

int decl_is_struct_or_union_possible_ptr(decl *d)
{
	return (d->type->primitive == type_struct || d->type->primitive == type_union);
}

int decl_is_struct_or_union(decl *d)
{
	return decl_is_struct_or_union_possible_ptr(d) && !decl_is_ptr(d);
}

int decl_is_struct_or_union_ptr(decl *d)
{
	return decl_is_struct_or_union_possible_ptr(d) && decl_is_ptr(d);
}

int decl_is_const(decl *d)
{
	/* const char *x is not const. char *const x is */
	decl_desc *dp = decl_leaf(d);
	if(dp)
		switch(dp->type){
			case decl_desc_ptr:
			case decl_desc_block:
				return dp->bits.qual & qual_const;
			default:
				break;
		}

	return d->type->qual & qual_const;
}

decl *decl_ptr_depth_inc(decl *d)
{
	decl_desc **p, *prev;

	for(prev = NULL, p = &d->desc; *p; prev = *p, p = &(*p)->child);

	EOF_WHERE(&d->where,
		*p = decl_desc_ptr_new(d, prev)
	);

	return d;
}

decl *decl_ptr_depth_dec(decl *d, where *from)
{
	decl_desc *last;

	if(decl_is_void_ptr(d))
		DIE_AT(from, "can't dereference %s pointer", decl_to_str(d));

	for(last = d->desc; last && last->child; last = last->child);

	if(!last || (last->type != decl_desc_ptr && last->type != decl_desc_array)){
		DIE_AT(from,
			"trying to dereference %s%s%s%s",
			decl_to_str(d),
			last ? " (" : "",
			last ? decl_desc_to_str(last->type) : "",
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
		case type__Bool:
		case type_short:
		case type_long:
		case type_llong:
		case type_enum:
		case type_intptr_t:
		case type_ptrdiff_t:
				return 1;

		case type_unknown:
		case type_void:
		case type_struct:
		case type_union:
		case type_float:
		case type_double:
		case type_ldouble:
				break;
	}

	return 0;
}

int decl_is_floating(decl *d)
{
	if(d->desc)
		return 0;

	switch(d->type->primitive){
		case type_float:
		case type_double:
		case type_ldouble:
			return 1;
		default:
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
	decl_desc *dp = decl_leaf(d);
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
	decl_desc *dp = decl_desc_tail(d);
	return dp ? dp->type == decl_desc_array : 0;
}

int decl_has_array(decl *d)
{
	decl_desc *dp;

	ITER_DESC_TYPE(d, dp, decl_desc_array)
		return 1;

	return 0;
}

int decl_has_incomplete_array(decl *d)
{
	decl_desc *tail = decl_desc_tail(d);

	if(tail && tail->type == decl_desc_array){
		intval iv;

		const_fold_need_val(tail->bits.array_size, &iv);

		return iv.val == 0;
	}
	return 0;
}

void decl_complete_array(decl *d, int n)
{
	decl_desc *ar_desc = decl_desc_tail(d);
	expr *expr_sz;

	UCC_ASSERT(ar_desc->type == decl_desc_array, "invalid array completion");

	expr_sz = ar_desc->bits.array_size;
	expr_mutate_wrapper(expr_sz, val);
	expr_sz->val.iv.val = n;
}

int decl_inner_array_count(decl *d)
{
	decl_desc *ar_desc = decl_desc_tail(d);
	intval iv;

	UCC_ASSERT(ar_desc->type == decl_desc_array, "%s: not array", __func__);

	const_fold_need_val(ar_desc->bits.array_size, &iv);

	return iv.val;
}

int decl_ptr_or_block(decl *d)
{
	decl_desc *dp;
	for(dp = d->desc; dp; dp = dp->child)
		switch(dp->type){
			case decl_desc_ptr:
			case decl_desc_block:
			case decl_desc_array:
				return 1;
			case decl_desc_func:
				break;
		}
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
	/* else: XXX: memleak */

	return d;
}

void decl_conv_array_func_to_ptr(decl *d)
{
	decl_desc *dp = decl_desc_tail(d);

	/* f(int x[][5]) decays to f(int (*x)[5]), not f(int **x) */

	if(dp){
		switch(dp->type){
			case decl_desc_array:
				expr_free(dp->bits.array_size);
				dp->type = decl_desc_ptr;
				dp->bits.qual = qual_none;
				break;

			case decl_desc_func:
			{
				decl_desc *ins = decl_desc_ptr_new(dp->parent_decl, dp);

				ins->child = dp->child;
				dp->child = ins;
				break;
			}

			case decl_desc_ptr:
			case decl_desc_block:
				break;
		}
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

const char *decl_desc_to_str(enum decl_desc_type t)
{
	switch(t){
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
		fprintf(stderr, "\t%s\n", decl_desc_to_str(i->type));
}

void decl_desc_add_str(decl_desc *dp, int show_spel, char **bufp, int sz)
{
#define BUF_ADD(...) \
	do{ int n = snprintf(*bufp, sz, __VA_ARGS__); *bufp += n, sz -= n; }while(0)

	const int need_paren = dp->child ? dp->type != dp->child->type : 0;

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
		decl_desc_add_str(dp->child, show_spel, bufp, sz);
	else if(show_spel)
		BUF_ADD("%s", dp->parent_decl->spel);

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
		{
			intval iv;

			const_fold_need_val(dp->bits.array_size, &iv);

			BUF_ADD("[%ld]", iv.val);
			break;
		}
	}

	if(need_paren)
		BUF_ADD(")");
#undef BUF_ADD
}

const char *decl_to_str_r_spel(char buf[DECL_STATIC_BUFSIZ], int show_spel, decl *d)
{
	char *bufp = buf;

#define BUF_ADD(...) \
	bufp += snprintf(bufp, DECL_STATIC_BUFSIZ - (bufp - buf), __VA_ARGS__)

	BUF_ADD("%s%s", type_to_str(d->type), d->desc ? " " : "");

	if(d->desc)
		decl_desc_add_str(d->desc, show_spel, &bufp, DECL_STATIC_BUFSIZ - (bufp - buf));
	else if(show_spel && d->spel)
		BUF_ADD(" %s", d->spel);

	return buf;
#undef BUF_ADD
}

const char *decl_to_str_r(char buf[DECL_STATIC_BUFSIZ], decl *d)
{
	return decl_to_str_r_spel(buf, 0, d);
}

const char *decl_to_str(decl *d)
{
	static char buf[DECL_STATIC_BUFSIZ];
	return decl_to_str_r_spel(buf, 0, d);
}

int decl_init_len(decl_init *di)
{
 switch(di->type){
	 case decl_init_scalar:
		 return 1;

	 case decl_init_brace:
		 return dynarray_count((void **)di->bits.inits);
 }
 ICE("decl init bad type");
 return -1;
}

decl_init *decl_init_new(enum decl_init_type t)
{
	decl_init *di = umalloc(sizeof *di);
	where_new(&di->where);
	di->type = t;
	return di;
}

const char *decl_init_to_str(enum decl_init_type t)
{
	switch(t){
		CASE_STR_PREFIX(decl_init, scalar);
		CASE_STR_PREFIX(decl_init, brace);
	}
	return NULL;
}

/*
decl_init_sub *decl_init_sub_zero_for_decl(decl *d)
{
	decl_init_sub *s = umalloc(sizeof *s);
	memcpy(&s->where, &d->where, sizeof s->where);

	if(decl_is_struct_or_union(d)){
		sue_member **m;

		s->init = decl_init_new(decl_init_struct);

		for(m = d->type->sue->members; m && *m; m++){
			decl_init_sub *sub = decl_init_sub_zero_for_decl((*m)->struct_member);
			dynarray_add((void ***)&s->init->bits.subs, sub);
		}

	}else if(decl_is_array(d)){
		ICE("TODO: zero init array");
	}else{
		s->init = decl_init_new(decl_init_scalar);

		s->init->bits.expr = expr_new_val(0);
	}

	return s;
}
*/
