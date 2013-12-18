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

	/* klhs.nonstandard_const || k->nonstandard_const
	 * ^ doesn't matter - comma expressions are nonstandard-const anyway
	 */
	k->nonstandard_const = e;

	if(!CONST_AT_COMPILE_TIME(klhs.type))
		k->type = CONST_NO;
}

static void comma_lea(expr *e)
{
	gen_unused_expr(e->lhs);
	out_pop();
	lea_expr(e->rhs);
}

void fold_expr_comma(expr *e, symtable *stab)
{
	FOLD_EXPR(e->lhs, stab);
	fold_check_expr(
			e->lhs,
			FOLD_CHK_ALLOW_VOID,
			"comma-expr");

	FOLD_EXPR(e->rhs, stab);
	fold_check_expr(
			e->rhs,
			FOLD_CHK_ALLOW_VOID,
			"comma-expr");

	e->tree_type = e->rhs->tree_type;

	if(!e->lhs->freestanding && !type_ref_is_void(e->lhs->tree_type))
		warn_at(&e->lhs->where, "left hand side of comma is unused");

	e->freestanding = e->rhs->freestanding;

	if(expr_is_lval(e->rhs)){
		e->f_lea = comma_lea;
		e->lvalue_internal = 1;
	}
}

void gen_expr_comma(expr *e)
{
	/* attempt lea, don't want full struct dereference */
	gen_unused_expr(e->lhs);

	out_pop();
	out_comment("unused comma expr");
	gen_expr(e->rhs);
}

void gen_expr_str_comma(expr *e)
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

void gen_expr_style_comma(expr *e)
{
	gen_expr(e->lhs);
	stylef(", ");
	gen_expr(e->rhs);
}
