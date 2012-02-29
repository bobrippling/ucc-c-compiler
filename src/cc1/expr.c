#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "data_structs.h"

expr *expr_new(func_fold *f_fold, func_gen *f_gen, func_str *f_str)
{
	expr *e = umalloc(sizeof *e);
	where_new(&e->where);
	e->tree_type = decl_new();
	e->f_fold = f_fold;
	e->f_gen  = f_gen;
	e->f_str  = f_str;
	return e;
}

expr *expr_new_intval(intval *iv)
{
	expr *e = expr_new_val();
	e->tree_type->type->spec |= spec_const;
	memcpy(&e->val.i, iv, sizeof e->val.i);
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

	ret = expr_new_op();
	memcpy(&ret->where, &e->where, sizeof e->where);

	decl_free(ret->tree_type);
	ret->tree_type = decl_copy(e->tree_type);

	ret->op   = op_multiply;

	ret->lhs  = e;
	ret->rhs  = expr_new_val();
	ret->rhs->val.i.val = sz;

	return ret;
}

expr *expr_assignment(expr *to, expr *from)
{
	expr *ass = expr_new_assign();

	ass->lhs = to;
	ass->rhs = from;

	return ass;
}
