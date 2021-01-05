#include <string.h>
#include <assert.h>

#include "ops.h"
#include "stmt_expr.h"
#include "../type_is.h"

#include "expr_funcall.h"
#include "expr_stmt.h"
#include "expr_cast.h"
#include "__builtin.h"

const char *str_stmt_expr(void)
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
				"unused expression (%s)", expr_str_friendly(s->expr, 0));
	}
}

void gen_stmt_expr(const stmt *s, out_ctx *octx)
{
	const out_val *v = gen_expr(s->expr, octx);

	expr *const lvalue = expr_kind(s->expr, cast) && expr_cast_is_lval2rval(s->expr)
		? expr_cast_child(s->expr)
		: NULL;

	const int is_volatile = type_qual(s->expr->tree_type) & qual_volatile
		|| (lvalue && type_qual(lvalue->tree_type) & qual_volatile);

	if(is_volatile){
		assert(lvalue && "only lvalues can be the source of volatile");

		out_force_read(octx, s->expr->tree_type, v);
	}else{
		out_val_consume(octx, v);
	}
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

static int expr_passable(stmt *s, int break_means_passable)
{
	(void)break_means_passable;
	/*
	 * TODO: ({}) - return inside?
	 * if we have a funcall marked noreturn, we're not passable
	 */
	if(expr_kind(s->expr, funcall))
		return expr_func_passable(s->expr);

	if(expr_kind(s->expr, stmt))
		return fold_passable(s->expr->code, 0);

	if(expr_kind(s->expr, builtin))
		return !func_or_builtin_attr_present(s->expr, attr_noreturn);

	return 1;
}

void init_stmt_expr(stmt *s)
{
	s->f_passable = expr_passable;
}
