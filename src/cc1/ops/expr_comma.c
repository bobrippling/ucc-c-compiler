#include "ops.h"
#include "expr_comma.h"
#include "../type_is.h"

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

void fold_expr_comma(expr *e, symtable *stab)
{
	FOLD_EXPR(e->lhs, stab);
	fold_check_expr(
			e->lhs,
			FOLD_CHK_NO_ST_UN | FOLD_CHK_ALLOW_VOID | FOLD_CHK_NOWARN_ASSIGN,
			"comma-expr");

	FOLD_EXPR(e->rhs, stab);
	fold_check_expr(
			e->rhs,
			FOLD_CHK_NO_ST_UN | FOLD_CHK_ALLOW_VOID | FOLD_CHK_NOWARN_ASSIGN,
			"comma-expr");

	e->tree_type = e->rhs->tree_type;

	if(!e->lhs->freestanding && !type_is_void(e->lhs->tree_type))
		cc1_warn_at(&e->lhs->where, unused_comma,
				"left hand side of comma is unused");

	e->freestanding = e->rhs->freestanding;
}

const out_val *gen_expr_comma(const expr *e, out_ctx *octx)
{
	out_val_consume(octx, gen_expr(e->lhs, octx));
	return gen_expr(e->rhs, octx);
}

const out_val *gen_expr_str_comma(const expr *e, out_ctx *octx)
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
	UNUSED_OCTX();
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

const out_val *gen_expr_style_comma(const expr *e, out_ctx *octx)
{
	IGNORE_PRINTGEN(gen_expr(e->lhs, octx));
	stylef(", ");
	return gen_expr(e->rhs, octx);
}
