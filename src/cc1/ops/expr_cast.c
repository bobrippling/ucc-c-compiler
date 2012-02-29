#include "../data_structs.h"
#include "expr_sizeof.h"

void fold_const_expr_cast(expr *e)
{
	return const_fold(e->rhs);
}

void fold_expr_cast(expr *e, symtable *stab)
{
	fold_expr(e->rhs, stab);
	if(e->rhs->type == expr_cast){
		/* get rid of e->rhs, replace with e->rhs->rhs */
		expr *del = e->rhs;

		e->rhs = e->rhs->rhs;

		expr_free(del->lhs); /* the overridden cast */
		expr_free(del);
	}
	GET_TREE_TYPE(e->lhs->tree_type);
}

void gen_expr_cast(expr *e, symtable *stab)
{
	/* ignore the lhs, it's just a type spec */
	/* FIXME: size changing? */
	gen_expr(e->rhs, stab);
}
