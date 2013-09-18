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
	out_label(b_from, s->expr->bits.ident.spel);
	gen_stmt(s->lhs); /* the code-part of the compound statement */
}

void style_stmt_label(stmt *s)
{
	stylef("\n%s: ", s->expr->bits.ident.spel);
	gen_stmt(s->lhs);
}

int label_passable(stmt *s)
{
	return fold_passable(s->lhs);
}

void mutate_stmt_label(stmt *s)
{
	s->f_passable = label_passable;
}
