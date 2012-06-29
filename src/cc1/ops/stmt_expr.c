#include <string.h>

#include "ops.h"
#include "stmt_expr.h"

const char *str_stmt_expr()
{
	return "expr";
}

void fold_stmt_expr(stmt *s)
{
	fold_expr(s->expr, s->symtab);
	if(!s->freestanding && !s->expr->freestanding && !decl_is_void(s->expr->tree_type))
		cc1_warn_at(&s->expr->where, 0, 1, WARN_UNUSED_EXPR, "unused expression");
}

void gen_stmt_expr(stmt *s)
{
	gen_expr(s->expr, s->symtab);

	if(s->expr->tree_type->builtin)
		return;

	if((fopt_mode & FOPT_ENABLE_ASM) == 0
	|| !s->expr
	|| expr_kind(s->expr, funcall)
	|| !s->expr->spel
	|| strcmp(s->expr->spel, ASM_INLINE_FNAME))
	{
		asm_pop(NULL, ASM_REG_A);
		asm_comment("unused expr");
	}
}

static int expr_passable(stmt *s)
{
	/*
	 * TODO: ({}) - return inside?
	 * if we have a funcall marked noreturn, we're not passable
	 */
	if(expr_kind(s->expr, funcall))
		return !decl_attr_present(s->expr->tree_type->attr, attr_noreturn);

	if(expr_kind(s->expr, stmt))
		return fold_passable(s->expr->code);

	return 1;
}

void mutate_stmt_expr(stmt *s)
{
	s->f_passable = expr_passable;
}
