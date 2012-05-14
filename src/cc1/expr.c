#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "data_structs.h"
#include "cc1.h"
#include "namespace.h"

/* needed for expr_assignment() */
#include "ops/expr_assign.h"

void expr_mutate(expr *e, func_mutate_expr *f,
		func_fold *f_fold,
		func_str *f_str,
		func_gen *f_gen,
		func_gen *f_gen_str,
		func_gen *f_gen_style
		)
{
	e->f_fold = f_fold;
	e->f_str  = f_str;

	switch(cc1_backend){
		case BACKEND_ASM:   e->f_gen = f_gen;       break;
		case BACKEND_PRINT: e->f_gen = f_gen_str;   break;
		case BACKEND_STYLE: e->f_gen = f_gen_style; break;
		default: ICE("bad backend");
	}

	e->f_const_fold = NULL;
	e->f_gen_1 = NULL;
	e->f_store = NULL;

	f(e);
}

expr *expr_new(func_mutate_expr *f,
		func_fold *f_fold,
		func_str *f_str,
		func_gen *f_gen,
		func_gen *f_gen_str,
		func_gen *f_gen_style)
{
	expr *e = umalloc(sizeof *e);
	where_new(&e->where);
	expr_mutate(e, f, f_fold, f_str, f_gen, f_gen_str, f_gen_style);
	return e;
}

expr *expr_new_intval(intval *iv)
{
	expr *e = expr_new_val(0);
	memcpy(&e->val.iv, iv, sizeof e->val.iv);
	return e;
}

expr *expr_ptr_multiply(expr *e, decl *d)
{
	decl *dtmp;
	expr *ret;
	int sz;

	dtmp = decl_copy(d);

	sz = decl_size(decl_ptr_depth_dec(dtmp, NULL));

	decl_free(dtmp);

	if(sz == 1)
		return e;

	ret = expr_new_op(op_multiply);
	memcpy(&ret->where, &e->where, sizeof e->where);

	ret->tree_type = decl_copy(e->tree_type);

	ret->lhs  = e;
	ret->rhs  = expr_new_val(sz);

	return ret;
}

expr *expr_new_decl_init(decl *d)
{
	UCC_ASSERT(d->init, "no init");
	return expr_new_assign(expr_new_identifier(namespace_none(d->spel)), d->init);
}

expr *expr_new_array_decl_init(decl *d, int ival, int idx)
{
	expr *deref;
	expr *sum;

	UCC_ASSERT(d->init, "no init");

	deref = expr_new_op(op_deref);

	sum = op_deref_expr(deref) = expr_new_op(op_plus);

	sum->lhs = expr_new_identifier(namespace_none(d->spel));
	sum->rhs = expr_new_val(idx); /* fold will multiply this */

	return expr_new_assign(deref, expr_new_val(ival));
}
