#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "data_structs.h"

/* needed for expr_assignment() */
#include "ops/expr_assign.h"

void expr_mutate(expr *e, func_fold *f_fold, func_str *f_str, func_gen *f_gen, func_gen *f_gen_str)
{
	extern int backend_str;

	e->f_fold = f_fold;
	e->f_str  = f_str;
	e->f_gen  = backend_str ? f_gen_str : f_gen;
}

expr *expr_new(func_fold *f_fold, func_str *f_str, func_gen *f_gen, func_gen *f_gen_str)
{
	expr *e = umalloc(sizeof *e);
	where_new(&e->where);
	expr_mutate(e, f_fold, f_str, f_gen, f_gen_str);
	return e;
}

expr *expr_new_intval(intval *iv)
{
	expr *e = expr_new_val(0);
	memcpy(&e->val.iv, iv, sizeof e->val.iv);
	return e;
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

expr *expr_ptr_multiply(expr *e, decl *d)
{
	decl *dtmp;
	expr *ret;
	int sz;

	dtmp = decl_copy(d);

	sz = decl_size(decl_ptr_depth_dec(dtmp));

	decl_free(dtmp);

	if(sz == 1)
		return e;

	ret = expr_new_op(op_multiply);
	memcpy(&ret->where, &e->where, sizeof e->where);

	if(ret->tree_type)
		decl_free(ret->tree_type);
	ret->tree_type = decl_copy(e->tree_type);

	ret->lhs  = e;
	ret->rhs  = expr_new_val(sz);

	return ret;
}

expr *expr_assignment(expr *to, expr *from)
{
	expr *ass = expr_new_assign();

	ass->lhs = to;
	ass->rhs = from;

	return ass;
}

expr *expr_new_decl_init(decl *d)
{
	UCC_ASSERT(d->init, "no init in %s", __func__);
	return expr_assignment(expr_new_identifier(d->spel), d->init);
}
