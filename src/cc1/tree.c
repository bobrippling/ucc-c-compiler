#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "../util/alloc.h"
#include "../util/util.h"
#include "tree.h"
#include "macros.h"
#include "sym.h"
#include "../util/platform.h"
#include "struct.h"

void where_new(struct where *w)
{
	extern int current_line, current_chr;
	extern const char *current_fname;

	w->line  = current_line;
	w->chr   = current_chr;
	w->fname = current_fname;
}

tree_flow *tree_flow_new()
{
	tree_flow *t = umalloc(sizeof *t);
	return t;
}

tree *tree_new()
{
	tree *t = umalloc(sizeof *t);
	where_new(&t->where);
	return t;
}

tree *tree_new_code()
{
	tree *t = tree_new();
	t->type = stat_code;
	t->symtab = symtab_new();
	return t;
}

expr *expr_new()
{
	expr *e = umalloc(sizeof *e);
	where_new(&e->where);
	e->tree_type = decl_new();
	return e;
}

expr *expr_new_intval(intval *iv)
{
	expr *e = expr_new();
	e->type = expr_val;
	e->tree_type->type->spec |= spec_const;
	memcpy(&e->val.i, iv, sizeof e->val.i);
	return e;
}

expr *expr_new_val(int i)
{
	expr *e = expr_new();
	e->type = expr_val;
	e->tree_type->type->spec |= spec_const;
	e->val.i.val = i;
	return e;
}

decl *decl_new()
{
	decl *d = umalloc(sizeof *d);
	where_new(&d->where);
	d->type = type_new();
	return d;
}

decl *decl_new_where(where *w)
{
	decl *d = decl_new();
	memcpy(&d->where,       w, sizeof w);
	memcpy(&d->type->where, w, sizeof w);
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
	t->spec = spec_none;
	t->primitive = type_unknown;
	return t;
}

type *type_copy(type *t)
{
	type *ret = umalloc(sizeof *ret);
	memcpy(ret, t, sizeof *ret);
	return ret;
}

decl *decl_copy(decl *d)
{
	decl *ret = umalloc(sizeof *ret);
	memcpy(ret, d, sizeof *ret);
	ret->type = type_copy(d->type);
	/*ret->spel = NULL;*/
	return ret;
}

#if 0
expr *expr_copy(expr *e)
{
	expr *ret;

	if(!e)
		return NULL;

	ret = umalloc(sizeof *ret);

	ret->lhs  = expr_copy(e->lhs);
	ret->rhs  = expr_copy(e->rhs);
	ret->spel = ustrdup(  e->spel);
	ret->expr = expr_copy(e->expr);
	ret->funcargs = NULL;
	ret->tree_type = decl_copy(e->tree_type);

	return ret;
}
#endif

function *function_new()
{
	function *f = umalloc(sizeof *f);
	where_new(&f->where);
	return f;
}

expr *expr_ptr_multiply(expr *e, decl *d)
{
	decl tmp;
	int sz;
	expr *ret;

	/*
	 * we need to adjust the pointer-ness
	 * normally this is fine, but if we do
	 * this for a typedef'd variable, then
	 * we could get:
	 * d->ptr_depth = 0
	 * d->primitive = abc
	 * d->tdef = { .ptr_depth = 1, .primitive = abc }
	 *
	 * which means they are out of sync
	 * thus the decl-clone
	 */

	memcpy(&tmp, d, sizeof tmp);
	tmp.ptr_depth--;
	if(tmp.type->tdef){
		if(tmp.type->tdef->type->tdef)
			ICE("FIXME: typedef'd typedef");

		tmp.type->tdef = decl_copy(tmp.type->tdef);
		tmp.type->tdef->ptr_depth--;
	}

	sz = decl_size(&tmp);

	if(tmp.type->tdef)
		decl_free(tmp.type->tdef);

	if(sz == 1)
		return e;

	ret = expr_new();
	memcpy(&ret->where, &e->where, sizeof e->where);

	ret->type = expr_op;
	ret->op   = op_multiply;

	ret->lhs  = e;
	ret->rhs  = expr_new_val(sz);

	return ret;
}

expr *expr_assignment(expr *to, expr *from)
{
	expr *ass = expr_new();

	ass->type = expr_assign;

	ass->lhs = to;
	ass->rhs = from;

	return ass;
}

int type_size(const type *t)
{
	switch(t->primitive){
		case type_char:
		case type_void:
			return 1;

		case type_int:
			/* FIXME: 4 for int */
			return platform_word_size();

		case type_typedef:
			return decl_size(t->tdef);

		case type_struct:
			return struct_size(t->struc);

		default:
		case type_unknown:
			ICE("type %s in decl_size()", type_to_str(t));
			return -1;
	}
}

int decl_size(const decl *d)
{
	if(d->arraysizes){
		/* should've been folded fully */
		const int siz = type_size(d->type);
		int i;
		int ret = 0;

		for(i = 0; d->arraysizes[i]; i++)
			ret += d->arraysizes[i]->val.i.val * siz;

		return ret;
	}

	if(d->ptr_depth)
		return platform_word_size();

	return type_size(d->type);
}

int type_equal(const type *a, const type *b, int strict)
{
	/*
	 * basic const checking, doesn't work with
	 * const char *const x, etc..
	 */
	if(strict && b->spec & spec_const && (a->spec & spec_const) == 0)
		return 0; /* we can assign from const to non-const, but not vice versa */

	return strict ? a->primitive == b->primitive : 1; /* int == char */
}

int decl_equal(const decl *a, const decl *b, int strict)
{
	const int ptreq = a->ptr_depth == b->ptr_depth;

#define VOID_PTR(d) \
		 (d->type->primitive == type_void && d->ptr_depth == 1)

	if(VOID_PTR(a) || VOID_PTR(b))
		return 1; /* one side is void * */

	return ptreq && type_equal(a->type, b->type, strict);
}

void function_empty_args(function *func)
{
	if(func->args){
		decl_free(func->args[0]);
		free(func->args);
		func->args = NULL;
	}
	func->args_void = 0;
}

const char *expr_to_str(const enum expr_type t)
{
	switch(t){
		CASE_STR_PREFIX(expr, op);
		CASE_STR_PREFIX(expr, val);
		CASE_STR_PREFIX(expr, addr);
		CASE_STR_PREFIX(expr, sizeof);
		CASE_STR_PREFIX(expr, identifier);
		CASE_STR_PREFIX(expr, assign);
		CASE_STR_PREFIX(expr, funcall);
		CASE_STR_PREFIX(expr, cast);
		CASE_STR_PREFIX(expr, if);
		CASE_STR_PREFIX(expr, comma);
	}
	return NULL;
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

const char *stat_to_str(const enum stat_type t)
{
	switch(t){
		CASE_STR_PREFIX(stat, do);
		CASE_STR_PREFIX(stat, if);
		CASE_STR_PREFIX(stat, while);
		CASE_STR_PREFIX(stat, for);
		CASE_STR_PREFIX(stat, break);
		CASE_STR_PREFIX(stat, return);
		CASE_STR_PREFIX(stat, expr);
		CASE_STR_PREFIX(stat, noop);
		CASE_STR_PREFIX(stat, code);
		CASE_STR_PREFIX(stat, goto);
		CASE_STR_PREFIX(stat, label);
		CASE_STR_PREFIX(stat, switch);
		CASE_STR_PREFIX(stat, case);
		CASE_STR_PREFIX(stat, case_range);
		CASE_STR_PREFIX(stat, default);
	}
	return NULL;
}

const char *spec_to_str(const enum type_spec s)
{
	switch(s){
		CASE_STR_PREFIX(spec, const);
		CASE_STR_PREFIX(spec, extern);
		CASE_STR_PREFIX(spec, static);
		CASE_STR_PREFIX(spec, signed);
		CASE_STR_PREFIX(spec, unsigned);
		CASE_STR_PREFIX(spec, auto);
		CASE_STR_PREFIX(spec, typedef);
		case spec_none: return "";
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

const char *type_to_str(const type *t)
{
#define BUF_SIZE (sizeof(buf) - (bufp - buf))
	static char buf[TYPE_STATIC_BUFSIZ];
	int i;
	char *bufp = buf;

	if(t->tdef)
		return type_to_str(t->tdef->type);

	for(i = 0; i < SPEC_MAX; i++)
		if(t->spec & (1 << i))
			bufp += snprintf(bufp, BUF_SIZE, "%s ", spec_to_str(1 << i));

	if(t->struc)
		snprintf(bufp, BUF_SIZE, "struct %s", t->struc->spel);
	else
		switch(t->primitive){
#define APPEND(t) case type_ ## t: snprintf(bufp, BUF_SIZE, "%s", #t); break
			APPEND(int);
			APPEND(char);
			APPEND(void);
			case type_unknown:
				ICE("unknown type primitive");
			case type_typedef:
				ICE("typedef without ->tdef");
			case type_struct:
				ICE("struct without ->struc");
#undef APPEND
		}

	return buf;
}

const char *decl_to_str(const decl *d)
{
	static char buf[DECL_STATIC_BUFSIZ];
	unsigned int i;
	int n;

	i = snprintf(buf, sizeof buf, "%s%s", type_to_str(d->type), d->ptr_depth ? " " : "");

	for(n = d->ptr_depth; i + 1 < sizeof buf && n > 0; n--)
		buf[i++] = '*';
	buf[i] = '\0';

	return buf;
}
