#include <stdlib.h>

#include "alloc.h"
#include "tree.h"
#include "macros.h"

void where_new(struct where *w)
{
	extern int currentline, currentchar;
	extern const char *currentfname;

	w->line  = currentline;
	w->chr   = currentchar;
	w->fname = currentfname;
}

tree *tree_new()
{
	tree *t = umalloc(sizeof *t);
	where_new(&t->where);
	return t;
}

expr *expr_new()
{
	expr *e = umalloc(sizeof *e);
	where_new(&e->where);
	return e;
}

decl *decl_new()
{
	decl *d = umalloc(sizeof *d);
	where_new(&d->where);
	return d;
}

function *function_new()
{
	function *f = umalloc(sizeof *f);
	where_new(&f->where);
	return f;
}

const char *expr_to_str(enum expr_type t)
{
	switch(t){
		CASE_STR(expr_op);
		CASE_STR(expr_val);
		CASE_STR(expr_addr);
		CASE_STR(expr_sizeof);
		CASE_STR(expr_str);
		CASE_STR(expr_identifier);
		CASE_STR(expr_assign);
		CASE_STR(expr_funcall);
	}
	return NULL;
}

const char *op_to_str(enum op_type o)
{
	switch(o){
		CASE_STR(op_multiply);
		CASE_STR(op_divide);
		CASE_STR(op_plus);
		CASE_STR(op_minus);
		CASE_STR(op_modulus);
		CASE_STR(op_deref);
		CASE_STR(op_eq);
		CASE_STR(op_ne);
		CASE_STR(op_le);
		CASE_STR(op_lt);
		CASE_STR(op_ge);
		CASE_STR(op_gt);
		CASE_STR(op_or);
		CASE_STR(op_and);
		CASE_STR(op_orsc);
		CASE_STR(op_andsc);
		CASE_STR(op_not);
		CASE_STR(op_bnot);
		CASE_STR(op_unknown);
	}
	return NULL;
}

const char *stat_to_str(enum stat_type t)
{
	switch(t){
		CASE_STR(stat_do);
		CASE_STR(stat_if);
		CASE_STR(stat_else);
		CASE_STR(stat_while);
		CASE_STR(stat_for);
		CASE_STR(stat_break);
		CASE_STR(stat_return);
		CASE_STR(stat_expr);
		CASE_STR(stat_noop);
		CASE_STR(stat_code);
	}
	return NULL;
}
