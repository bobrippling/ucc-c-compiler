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

expr *expr_new_val(int v)
{
	expr *e = expr_new();
	e->type = expr_val;
	e->tree_type->type->spec |= spec_const;
	e->val.i = v;
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

	memcpy(&tmp, d, sizeof tmp);
	tmp.ptr_depth--;
	sz = decl_size(&tmp);

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

int decl_size(const decl *d)
{
	if(d->ptr_depth){
		if(d->type->primitive == type_void)
			return 1;
		return platform_word_size();
	}

	switch(d->type->primitive){
		case type_char:
			return 1;

		case type_unknown:
		case type_void:
		case type_int:
			return platform_word_size(); /* should be 4 */
	}
	return platform_word_size();
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

	/* if the pointers are equal depth and we're not strict, and they are actually pointers... */
	if(!strict && a->ptr_depth && ptreq)
		return 1;

	return ptreq && type_equal(a->type, b->type, strict);
}

void function_empty_args(decl *d)
{
	if(d->func->args){
		decl_free(d->func->args[0]);
		free(d->func->args);
		d->func->args = NULL;
	}
	d->func->args_void = 0;
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
		case spec_none: return "";
	}
	return NULL;
}

const char *type_to_str(const type *t)
{
#define buf_size (sizeof(buf) - (bufp - buf))
	static char buf[TYPE_STATIC_BUFSIZ];
	int i;
	char *bufp = buf;

	for(i = 0; i < SPEC_MAX; i++)
		if(t->spec & (1 << i))
			bufp += snprintf(bufp, buf_size, "%s ", spec_to_str(1 << i));

#define APPEND(t) case type_ ## t: snprintf(bufp, buf_size, "%s", #t); break
	switch(t->primitive){
		APPEND(int);
		APPEND(char);
		APPEND(void);
		APPEND(unknown);
	}
#undef APPEND

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
