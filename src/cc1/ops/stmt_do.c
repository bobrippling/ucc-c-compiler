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

void gen_stmt_do(const stmt *s, out_ctx *octx)
{
	const out_val *cond;
	out_blk *begin;

	begin = out_blk_new(octx, "do_begin");
	stmt_init_blks(s,
			out_blk_new(octx, "do_test"),
			out_blk_new(octx, "do_end"));

	out_ctrl_transfer(octx, begin, NULL, NULL);

	out_current_blk(octx, begin);
	{
		gen_stmt(s->lhs, octx);
		out_ctrl_transfer(octx, s->blk_continue, NULL, NULL);
	}

	out_current_blk(octx, s->blk_continue);
	{
		cond = gen_expr(s->expr, octx);
		out_ctrl_branch(octx, cond, begin, s->blk_break);
	}

	out_current_blk(octx, s->blk_break);
}

void style_stmt_do(const stmt *s, out_ctx *octx)
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
