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
	out_label(s->expr->spel);
	gen_stmt(s->lhs); /* the code-part of the compound statement */
}

int label_passable(stmt *s)
{
	return fold_passable(s->lhs);
}

void mutate_stmt_label(stmt *s)
{
	s->f_passable = label_passable;
}
