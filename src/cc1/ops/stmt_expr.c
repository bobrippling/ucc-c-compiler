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
	asm_temp(1, "pop rax ; unused expr");
}
