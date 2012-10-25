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
#include "funcargs.h"

#define ITER_DESC_TYPE(d, dp, typ)     \
	for(dp = d->desc; dp; dp = dp->child) \
		if(dp->type == typ)


decl_ref *decl_ref_new(enum decl_ref_type t)
{
	decl_ref *r = umalloc(sizeof *r);
	r->type = t;
	return r;
}

decl_ref *decl_ref_new_type(type *t)
{
	decl_ref *r = decl_ref_new(decl_ref_type);
	r->bits.type = t;
	return r;
}

decl_ref *decl_ref_new_tdef(expr *e)
{
	decl_ref *r = decl_ref_new(decl_ref_tdef);
	r->bits.type_of = e;
	return r;
}

decl_ref *decl_ref_new_ptr(decl_ref *to, enum type_qualifier q)
{
	decl_ref *r = decl_ref_new(decl_ref_ptr);
	r->ref = to;
	r->bits.qual = q;
	return r;
}

decl_ref *decl_ref_new_block(decl_ref *to, enum type_qualifier q)
{
	decl_ref *r = decl_ref_new_ptr(to, q);
	r->type = decl_ref_block;
	return r;
}

decl *decl_new()
{
	decl *d = umalloc(sizeof *d);
	where_new(&d->where);
	return d;
}

decl *decl_new_type(enum type_primitive p)
{
	decl *d = decl_new();
	type *t = d->ref->bits.type;

	t->primitive = p;

	switch(p){
		case type_ptrdiff_t:
		case type_intptr_t:
			t->is_signed = 0;
		default:
			break;
	}

	return d;
}

void decl_ref_free(decl_ref *r)
{
	if(!r)
		return;

	decl_ref_free(r->ref);

	switch(r->type){
		case decl_ref_type:
			type_free(r->bits.type);
			break;

		case decl_ref_block:
		case decl_ref_func:
			funcargs_free(r->bits.func, 1);
			break;

		case decl_ref_array:
			expr_free(r->bits.array_size);
			break;

		case decl_ref_ptr:
		case decl_ref_tdef:
			break;
	}

	decl_attr_free(r->attr);

	free(r);
}

void decl_free(decl *d)
{
	if(!d)
		return;

	decl_ref_free(d->ref);
	expr_free(d->field_width);

	free(d);
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

void decl_attr_free(decl_attr *a)
{
	if(!a)
		return;

	decl_attr_free(a->next);
	free(a);
}

#include "decl_is.c"

int decl_ref_size(decl_ref *r)
{
	switch(r->type){
		case decl_ref_type:
			return type_size(r->bits.type);

		case decl_ref_tdef:
			return decl_ref_size(r->ref);

		case decl_ref_ptr:
		case decl_ref_block:
			return type_primitive_size(type_intptr_t);

		case decl_ref_func:
			/* don't return r->ref's size */
			return 1;

		case decl_ref_array:
		{
			intval sz;

			const_fold_need_val(r->bits.array_size, &sz);

			if(sz.val == 0)
				DIE_AT(&r->where, "incomplete array size attempt");

			return sz.val * decl_ref_size(r->ref);
		}
	}

	ucc_unreach();
}

int decl_size(decl *d)
{
	if(d->field_width){
		intval iv;

		ICW("use of struct field width - brace for incorrect code (%s)",
				where_str(&d->where));

		const_fold_need_val(d->field_width, &iv);

		return iv.val;
	}

	return decl_ref_size(d->ref);
}

enum funcargs_cmp funcargs_equal(
		funcargs *args_to, funcargs *args_from,
		int strict_types, const char *fspel)
{
	const int count_to = dynarray_count((void **)args_to->arglist);
	const int count_from = dynarray_count((void **)args_from->arglist);

	if((count_to   == 0 && !args_to->args_void)
	|| (count_from == 0 && !args_from->args_void)){
		/* a() or b() */
		return funcargs_are_equal;
	}

	if(!(args_to->variadic ? count_to <= count_from : count_to == count_from))
		return funcargs_are_mismatch_count;

	if(count_to){
		const enum decl_cmp flag = DECL_CMP_ALLOW_VOID_PTR | (strict_types ? DECL_CMP_EXACT_MATCH : 0);
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

				return funcargs_are_mismatch_types;
			}
	}

	return funcargs_are_equal;
}

int decl_ref_equal(decl_ref *a, decl_ref *b, enum decl_cmp mode)
{
	if(!a || !b)
		return a == b ? 1 : 0;


	/* array/func decay takes care of most of this */
	if(a->type != b->type)
		return 0;

	switch(a->type){
		case decl_ref_type:
			return type_equal(a->bits.type, b->bits.type, mode & DECL_CMP_EXACT_MATCH ? TYPE_CMP_EXACT : 0);

		case decl_ref_array:
		{
			intval av, bv;

			const_fold_need_val(a->bits.array_size, &av);
			const_fold_need_val(b->bits.array_size, &bv);

			if(av.val != bv.val)
				return 0;

			goto ref_eq;
		}

		case decl_ref_ptr:
		case decl_ref_block:
			if(a->bits.qual != b->bits.qual)
				return 0;
			/* fall */
ref_eq:
		case decl_ref_tdef:
			return decl_ref_equal(a->ref, b->ref, mode);

		case decl_ref_func:
			if(funcargs_are_equal != funcargs_equal(a->bits.func, b->bits.func, 1 /* exact match */, NULL))
				return 0;
			break;
	}

	return decl_ref_equal(a->ref, b->ref, mode);
}

int decl_equal(decl *a, decl *b, enum decl_cmp mode)
{
	const int a_ptr = decl_is_ptr(a);
	const int b_ptr = decl_is_ptr(b);

	if(mode & DECL_CMP_ALLOW_VOID_PTR){
		/* one side is void * */
		if(decl_is_void_ptr(a) && b_ptr)
			return 1;
		if(decl_is_void_ptr(b) && a_ptr)
			return 1;
	}

	/* we are exact if told, or if either are a pointer - types must be equal */
	if(a_ptr || b_ptr)
		mode |= DECL_CMP_EXACT_MATCH;

	return decl_ref_equal(a->ref, b->ref, mode);
}

int decl_ptr_depth(decl *d)
{
	int depth = 0;
	decl_ref *r;

	for(r = d->ref; r; r = r->ref)
		if(r->type == decl_ref_ptr)
			depth++;

	return depth;
}

decl *decl_ptr_depth_inc(decl *d)
{
	EOF_WHERE(&d->where,
		d->ref = decl_ref_new_ptr(d->ref, qual_none)
	);

	return d;
}

decl_ref *decl_orphan(decl *d)
{
	decl_ref *orphan = d->ref;
	d->ref = d->ref->ref;
	orphan->ref = NULL;
	return orphan;
}

decl *decl_ptr_depth_dec(decl *d, where *from)
{
	decl_ref *orphan;

	/* *(void (*)()) does nothing */
	if(decl_is_fptr(d))
		goto fin;

	if(d->ref->type != decl_ref_ptr)
		DIE_AT(from, "invalid indirection applied to %s", decl_to_str(d));

	orphan = decl_orphan(d);

	if(!decl_complete(d))
		DIE_AT(from, "dereference pointer to incomplete type %s", decl_to_str(d));

	decl_ref_free(orphan);

fin:
	return d;
}

decl *decl_func_called(decl *d, funcargs **pfuncargs)
{
	decl_ref *orphan = decl_orphan(d);

	switch(orphan->type){
		case decl_ref_ptr:
		case decl_ref_block:
		{
			decl_ref *func = decl_orphan(d);
			funcargs *args;

			UCC_ASSERT(func->type == decl_ref_func, "func call not a func");

			args = func->bits.func;
			func->bits.func = NULL;

			decl_ref_free(func);
			decl_ref_free(orphan);

			if(pfuncargs)
				*pfuncargs = args;

			return d;
		}

		case decl_ref_func: /* can't call this - decays to type(*)() */
		default:
			ICE("can't func-deref non func-ptr/block ref");
	}

	ucc_unreach();
}

void decl_conv_array_func_to_ptr(decl *d)
{
	decl_ref *r = d->ref;

	/* f(int x[][5]) decays to f(int (*x)[5]), not f(int **x) */

	switch(r->type){
		case decl_ref_array:
		{
			decl_ref *orphan = decl_orphan(d);
			/* orphan == r */
			decl_ref_free(orphan);

			/* fall */
		}

		case decl_ref_func:
			d->ref = decl_ref_new_ptr(r, qual_none);
			break;

		default:break;
	}
}

static void decl_ref_add_str(decl_ref *r, char *spel, char **bufp, int sz)
{
#define BUF_ADD(...) \
	do{ int n = snprintf(*bufp, sz, __VA_ARGS__); *bufp += n, sz -= n; }while(0)

	int need_paren;

	switch(r->type){
		case decl_ref_type:
		case decl_ref_tdef:
			return;
		default:break;
	}

	need_paren = r->ref == decl_ref_type ? 0 : r->type != r->ref->type;

	if(need_paren)
		BUF_ADD("(");

	switch(r->type){
		case decl_ref_ptr:
		case decl_ref_block:
			BUF_ADD("%c%s",
					r->type == decl_ref_ptr ? '*' : '^',
					type_qual_to_str(r->bits.qual));
			break;
		default:
			break;
	}

	if(r->ref)
		decl_ref_add_str(r->ref, spel, bufp, sz);
	else if(spel)
		BUF_ADD("%s", spel);

	if(need_paren)
		BUF_ADD(")");

	switch(r->type){
		case decl_ref_tdef:
			/* TODO: "aka: %s" */
		case decl_ref_type:
		case decl_ref_block:
		case decl_ref_ptr:
			break;
		case decl_ref_func:
		{
			const char *comma = "";
			decl **i;
			funcargs *args = r->bits.func;

			BUF_ADD("(");
			for(i = args->arglist; i && *i; i++){
				char tmp_buf[DECL_STATIC_BUFSIZ];
				BUF_ADD("%s%s", comma, decl_to_str_r(tmp_buf, *i));
				comma = ", ";
			}
			BUF_ADD("%s)", args->variadic ? ", ..." : args->args_void ? "void" : "");
			break;
		}
		case decl_ref_array:
		{
			intval iv;

			const_fold_need_val(r->bits.array_size, &iv);

			if(iv.val == 0)
				BUF_ADD("[]");
			else
				BUF_ADD("[%ld]", iv.val);
			break;
		}
	}
#undef BUF_ADD
}

static void decl_ref_add_type_str(decl_ref *r, char *spel, char **bufp, int sz)
{
	/* go down to the first type or typedef, print it and then its descriptions */
	{
		int len;
		decl_ref *rt;

		for(rt = r; rt->type != decl_ref_type && rt->type != decl_ref_tdef; rt = rt->ref);
		strcpy(*bufp, type_to_str(rt->bits.type));

		len = strlen(*bufp);

		bufp += len;
		*(*bufp)++ = ' ';

		sz -= len + 1;
	}

	decl_ref_add_str(r, spel, bufp, sz);
}

const char *decl_to_str_r_spel(char buf[DECL_STATIC_BUFSIZ], int show_spel, decl *d)
{
	char *bufp = buf;

	decl_ref_add_type_str(d->ref, show_spel ? d->spel : NULL,
			&bufp, DECL_STATIC_BUFSIZ - (bufp - buf));

	return buf;
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
