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

void gen_stmt_do(stmt *s)
{
	char *begin = out_label_flow(b_from, "do_start");

	out_label(b_from, begin);
	gen_stmt(s->lhs);

	out_label(b_from, s->lbl_continue);
	gen_expr(s->expr);

	out_jtrue(b_from, begin);

	out_label(b_from, s->lbl_break);

	free(begin);
}

void style_stmt_do(stmt *s)
{
	stylef("do");
	gen_stmt(s->lhs);
	stylef("while(");
	gen_expr(s->expr);
	stylef(");");
}

void mutate_stmt_do(stmt *s)
{
	s->f_passable = while_passable;
}
