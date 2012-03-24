#include "ops.h"
#include "stmt_label.h"

const char *str_stmt_label()
{
	return "label";
}

void fold_stmt_label(stmt *s)
{
	fold_stmt_goto(s);
	fold_stmt(s->lhs); /* compound */
}

void gen_stmt_label(stmt *s)
{
	asm_label(s->expr->spel);
	gen_stmt(s->lhs); /* the code-part of the compound statement */
}
