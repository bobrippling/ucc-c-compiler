#include <string.h>

#include "ops.h"
#include "stat_expr.h"

const char *str_stat_expr()
{
	return "expr";
}

void fold_stat_expr(stat *s)
{
	fold_expr(s->expr, s->symtab);
	if(!s->expr->freestanding)
		cc1_warn_at(&s->expr->where, 0, WARN_UNUSED_EXPR, "unused expression");
}

void gen_stat_expr(stat *s)
{
	gen_expr(s->expr, s->symtab);
	if((fopt_mode & FOPT_ENABLE_ASM) == 0
	|| !s->expr
	|| expr_kind(s->expr, funcall)
	|| !s->expr->spel
	|| strcmp(s->expr->spel, ASM_INLINE_FNAME))
	{
		asm_temp(1, "pop rax ; unused expr");
	}
}
