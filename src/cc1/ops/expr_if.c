#include <stdlib.h>
#include "ops.h"

const char *str_expr_if()
{
	return "if";
}

int fold_const_expr_if(expr *e)
{
	if(!const_fold(e->expr) && (e->lhs ? !const_fold(e->lhs) : 1) && !const_fold(e->rhs)){
		expr_mutate_wrapper(e, val);

		e->val.iv.val = e->expr->val.iv.val ? (e->lhs ? e->lhs->val.iv.val : e->expr->val.iv.val) : e->rhs->val.iv.val;
		return 0;
	}
	return 1;
}

void fold_expr_if(expr *e, symtable *stab)
{
	fold_expr(e->expr, stab);
	if(const_expr_is_const(e->expr))
		POSSIBLE_OPT(e->expr, "constant ?: expression");

	fold_test_expr(e->expr, "?: expr");

	if(e->lhs)
		fold_expr(e->lhs, stab);
	fold_expr(e->rhs, stab);
	e->tree_type = decl_copy(e->rhs->tree_type); /* TODO: check they're the same */

	e->freestanding = e->lhs->freestanding || e->rhs->freestanding;
}


void gen_expr_if(expr *e, symtable *stab)
{
	char *lblfin, *lblelse;
	lblfin  = asm_label_code("ifexpa");
	lblelse = asm_label_code("ifexpb");

	gen_expr(e->expr, stab);
	asm_pop( e->expr->tree_type, ASM_REG_A);
	ASM_TEST(e->expr->tree_type, ASM_REG_A);
	asm_jmp_if_zero(0, lblelse);

	gen_expr(e->lhs ? e->lhs : e->expr, stab);

	asm_jmp(lblfin);

	asm_label(lblelse);
	gen_expr(e->rhs, stab);
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
	if(e->lhs)
		SUB_PRINT(lhs);
	else
		idt_printf("?: syntactic sugar\n");

	SUB_PRINT(rhs);
#undef SUB_PRINT
}

void mutate_expr_if(expr *e)
{
	(void)e;
}

expr *expr_new_if(expr *test)
{
	expr *e = expr_new_wrapper(if);
	e->expr = test;
	return e;
}

void gen_expr_style_if(expr *e, symtable *stab)
{ (void)e; (void)stab; /* TODO */ }
