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
	s->lbl_break    = out_label_flow("while_break");
	s->lbl_continue = out_label_flow("while_cont");

	fold_check_expr(
			s->expr,
			FOLD_CHK_NO_ST_UN | FOLD_CHK_BOOL,
			s->f_str());

	fold_stmt(s->lhs);
}

void gen_stmt_while(stmt *s)
{
	const char *endlbls[2];

	out_label(s->lbl_continue);

	flow_gen(s->flow, s->symtab, endlbls);
	gen_expr(s->expr);

	out_op_unary(op_not);
	out_jtrue(s->lbl_break);

	gen_stmt(s->lhs);

	out_push_lbl(s->lbl_continue, 0);
	out_jmp();

	flow_end(endlbls);
	out_label(s->lbl_break);
}

void style_stmt_while(stmt *s)
{
	stylef("while(");
	gen_expr(s->expr);
	stylef(")");
	gen_stmt(s->lhs);
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
