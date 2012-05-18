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
	fold_disallow_st_un(e->lhs, "comma-expr");

	fold_expr(e->rhs, stab);
	fold_disallow_st_un(e->lhs, "comma-expr");

	e->tree_type = decl_copy(e->rhs->tree_type);

	/* TODO: warn if either of the sub-exps are not freestanding */
	e->freestanding = e->lhs->freestanding || e->rhs->freestanding;
}

void gen_expr_comma(expr *e, symtable *stab)
{
	gen_expr(e->lhs, stab);
	asm_pop(e->lhs->tree_type, ASM_REG_A);
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

void mutate_expr_comma(expr *e)
{
	e->f_const_fold = fold_const_expr_comma;
}

void gen_expr_style_comma(expr *e, symtable *stab)
{ (void)e; (void)stab; /* TODO */ }
