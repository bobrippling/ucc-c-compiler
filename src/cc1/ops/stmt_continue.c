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

func_gen_stmt *gen_stmt_continue = gen_stmt_goto;
