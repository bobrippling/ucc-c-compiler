#include "ops.h"
#include "stmt_continue.h"
#include "stmt_break.h"

const char *str_stmt_continue()
{
	return "continue";
}

void fold_stmt_continue(stmt *t)
{
	fold_stmt_break_continue(t, t->parent ? t->parent->lbl_continue : NULL);
}

void init_stmt_continue(stmt *s)
{
	s->f_passable = fold_passable_no;
}

STMT_GOTO_DEFS(continue);
