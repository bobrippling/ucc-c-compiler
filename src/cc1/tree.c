#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "../util/alloc.h"
#include "../util/util.h"
#include "data_structs.h"
#include "macros.h"
#include "sym.h"
#include "../util/platform.h"
#include "struct.h"
#include "enum.h"

where *eof_where = NULL;

void where_new(struct where *w)
{
	extern int current_line, current_chr;
	extern const char *current_fname;
	extern int buffereof;

	if(buffereof){
		if(eof_where)
			memcpy(w, eof_where, sizeof *w);
		else
			memset(w, 0, sizeof *w); /*ICE("where_new() after buffer eof");*/
	}else{
		extern int current_fname_used;

		w->line  = current_line;
		w->chr   = current_chr;
		w->fname = current_fname;

		UCC_ASSERT(current_fname, "no current fname");

		current_fname_used = 1;
	}
}

decl_ptr *decl_ptr_new()
{
	decl_ptr *dp = umalloc(sizeof *dp);
	where_new(&dp->where);
	return dp;
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

type *type_new()
{
	type *t = umalloc(sizeof *t);
	where_new(&t->where);
	t->is_signed = 1;
	t->primitive = type_unknown;
	return t;
}

type *type_copy(type *t)
{
	type *ret = umalloc(sizeof *ret);
	memcpy(ret, t, sizeof *ret);
	return ret;
}

decl_ptr *decl_ptr_copy(decl_ptr *dp)
{
	decl_ptr *ret = umalloc(sizeof *ret);
	memcpy(ret, dp, sizeof *ret);
	if(dp->child)
		ret->child = decl_ptr_copy(dp->child);
	/* leave func and spel, tis fine */
	return ret;
}

decl *decl_copy(decl *d)
{
	decl *ret = umalloc(sizeof *ret);
	memcpy(ret, d, sizeof *ret);
	ret->type = type_copy(d->type);
	if(d->decl_ptr)
		ret->decl_ptr = decl_ptr_copy(d->decl_ptr);
	/*ret->spel = NULL;*/
	return ret;
}

funcargs *funcargs_new()
{
	funcargs *f = umalloc(sizeof *f);
	where_new(&f->where);
	return f;
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

void funcargs_free(funcargs *args, int free_decls)
{
	if(free_decls){
		int i;
		for(i = 0; args->arglist[i]; i++)
			decl_free(args->arglist[i]);
	}
	free(args);
}

int type_size(const type *t)
{
	if(t->typeof)
		return decl_size(t->typeof->decl);

	switch(t->primitive){
		case type_char:
		case type_void:
			return 1;

		case type_short:
			return 2;

		case type_enum:
		case type_int:
		case type_float:
			return 4;

		case type_long:
		case type_double:
			return 8; /* FIXME: 4 on 32-bit */

		case type_union:
		case type_struct:
			return struct_union_size(t->struct_union);

		case type_unknown:
			break;
	}

	ICE("type %s in type_size()", type_to_str(t));
	return -1;
}

int decl_size(decl *d)
{
	if(decl_has_array(d)){
		const int siz = type_size(d->type);
		decl_ptr *dp;
		int ret = 0;

		for(dp = d->decl_ptr; dp; dp = dp->child)
			if(dp->array_size){
				/* should've been folded fully */
				long v = dp->array_size->val.iv.val;
				if(!v)
					v = platform_word_size(); /* int x[0] - the 0 is a sentinel */
				ret += v * siz;
			}

		return ret;
	}

	if(d->decl_ptr) /* pointer */
		return platform_word_size();

	if(d->field_width)
		return d->field_width;

	return type_size(d->type);
}

int type_equal(const type *a, const type *b, int strict)
{
	/*
	 * basic const checking, doesn't work with
	 * const char *const x, etc..
	 */
	if(strict && (b->qual & qual_const) && (a->qual & qual_const) == 0)
		return 0; /* we can assign from const to non-const, but not vice versa - FIXME should be elsewhere? */

	if(a->struct_union != b->struct_union || a->enu != b->enu)
		return 0;

	return strict ? a->primitive == b->primitive : 1;
}

int decl_ptr_equal(decl_ptr *dpa, decl_ptr *dpb)
{
	/* if we are assigning from const, target must be const */
	if(dpb->is_const ? dpa->is_const : 0)
		return 0;

	if(dpa->child)
		return dpb->child && decl_ptr_equal(dpa->child, dpb->child);

	return !dpb->child;
}

int decl_equal(decl *a, decl *b, enum decl_cmp mode)
{
#define VOID_PTR(d) (                   \
			d->type->primitive == type_void   \
			&&  d->decl_ptr                   \
			&& !d->decl_ptr->child            \
		)

	if((mode & DECL_CMP_ALLOW_VOID_PTR) && (VOID_PTR(a) || VOID_PTR(b)))
		return 1; /* one side is void * */

	if(!type_equal(a->type, b->type, mode & DECL_CMP_STRICT_PRIMITIVE))
		return 0;

	if(a->decl_ptr){
		if(b->decl_ptr)
			return decl_ptr_equal(a->decl_ptr, b->decl_ptr);
	}else if(!b->decl_ptr){
		return 1;
	}

	return 0;
}

void function_empty_args(funcargs *func)
{
	if(func->arglist){
		UCC_ASSERT(!func->arglist[1], "empty_args called when it shouldn't be");

		decl_free(func->arglist[0]);
		free(func->arglist);
		func->arglist = NULL;
	}
	func->args_void = 0;
}

const char *op_to_str(const enum op_type o)
{
	switch(o){
		CASE_STR_PREFIX(op, multiply);
		CASE_STR_PREFIX(op, divide);
		CASE_STR_PREFIX(op, plus);
		CASE_STR_PREFIX(op, minus);
		CASE_STR_PREFIX(op, modulus);
		CASE_STR_PREFIX(op, deref);
		CASE_STR_PREFIX(op, eq);
		CASE_STR_PREFIX(op, ne);
		CASE_STR_PREFIX(op, le);
		CASE_STR_PREFIX(op, lt);
		CASE_STR_PREFIX(op, ge);
		CASE_STR_PREFIX(op, gt);
		CASE_STR_PREFIX(op, or);
		CASE_STR_PREFIX(op, xor);
		CASE_STR_PREFIX(op, and);
		CASE_STR_PREFIX(op, orsc);
		CASE_STR_PREFIX(op, andsc);
		CASE_STR_PREFIX(op, not);
		CASE_STR_PREFIX(op, bnot);
		CASE_STR_PREFIX(op, shiftl);
		CASE_STR_PREFIX(op, shiftr);
		CASE_STR_PREFIX(op, struct_ptr);
		CASE_STR_PREFIX(op, struct_dot);
		CASE_STR_PREFIX(op, unknown);
	}
	return NULL;
}

const char *type_primitive_to_str(const enum type_primitive p)
{
	switch(p){
		CASE_STR_PREFIX(type, void);
		CASE_STR_PREFIX(type, char);
		CASE_STR_PREFIX(type, short);
		CASE_STR_PREFIX(type, int);
		CASE_STR_PREFIX(type, long);
		CASE_STR_PREFIX(type, float);
		CASE_STR_PREFIX(type, double);

		CASE_STR_PREFIX(type, struct);
		CASE_STR_PREFIX(type, union);
		CASE_STR_PREFIX(type, enum);

		CASE_STR_PREFIX(type, unknown);
	}
	return NULL;
}

const char *type_store_to_str(const enum type_storage s)
{
	switch(s){
		CASE_STR_PREFIX(store, auto);
		CASE_STR_PREFIX(store, static);
		CASE_STR_PREFIX(store, extern);
		CASE_STR_PREFIX(store, register);
		CASE_STR_PREFIX(store, typedef);
	}
	return NULL;
}

int op_is_cmp(enum op_type o)
{
	switch(o){
		case op_eq:
		case op_ne:
		case op_le:
		case op_lt:
		case op_ge:
		case op_gt:
			return 1;
		default:
			break;
	}
	return 0;
}

int decl_ptr_depth(decl *d)
{
	decl_ptr *dp;
	int i = 0;

	for(dp = d->decl_ptr; dp; dp = dp->child)
		i++;

	return i;
}

decl_ptr **decl_leaf(decl *d)
{
	decl_ptr **dp;
	UCC_ASSERT(d, "null decl param");
	for(dp = &d->decl_ptr; *dp; dp = &(*dp)->child);
	return dp;
}

funcargs *decl_funcargs(decl *d)
{
	/* either ->func on the decl, or on a decl_ptr (in the case of a funcptr) */
	decl_ptr *dp;

	if(d->funcargs)
		return d->funcargs;

	for(dp = d->decl_ptr; dp; dp = dp->child)
		if(dp->fptrargs)
			return dp->fptrargs;
	return NULL;
}

int decl_is_callable(decl *d)
{
	return !!decl_funcargs(d);
}

int decl_is_struct_or_union(decl *d)
{
	return d->type->primitive == type_struct || d->type->primitive == type_union;
}

int decl_has_array(decl *d)
{
	decl_ptr *dp;

	for(dp = d->decl_ptr; dp; dp = dp->child)
		if(dp->array_size)
			return 1;
	return 0;
}

int decl_is_const(decl *d)
{
	decl_ptr *dp = *decl_leaf(d);
	if(dp)
		return dp->is_const;
	return 0;/*d->type->spec & spec_const; TODO */
}

int decl_is_func_ptr(decl *d)
{
	return !!decl_funcargs(d);
}

decl *decl_ptr_depth_inc(decl *d)
{
	*decl_leaf(d) = decl_ptr_new();
	return d;
}

decl *decl_ptr_depth_dec(decl *d)
{
	d->decl_ptr = d->decl_ptr->child;
	return d;
}

decl *decl_func_deref(decl *d)
{
	static int warned = 0;
	if(!warned && decl_ptr_depth(d)){
		extern decl *curdecl_func;
		ICW("funcall type propagation (for funcs returning pointers) is broken\n(in %s, calling %s())",
				curdecl_func->spel, d->spel);
		warned = 1;
	}
	/*d->funcargs = NULL;*/
	return d;
}

const char *type_to_str(const type *t)
{
#define BUF_SIZE (sizeof(buf) - (bufp - buf))
	static char buf[TYPE_STATIC_BUFSIZ];
	char *bufp = buf;

	if(t->typeof)     bufp += snprintf(bufp, BUF_SIZE, "typedef ");
	if(t->qual)       bufp += snprintf(bufp, BUF_SIZE, "%s%s",
		                          t->qual & qual_const    ? "const "    : "",
		                          t->qual & qual_volatile ? "volatile " : "");

	if(t->store)      bufp += snprintf(bufp, BUF_SIZE, "%s ", type_store_to_str(t->store));
	if(!t->is_signed) bufp += snprintf(bufp, BUF_SIZE, "unsigned ");

	if(t->struct_union){
		snprintf(bufp, BUF_SIZE, "%s %s",
				t->struct_union->is_union ? "union" : "struct",
				t->struct_union->spel);

	}else if(t->enu){
		snprintf(bufp, BUF_SIZE, "enum %s", t->enu->spel);
	}else{
		switch(t->primitive){
#define APPEND(t) case type_ ## t: snprintf(bufp, BUF_SIZE, "%s", #t); break
			APPEND(void);
			APPEND(char);
			APPEND(short);
			APPEND(int);
			APPEND(long);
			APPEND(float);
			APPEND(double);

			case type_unknown:
				ICE("unknown type primitive (%s)", where_str(&t->where));
			case type_enum:
				ICE("enum without ->enu");
			case type_struct:
			case type_union:
				snprintf(bufp, BUF_SIZE, "incomplete-%s %s",
						t->primitive == type_struct ? "struct" : "union",
						t->spel);
				break;
#undef APPEND
		}
	}

	return buf;
}

const char *decl_to_str(decl *d)
{
	static char buf[DECL_STATIC_BUFSIZ];
	unsigned int i = 0;
	decl_ptr *dp;

#define BUF_ADD(...) i += snprintf(buf + i, sizeof buf - i, __VA_ARGS__)

	BUF_ADD("%s%s", type_to_str(d->type), d->decl_ptr ? " " : "");

	for(dp = d->decl_ptr; dp; dp = dp->child)
		BUF_ADD("%s*%s%s%s%s",
				dp->fptrargs   ? "("  : "",
				dp->is_const   ? "K"  : "",
				dp->fptrargs   ? "()" : "",
				dp->array_size ? "[]" : "",
				dp->fptrargs   ? ")"  : "");

	buf[i] = '\0';

	if(d->funcargs)
		BUF_ADD("()");

	return buf;
}

intval *intval_new(long v)
{
	intval *r = umalloc(sizeof *r);
	r->val = v;
	return r;
}
