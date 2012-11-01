#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "data_structs.h"
#include "cc1.h"
#include "const.h"

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
	e->f_static_addr = NULL;
	e->f_lea = NULL;

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

expr *expr_new_decl_init(decl *d, decl_init *di)
{
	ICE("TODO - only allow simple expr inits");
	(void)d;
	(void)di;
	/*UCC_ASSERT(d->init, "no init");
	return expr_new_assign_init(expr_new_identifier(d->spel), d->init);*/
	return 0;
}

#if 0
expr *expr_new_array_decl_init(decl *d, int ival, int idx)
{
	expr *sum;

	UCC_ASSERT(d->init, "no init");

	sum = expr_new_op(op_plus);

	sum->lhs = expr_new_identifier(d->spel);
	sum->rhs = expr_new_val(idx);

	return expr_new_assign(expr_new_deref(sum), expr_new_val(ival));
}
#endif

int expr_is_null_ptr(expr *e)
{
	return const_expr_and_zero(e)
	&& (type_ref_is_void_ptr(e->tree_type) || type_ref_is_integral(e->tree_type));
}
