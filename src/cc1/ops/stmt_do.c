#include <stdlib.h>

#include "ops.h"
#include "stmt_do.h"
#include "../out/lbl.h"

const char *str_stmt_do(void)
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

	out_ctrl_transfer(octx, begin, NULL, NULL, 0);

	out_current_blk(octx, begin);
	{
		gen_stmt(s->lhs, octx);
		out_ctrl_transfer(octx, s->blk_continue, NULL, NULL, 0);
	}

	out_current_blk(octx, s->blk_continue);
	{
		cond = gen_expr(s->expr, octx);
		out_ctrl_branch(octx, cond, begin, s->blk_break);
	}

	out_current_blk(octx, s->blk_break);
}

void dump_stmt_do(const stmt *s, dump *ctx)
{
	dump_desc_stmt(ctx, "do-while", s);

	dump_inc(ctx);

	dump_stmt(s->lhs, ctx);
	dump_expr(s->expr, ctx);

	dump_dec(ctx);
}

void style_stmt_do(const stmt *s, out_ctx *octx)
{
	stylef("do");
	gen_stmt(s->lhs, octx);
	stylef("while(");
	IGNORE_PRINTGEN(gen_expr(s->expr, octx));
	stylef(");");
}

static int do_passable(stmt *s, int break_means_passable)
{
	(void)break_means_passable;

	if(!fold_passable(s->lhs, /* break */1)){
		/* do { cantpass(); } while(doesntmatter) */
		return 0;
	}

	if(const_expr_and_non_zero(s->expr)){
		/* do { no_break_or_goto(); } while(1) */
		return fold_infinite_loop_has_break_goto(s);
	}

	/* do { ... } while(0)
	 * do { ... } while(x) */
	return 1;
}

void init_stmt_do(stmt *s)
{
	s->f_passable = do_passable;
}
