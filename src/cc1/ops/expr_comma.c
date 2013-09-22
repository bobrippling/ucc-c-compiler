#include "ops.h"
#include "expr_comma.h"

const char *str_expr_comma()
{
	return "comma";
}

static void fold_const_expr_comma(expr *e, consty *k)
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
	fold_check_expr(e->lhs, FOLD_CHK_NO_ST_UN, "comma-expr");

	FOLD_EXPR(e->rhs, stab);
	fold_check_expr(e->rhs, FOLD_CHK_NO_ST_UN, "comma-expr");

	e->tree_type = e->rhs->tree_type;

	if(!e->lhs->freestanding)
		warn_at(&e->lhs->where, "left hand side of comma is unused");

	e->freestanding = e->rhs->freestanding;
}

basic_blk *gen_expr_comma(expr *e, basic_blk *bb)
{
	bb = gen_expr(e->lhs, bb);
	out_pop(bb);
	out_comment(bb, "unused comma expr");
	bb = gen_expr(e->rhs, bb);
	return bb;
}

basic_blk *gen_expr_str_comma(expr *e, basic_blk *bb)
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

	return bb;
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

basic_blk *gen_expr_style_comma(expr *e, basic_blk *bb)
{
	bb = gen_expr(e->lhs, bb);
	stylef(", ");
	bb = gen_expr(e->rhs, bb);
	return bb;
}
