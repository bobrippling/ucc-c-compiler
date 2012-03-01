#include "ops.h"

const char *expr_str_comma()
{
	return "comma";
}

int fold_const_expr_comma(expr *e)
{
	return !const_fold(e->lhs) && !const_fold(e->rhs);
}

void expr_fold_comma(expr *e, symtable *stab)
{
	fold_expr(e->lhs, stab);
	fold_expr(e->rhs, stab);
	e->tree_type = decl_copy(e->rhs->tree_type);
}

void expr_gen_comma(expr *e, symtable *stab)
{
	gen_expr(e->lhs, stab);
	asm_temp(1, "pop rax ; unused comma expr");
	gen_expr(e->rhs, stab);
}

void expr_gen_str_comma(expr *e)
{
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
