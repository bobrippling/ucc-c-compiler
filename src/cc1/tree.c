#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "alloc.h"
#include "tree.h"
#include "macros.h"
#include "sym.h"
#include "platform.h"

void where_new(struct where *w)
{
	extern int currentline, currentchar;
	extern const char *currentfname;

	w->line  = currentline;
	w->chr   = currentchar;
	w->fname = currentfname;
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
	e->vartype = type_new();
	return e;
}

expr *expr_new_val(int v)
{
	expr *e = expr_new();
	e->type = expr_val;
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

type *type_new()
{
	type *t = umalloc(sizeof *t);
	where_new(&t->where);
	return t;
}

type *type_copy(type *t)
{
	type *ret = umalloc(sizeof *ret);
	memcpy(ret, t, sizeof *ret);
	return ret;
}

function *function_new()
{
	function *f = umalloc(sizeof *f);
	where_new(&f->where);
	return f;
}

global *global_new(function *f, decl *d)
{
	global *g = umalloc(sizeof *g);
	where_new(&g->where);
	if(f)
		g->ptr.f = f;
	else
		g->ptr.d = d;
	g->isfunc = !!f;
	return g;
}

expr *expr_ptr_multiply(expr *e, type *t)
{
	expr *ret = expr_new();
	memcpy(&ret->where, &e->where, sizeof e->where);

	ret->type = expr_op;
	ret->op   = op_multiply;

	ret->lhs  = e;
	ret->rhs  = expr_new_val(type_size(t));

	return ret;
}

int type_size(type *t)
{
	if(t->ptr_depth)
		return platform_word_size();
	switch(t->primitive){
		case type_char:
			return 1;

		case type_unknown:
		case type_void:
		case type_int:
			return platform_word_size(); /* should be 4 */
	}
	return platform_word_size();
}

const char *expr_to_str(enum expr_type t)
{
	switch(t){
		CASE_STR_PREFIX(expr, op);
		CASE_STR_PREFIX(expr, val);
		CASE_STR_PREFIX(expr, addr);
		CASE_STR_PREFIX(expr, sizeof);
		CASE_STR_PREFIX(expr, str);
		CASE_STR_PREFIX(expr, identifier);
		CASE_STR_PREFIX(expr, assign);
		CASE_STR_PREFIX(expr, funcall);
		CASE_STR_PREFIX(expr, cast);
		CASE_STR_PREFIX(expr, if);
	}
	return NULL;
}

const char *op_to_str(enum op_type o)
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

const char *stat_to_str(enum stat_type t)
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
	}
	return NULL;
}

const char *type_to_str(type *t)
{
	switch(t->primitive){
		CASE_STR_PREFIX(type, int);
		CASE_STR_PREFIX(type, char);
		CASE_STR_PREFIX(type, void);
		CASE_STR_PREFIX(type, unknown);
	}
	return NULL;
}

const char *spec_to_str(enum type_spec s)
{
	switch(s){
		CASE_STR_PREFIX(spec, const);
		CASE_STR_PREFIX(spec, extern);
		CASE_STR_PREFIX(spec, static);
		CASE_STR_PREFIX(spec, none);
	}
	return NULL;
}

const char *assign_to_str(enum assign_type t)
{
	switch(t){
		CASE_STR_PREFIX(assign, normal);
		CASE_STR_PREFIX(assign, pre_increment);
		CASE_STR_PREFIX(assign, pre_decrement);
		CASE_STR_PREFIX(assign, post_increment);
		CASE_STR_PREFIX(assign, post_decrement);
	}
	return NULL;
}

const char *where_str(struct where *w)
{
	static char buf[128];
	snprintf(buf, sizeof buf, "%s:%d:%d", w->fname, w->line, w->chr + 1);
	return buf;
}
