#include "../data_structs.h"
#include "expr_sizeof.h"

void fold_const_expr_if(expr *e)
{
	if(!const_fold(e->expr) && (e->lhs ? !const_fold(e->lhs) : 1) && !const_fold(e->rhs)){
		e->type = expr_val;
		e->val.i.val = e->expr->val.i.val ? (e->lhs ? e->lhs->val.i.val : e->expr->val.i.val) : e->rhs->val.i.val;
		return 0;
	}
}

void fold_expr_if(expr *e, symtable *stab)
{
	fold_expr(e->expr, stab);
	if(const_expr_is_const(e->expr))
		POSSIBLE_OPT(e->expr, "constant ?: expression");
	if(e->lhs)
		fold_expr(e->lhs, stab);
	fold_expr(e->rhs, stab);
	GET_TREE_TYPE(e->rhs->tree_type); /* TODO: check they're the same */
}


void gen_expr_if(expr *e, symtable *stab)
{
	char *lblfin, *lblelse;
	lblfin  = asm_label_code("ifexpa");
	lblelse = asm_label_code("ifexpb");

	gen_expr(e->expr, stab);
	asm_temp(1, "pop rax");
	asm_temp(1, "test rax, rax");
	asm_temp(1, "jz %s", lblelse);
	gen_expr(e->lhs ? e->lhs : e->expr, stab);
	asm_temp(1, "jmp %s", lblfin);
	asm_label(lblelse);
	gen_expr(e->rhs, stab);
	asm_label(lblfin);

	free(lblfin);
	free(lblelse);
}
