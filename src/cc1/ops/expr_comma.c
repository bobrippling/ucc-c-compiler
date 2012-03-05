#include "ops.h"

const char *str_expr_comma()
{
	return "comma";
}

int fold_const_expr_comma(expr *e)
{
	return !const_fold(e->lhs) && !const_fold(e->rhs);
}

void fold_expr_comma(expr *e, symtable *stab)
{
	fold_expr(e->lhs, stab);
	fold_expr(e->rhs, stab);
	e->tree_type = decl_copy(e->rhs->tree_type);
}

void gen_expr_comma(expr *e, symtable *stab)
{
	gen_expr(e->lhs, stab);
	asm_pop(ASM_REG_A);
	asm_comment("unused comma expr");
	gen_expr(e->rhs, stab);
}

void gen_expr_str_comma(expr *e, symtable *stab)
{
	(void)stab;
	idt_printf("comma expression\n");
	idt_printf("comma lhs:\n");
	gen_str_indent++;
	print_expr(e->lhs);
	gen_str_indent--;
	idt_printf("comma rhs:\n");
	gen_str_indent++;
	print_expr(e->rhs);
	gen_str_indent--;
}

expr *expr_new_comma()
{
	expr *e = expr_new_wrapper(comma);
	e->f_const_fold = fold_const_expr_comma;
	return e;
}
