#include "ops.h"
#include "expr_comma.h"

const char *str_expr_comma()
{
	return "comma";
}

void fold_const_expr_comma(expr *e, intval *piv, enum constyness *type)
{
	enum constyness ok[2];

	const_fold(e->lhs, piv, &ok[0]); /* piv should be overwritten */
	const_fold(e->rhs, piv, &ok[1]);

	if(ok[0] != CONST_NO && ok[1] == CONST_WITH_VAL)
		*type = CONST_WITH_VAL;
}

void fold_expr_comma(expr *e, symtable *stab)
{
	FOLD_EXPR(e->lhs, stab);
	fold_disallow_st_un(e->lhs, "comma-expr");

	FOLD_EXPR(e->rhs, stab);
	fold_disallow_st_un(e->lhs, "comma-expr");

	e->tree_type = e->rhs->tree_type;

	/* TODO: warn if either of the sub-exps are not freestanding */
	e->freestanding = e->lhs->freestanding || e->rhs->freestanding;
}

void gen_expr_comma(expr *e, symtable *stab)
{
	gen_expr(e->lhs, stab);
	out_pop();
	out_comment("unused comma expr");
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
