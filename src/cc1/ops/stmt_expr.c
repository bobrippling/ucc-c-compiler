#include <string.h>

#include "ops.h"
#include "stmt_expr.h"
#include "../type_is.h"

const char *str_stmt_expr()
{
	return "expr";
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
	&& !type_is_void(s->expr->tree_type))
	{
		cc1_warn_at(&s->expr->where, unused_expr,
				"unused expression (%s)", expr_skip_casts(s->expr)->f_str());
	}
}

void gen_stmt_expr(const stmt *s, out_ctx *octx)
{
	out_val_consume(octx, gen_expr(s->expr, octx));
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
