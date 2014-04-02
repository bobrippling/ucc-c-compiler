#include <stdlib.h>

#include "ops.h"
#include "stmt_do.h"
#include "../out/lbl.h"

const char *str_stmt_do()
{
	return "do";
}

void fold_stmt_do(stmt *s)
{
	fold_stmt_while(s);
}

void gen_stmt_do(stmt *s, out_ctx *octx)
{
	out_blk *begin = out_blk_new("do_start");
	out_blk *end = out_blk_new("do_end");
	out_val *cond;

	out_current_blk(octx, begin);
	gen_stmt(s->lhs, octx);

	cond = gen_expr(s->expr, octx);
	out_ctrl_branch(octx, cond, begin, end);

	out_current_blk(octx, end);
}

void style_stmt_do(stmt *s, out_ctx *octx)
{
	stylef("do");
	gen_stmt(s->lhs, octx);
	stylef("while(");
	IGNORE_PRINTGEN(gen_expr(s->expr, octx));
	stylef(");");
}

void init_stmt_do(stmt *s)
{
	s->f_passable = while_passable;
}
