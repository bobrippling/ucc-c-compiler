#include <stdlib.h>

#include "ops.h"
#include "stmt_do.h"
#include "../out/lbl.h"
#include "../out/basic_block/bb.h"

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
	basic_blk *loop = bb_new_from(bb, "do_loop"),
	          *b_break = bb_new_from(bb, "do_break"),
	          *loop_head = loop;
	basic_blk_phi *phi;

	bb_link_forward(bb, loop);

	loop = gen_stmt(s->lhs, loop);
	loop = gen_expr(s->expr, loop);

	bb_split(loop, loop_head, b_break, &phi);

	bb_phi_incoming(phi, b_break);

	return bb_phi_next(phi);
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
