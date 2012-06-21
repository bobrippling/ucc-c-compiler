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

static int continue_passable(stmt *s)
{
	(void)s;
	return 0;
}

void mutate_stmt_continue(stmt *s)
{
	s->f_passable = continue_passable;
}

func_gen_stmt *gen_stmt_continue = gen_stmt_goto;
