#include <stdlib.h>
#include "ops.h"

const char *str_expr_if()
{
	return "if";
}

int fold_const_expr_if(expr *e)
{
	if(!const_fold(e->expr) && (e->expr2 ? !const_fold(e->expr2) : 1) && !const_fold(e->expr3)){
		expr_mutate_wrapper(e, val);

		e->val.iv.val = e->expr->val.iv.val ? (e->expr2 ? e->expr2->val.iv.val : e->expr->val.iv.val) : e->expr3->val.iv.val;
		return 0;
	}
	return 1;
}

void fold_expr_if(expr *e, symtable *stab)
{
	fold_expr(e->expr, stab);
	if(const_expr_is_const(e->expr))
		POSSIBLE_OPT(e->expr, "constant ?: expression");
	if(e->expr2)
		fold_expr(e->expr2, stab);
	fold_expr(e->expr3, stab);
	e->tree_type = decl_copy(e->expr3->tree_type); /* TODO: check they're the same */

	e->freestanding = e->expr2->freestanding || e->expr3->freestanding;
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
	gen_expr(e->expr2 ? e->expr2 : e->expr, stab);
	asm_temp(1, "jmp %s", lblfin);
	asm_label(lblelse);
	gen_expr(e->expr3, stab);
	asm_label(lblfin);

	free(lblfin);
	free(lblelse);
}

void gen_expr_str_if(expr *e, symtable *stab)
{
	(void)stab;
	idt_printf("if expression:\n");
	gen_str_indent++;
#define SUB_PRINT(nam) \
	do{\
		idt_printf(#nam  ":\n"); \
		gen_str_indent++; \
		print_expr(e->nam); \
		gen_str_indent--; \
	}while(0)

	SUB_PRINT(expr);
	if(e->expr2)
		SUB_PRINT(expr2);
	else
		idt_printf("?: syntactic sugar\n");

	SUB_PRINT(expr3);
#undef SUB_PRINT
}

expr *expr_new_if(expr *test, expr *a, expr *b)
{
	expr *e = expr_new_wrapper(if);
	e->expr = test;
	e->expr2 = a;
	e->expr3 = b;
	return e;
}
