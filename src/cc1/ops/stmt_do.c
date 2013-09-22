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

basic_blk *gen_stmt_do(stmt *s, basic_blk *bb)
{
	char *begin = out_label_flow("do_start");

	out_label(bb, begin);
	bb = gen_stmt(s->lhs, bb);

	out_label(bb, s->lbl_continue);
	bb = gen_expr(s->expr, bb);

	out_jtrue(bb, begin);

	out_label(bb, s->lbl_break);

	free(begin);

	return bb;
}

basic_blk *style_stmt_do(stmt *s, basic_blk *bb)
{
	stylef("do");
	bb = gen_stmt(s->lhs, bb);
	stylef("while(");
	bb = gen_expr(s->expr, bb);
	stylef(");");

	return bb;
}

void mutate_stmt_do(stmt *s)
{
	s->f_passable = while_passable;
}
