#include <string.h>

#include "ops.h"
#include "stmt_case.h"
#include "../out/lbl.h"

const char *str_stmt_case()
{
	return "case";
}

void fold_stmt_case(stmt *t)
{
	FOLD_EXPR(t->expr, t->symtab);
	fold_check_expr(t->expr, FOLD_CHK_INTEGRAL | FOLD_CHK_CONST_I, "case");

	fold_stmt_and_add_to_curswitch(t);
}

void gen_stmt_case(const stmt *s, out_ctx *octx)
{
	out_ctrl_transfer_make_current(octx, s->bits.case_blk);
	gen_stmt(s->lhs, octx);
}

void dump_stmt_case(const stmt *s, dump *ctx)
{
	dump_desc_stmt(ctx, "case", s);

	dump_inc(ctx);
	dump_expr(s->expr, ctx);
	dump_dec(ctx);

	dump_inc(ctx);
	dump_stmt(s->lhs, ctx);
	dump_dec(ctx);
}

void style_stmt_case(const stmt *s, out_ctx *octx)
{
	stylef("\ncase %ld: ", (long)const_fold_val_i(s->expr));
	gen_stmt(s->lhs, octx);
}

void init_stmt_case(stmt *s)
{
	s->f_passable = label_passable;
}
