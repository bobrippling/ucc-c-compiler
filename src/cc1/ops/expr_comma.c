#include "../data_structs.h"
#include "expr_sizeof.h"

int fold_const_expr_comma(expr *e)
{
	return !const_fold(e->lhs) && !const_fold(e->rhs);
}

void fold_expr_comma(expr *e, symtable *stab)
{
	fold_expr(e->lhs, stab);
	fold_expr(e->rhs, stab);
	GET_TREE_TYPE(e->rhs->tree_type);
}

void gen_expr_comma()
{
	gen_expr(e->lhs, stab);
	asm_temp(1, "pop rax ; unused comma expr");
	gen_expr(e->rhs, stab);
}
