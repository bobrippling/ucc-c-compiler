#include "ops.h"
#include "stmt_continue.h"
#include "stmt_break.h"

const char *str_stmt_continue()
{
	return "continue";
}

void fold_stmt_continue(stmt *t)
{
	fold_stmt_break_continue(t, t->parent);
}

void gen_stmt_continue(stmt *s, out_ctx *octx)
{
	(void)octx;
	out_ctrl_transfer(octx, s->parent->blk_continue, NULL);
}

void style_stmt_continue(stmt *s, out_ctx *octx)
{
	stylef("continue;");
	gen_stmt(s->lhs, octx);
}

void init_stmt_continue(stmt *s)
{
	s->f_passable = fold_passable_no;
}
