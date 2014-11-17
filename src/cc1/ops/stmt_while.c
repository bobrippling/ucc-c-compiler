#include <stdlib.h>

#include "ops.h"
#include "stmt_while.h"
#include "stmt_if.h"
#include "../out/lbl.h"

const char *str_stmt_while()
{
	return "while";
}

void fold_stmt_while(stmt *s)
{
	FOLD_EXPR(s->expr, s->symtab);

	fold_check_expr(
			s->expr,
			FOLD_CHK_NO_ST_UN | FOLD_CHK_BOOL,
			s->f_str());

	fold_stmt(s->lhs);
}

void gen_stmt_while(const stmt *s, out_ctx *octx)
{
	struct out_dbg_lbl *endlbls[2][2];
	out_blk *blk_body = out_blk_new(octx, "while_body");

	stmt_init_blks(s,
			out_blk_new(octx, "while_cont"),
			out_blk_new(octx, "while_break"));

	out_ctrl_transfer(octx, s->blk_continue, NULL, NULL);

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

		out_ctrl_transfer(octx, s->blk_continue, NULL, NULL);
	}

	out_current_blk(octx, s->blk_break);
	{
		flow_end(s->flow, s->symtab, endlbls, octx);
	}
}

void style_stmt_while(const stmt *s, out_ctx *octx)
{
	stylef("while(");
	IGNORE_PRINTGEN(gen_expr(s->expr, octx));
	stylef(")");
	gen_stmt(s->lhs, octx);
}

int while_passable(stmt *s)
{
	if(const_expr_and_non_zero(s->expr))
		return fold_code_escapable(s); /* while(1) */

	return 1; /* fold_passable(s->lhs) - doesn't depend on this */
}

void init_stmt_while(stmt *s)
{
	s->f_passable = while_passable;
}
