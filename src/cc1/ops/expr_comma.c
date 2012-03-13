#include "ops.h"

const char *str_expr_comma()
{
	return "comma";
}

int fold_const_expr_comma(expr *e)
{
	return !const_fold(e->expr) && !const_fold(e->expr2);
}

void fold_expr_comma(expr *e, symtable *stab)
{
	fold_expr(e->expr, stab);
	fold_expr(e->expr2, stab);
	e->tree_type = decl_copy(e->expr2->tree_type);
}

void gen_expr_comma(expr *e, symtable *stab)
{
	gen_expr(e->expr, stab);
	asm_temp(1, "pop rax ; unused comma expr");
	gen_expr(e->expr2, stab);
}

void gen_expr_str_comma(expr *e, symtable *stab)
{
	(void)stab;
	idt_printf("comma expression\n");
	idt_printf("comma lhs:\n");
	gen_str_indent++;
	print_expr(e->expr);
	gen_str_indent--;
	idt_printf("comma rhs:\n");
	gen_str_indent++;
	print_expr(e->expr2);
	gen_str_indent--;
}

expr *expr_new_comma(expr *l, expr *r)
{
	expr *e = expr_new_wrapper(comma);
	e->f_const_fold = fold_const_expr_comma;
	e->expr  = l;
	e->expr2 = r;
	return e;
}
