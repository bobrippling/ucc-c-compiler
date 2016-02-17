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

void gen_ir_stmt_do(const stmt *s, irctx *ctx)
{
	const unsigned blk_begin = ctx->curlbl++;
	const unsigned blk_continue = ctx->curlbl++;
	const unsigned blk_break = ctx->curlbl++;

	stmt_init_ir_blks(s, blk_continue, blk_break);

	printf("$%u:\n", blk_begin);
	{
		gen_ir_stmt(s->lhs, ctx);
		printf("jmp $%u\n", blk_continue);
	}

	printf("$%u:\n", blk_continue);
	{
		const unsigned val_test = ctx->curlbl++;
		irval *cond = gen_ir_expr(s->expr, ctx);

		printf("$%u = ne %s 0, %s\n",
				val_test,
				irtype_str(s->expr->tree_type, ctx),
				irval_str(cond, ctx));

		printf("br $%u, $%u, $%u\n",
				val_test,
				blk_begin,
				blk_break);
	}

	printf("$%u:\n", blk_break);
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

void init_stmt_do(stmt *s)
{
	s->f_passable = while_passable;
}
