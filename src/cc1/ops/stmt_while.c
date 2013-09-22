#include <stdlib.h>

#include "ops.h"
#include "stmt_while.h"
#include "stmt_if.h"
#include "../out/lbl.h"
#include "../out/basic_block.h"

const char *str_stmt_while()
{
	return "while";
}

void fold_stmt_while(stmt *s)
{
	symtable *stab = s->symtab;

	flow_fold(s->flow, &stab);

	s->lbl_break    = out_label_flow("while_break");
	s->lbl_continue = out_label_flow("while_cont");

	FOLD_EXPR(s->expr, stab);
	fold_check_expr(
			s->expr,
			FOLD_CHK_NO_ST_UN | FOLD_CHK_BOOL,
			s->f_str());

	fold_stmt(s->lhs);
}

basic_blk *gen_stmt_while(stmt *s, basic_blk *bb)
{
  /*          +----------------+
	 *          v                |
	 * bb -> bb_exp ?-> bb_loop -^
	 *              ?-> bb_break
	 */
	struct basic_blk *bb_exp, *bb_loop, *bb_break;

	bb = flow_gen(s->flow, s->symtab, bb);

	bb_exp = bb_new();
	bb_link_forward(bb, bb_exp);

	bb_split_new(
			gen_expr(s->expr, bb_exp),
			&bb_loop,
			&bb_break);

	bb_loop = gen_stmt(s->lhs, bb_loop);
	bb_link_forward(bb_loop, bb_exp);

	return bb_break;
}

basic_blk *style_stmt_while(stmt *s, basic_blk *bb)
{
	stylef("while(");
	bb = gen_expr(s->expr, bb);
	stylef(")");
	bb = gen_stmt(s->lhs, bb);
	return bb;
}

int while_passable(stmt *s)
{
	if(const_expr_and_non_zero(s->expr))
		return fold_code_escapable(s); /* while(1) */

	return 1; /* fold_passable(s->lhs) - doesn't depend on this */
}

void mutate_stmt_while(stmt *s)
{
	s->f_passable = while_passable;
}
