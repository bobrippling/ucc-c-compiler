#include "ops.h"
#include "expr_comma.h"

const char *str_expr_comma()
{
	return "comma";
}

void fold_const_expr_comma(expr *e, consty *k)
{
	consty klhs;

	const_fold(e->lhs, &klhs);
	const_fold(e->rhs, k);

	if(!CONST_AT_COMPILE_TIME(klhs.type))
		k->type = CONST_NO;
}

void fold_expr_comma(expr *e, symtable *stab)
{
	FOLD_EXPR(e->lhs, stab);
	fold_disallow_st_un(e->lhs, "comma-expr");

	FOLD_EXPR(e->rhs, stab);
	fold_disallow_st_un(e->lhs, "comma-expr");

	e->tree_type = e->rhs->tree_type;

	if(!e->lhs->freestanding)
		WARN_AT(&e->lhs->where, "left hand side of comma is unused");

	e->freestanding = e->rhs->freestanding;
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

expr *expr_new_comma2(expr *lhs, expr *rhs)
{
	expr *e = expr_new_comma();
	e->lhs = lhs, e->rhs = rhs;
	return e;
}

void mutate_expr_comma(expr *e)
{
	e->f_const_fold = fold_const_expr_comma;
}

void gen_expr_style_comma(expr *e, symtable *stab)
{ (void)e; (void)stab; /* TODO */ }
