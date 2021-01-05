#include <stdlib.h>

#include "ops.h"
#include "stmt_while.h"
#include "stmt_if.h"
#include "../out/lbl.h"

const char *str_stmt_while(void)
{
	return "while";
}

void fold_stmt_while(stmt *s)
{
	FOLD_EXPR(s->expr, s->symtab);

	(void)!fold_check_expr(
			s->expr,
			FOLD_CHK_NO_ST_UN | FOLD_CHK_BOOL,
			s->f_str());

	fold_stmt(s->lhs);
}

void gen_stmt_while(const stmt *s, out_ctx *octx)
{
	struct out_dbg_lbl *endlbls[2][2];
	out_blk *blk_body = out_blk_new(octx, "while_body");

	{
		out_blk *blk_cont = out_blk_new(octx, "while_cont");
		out_blk *blk_break = out_blk_new(octx, "while_break");
		stmt_init_blks(s, blk_cont, blk_break);
	}

	out_ctrl_transfer(octx, s->blk_continue, NULL, NULL, 0);

	out_current_blk(octx, s->blk_continue);
	{
		const out_val *cond;

		flow_gen(s->flow, s->symtab, endlbls, octx);
		cond = gen_expr(s->expr, octx);

		out_ctrl_branch(octx, cond, blk_body, s->blk_break);
	}

	out_current_blk(octx, blk_body);
	{
		gen_stmt(s->lhs, octx);

		out_ctrl_transfer(octx, s->blk_continue, NULL, NULL, 0);
	}

	out_current_blk(octx, s->blk_break);
	{
		flow_end(s->flow, s->symtab, endlbls, octx);
	}
}

void dump_stmt_while(const stmt *s, dump *ctx)
{
	dump_desc_stmt(ctx, "while", s);

	dump_inc(ctx);
	dump_flow(s->flow, ctx);
	dump_expr(s->expr, ctx);
	dump_stmt(s->lhs, ctx);
	dump_dec(ctx);
}

void style_stmt_while(const stmt *s, out_ctx *octx)
{
	stylef("while(");
	IGNORE_PRINTGEN(gen_expr(s->expr, octx));
	stylef(")");
	gen_stmt(s->lhs, octx);
}

static int while_passable(stmt *s, int break_means_passable)
{
	(void)break_means_passable;

	if(const_expr_and_non_zero(s->expr))
		return fold_infinite_loop_has_break_goto(s->lhs); /* while(1) */

	return 1; /* fold_passable(s->lhs) - doesn't depend on this */
}

void init_stmt_while(stmt *s)
{
	s->f_passable = while_passable;
}
