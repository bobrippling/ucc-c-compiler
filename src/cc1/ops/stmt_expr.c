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
		cc1_warn_at(&s->expr->where, 0, WARN_UNUSED_EXPR, "unused expression");
}

void gen_stmt_expr(stmt *s)
{
	gen_expr(s->expr, s->symtab);
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
