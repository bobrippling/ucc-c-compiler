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
	out_label(bb, s->lbl_continue);

	bb = flow_gen(s->flow, s->symtab, bb);
	bb = gen_expr(s->expr, bb);

	out_op_unary(bb, op_not);
	out_jtrue(bb, s->lbl_break);

	bb = gen_stmt(s->lhs, bb);

	out_push_lbl(bb, s->lbl_continue, 0);
	out_jmp(bb);

	out_label(bb, s->lbl_break);

	return bb;
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
