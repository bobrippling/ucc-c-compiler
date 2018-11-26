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

	if(cc1_std >= STD_C99){
		/* commas are allowed in ICEs in C99+ */

		if(!k->nonstandard_const)
			k->nonstandard_const = klhs.nonstandard_const;

	}else{
		k->nonstandard_const = e;
	}

	if(!CONST_AT_COMPILE_TIME(klhs.type))
		CONST_FOLD_NO(k, e);
}

void fold_expr_comma(expr *e, symtable *stab)
{
	e->lhs = fold_expr_nonstructdecay(e->lhs, stab);
	fold_check_expr(
			e->lhs,
			FOLD_CHK_ALLOW_VOID | FOLD_CHK_NOWARN_ASSIGN,
			"comma-expr");

	e->rhs = fold_expr_nonstructdecay(e->rhs, stab);
	fold_check_expr(
			e->rhs,
			FOLD_CHK_ALLOW_VOID | FOLD_CHK_NOWARN_ASSIGN,
			"comma-expr");

	e->tree_type = e->rhs->tree_type;

	if(!e->lhs->freestanding
	&& !e->expr_comma_synthesized
	&& !type_is_void(e->lhs->tree_type))
	{
		cc1_warn_at(&e->lhs->where, unused_comma,
				"left hand side of comma is unused");
	}

	e->freestanding = e->rhs->freestanding;

	switch(expr_is_lval(e->rhs)){
		case LVALUE_NO:
			break;
		case LVALUE_STRUCT:
		case LVALUE_USER_ASSIGNABLE:
			/* comma expressions aren't lvalues,
			 * but we need their address for things like:
			 * struct A from = ...;
			 * struct A to = (0, from);
			 */
			e->f_islval = expr_is_lval_struct;
	}
}

const out_val *gen_expr_comma(const expr *e, out_ctx *octx)
{
	out_val_consume(octx, gen_expr(e->lhs, octx));

	return gen_expr(e->rhs, octx);
}

void dump_expr_comma(const expr *e, dump *ctx)
{
	dump_desc_expr(ctx, "comma", e);
	dump_inc(ctx);
	dump_expr(e->lhs, ctx);
	dump_dec(ctx);
	dump_inc(ctx);
	dump_expr(e->rhs, ctx);
	dump_dec(ctx);
}

expr *expr_new_comma2(expr *lhs, expr *rhs, int compiler_gen)
{
	expr *e = expr_new_comma();
	e->lhs = lhs, e->rhs = rhs;
	e->expr_comma_synthesized = compiler_gen;
	return e;
}

static int expr_comma_has_sideeffects(const expr *e)
{
	return expr_has_sideeffects(e->lhs) || expr_has_sideeffects(e->rhs);
}

void mutate_expr_comma(expr *e)
{
	e->f_const_fold = fold_const_expr_comma;
	e->f_has_sideeffects = expr_comma_has_sideeffects;
}

const out_val *gen_expr_style_comma(const expr *e, out_ctx *octx)
{
	IGNORE_PRINTGEN(gen_expr(e->lhs, octx));
	stylef(", ");
	return gen_expr(e->rhs, octx);
}
