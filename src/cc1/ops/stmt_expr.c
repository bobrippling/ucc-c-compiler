#include <string.h>

#include "ops.h"
#include "stmt_expr.h"
#include "../type_is.h"

const char *str_stmt_expr()
{
	return "expression";
}

static int unused_expr_type(expr *e)
{
	type *t = e->tree_type;

	if(type_is_void(t))
		return 0;
	if(type_qual(t) & qual_volatile)
		return 0;

	/* check decayed exprs */
	if(expr_kind(e, cast) && expr_cast_is_lval2rval(e))
		return unused_expr_type(expr_cast_child(e));

	return 1;
}

void fold_stmt_expr(stmt *s)
{
	int folded = !s->expr->tree_type;

	fold_expr_nodecay(s->expr, s->symtab);

	if(type_qual(s->expr->tree_type) & qual_volatile){
		/* must generate a read */
		FOLD_EXPR(s->expr, s->symtab);
	}

	if(!folded
	&& !s->freestanding
	&& !s->expr->freestanding
	&& unused_expr_type(s->expr))
	{
		cc1_warn_at(&s->expr->where, unused_expr,
				"unused expression (%s)", expr_str_friendly(s->expr));
	}
}

static int expr_volatile_skipcasts(expr *e)
{
	if(type_qual(e->tree_type) & qual_volatile)
		return 1;

	return expr_kind(e, cast)
		&& expr_cast_is_lval2rval(e)
		&& type_qual(expr_cast_child(e)->tree_type) & qual_volatile;
}

void gen_stmt_expr(const stmt *s, out_ctx *octx)
{
	const out_val *v = gen_expr(s->expr, octx);

	if(expr_volatile_skipcasts(s->expr))
		out_force_read(octx, s->expr->tree_type, v);
	else
		out_val_consume(octx, v);
}

void dump_stmt_expr(const stmt *s, dump *ctx)
{
	dump_desc_stmt(ctx, "expression", s);

	dump_inc(ctx);

	dump_expr(s->expr, ctx);

	dump_dec(ctx);
}

void style_stmt_expr(const stmt *s, out_ctx *octx)
{
	IGNORE_PRINTGEN(gen_expr(s->expr, octx));
	stylef(";\n");
}

static int expr_passable(stmt *s)
{
	/*
	 * TODO: ({}) - return inside?
	 * if we have a funcall marked noreturn, we're not passable
	 */
	if(expr_kind(s->expr, funcall))
		return expr_func_passable(s->expr);

	if(expr_kind(s->expr, stmt))
		return fold_passable(s->expr->code);

	return 1;
}

void init_stmt_expr(stmt *s)
{
	s->f_passable = expr_passable;
}
