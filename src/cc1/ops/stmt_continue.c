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

void gen_stmt_continue(stmt *s, out_ctx *octx)
{
	out_push_lbl(s->parent->lbl_continue, 0);
	out_jmp();
}

void style_stmt_continue(stmt *s)
{
	stylef("continue;");
	gen_stmt(s->lhs);
}

void init_stmt_continue(stmt *s)
{
	s->f_passable = fold_passable_no;
}
