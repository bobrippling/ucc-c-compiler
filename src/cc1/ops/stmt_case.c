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
	integral_t val;

	FOLD_EXPR(t->expr, t->symtab);
	fold_check_expr(t->expr, FOLD_CHK_INTEGRAL | FOLD_CHK_CONST_I, "case");
	val = const_fold_val_i(t->expr);

	t->bits.case_lbl = out_label_case(CASE_CASE, val);
	fold_stmt_and_add_to_curswitch(t, &t->bits.case_lbl);
}

void gen_stmt_case(stmt *s)
{
	out_label(s->bits.case_lbl);
	gen_stmt(s->lhs);
}

void style_stmt_case(stmt *s)
{
	stylef("\ncase %ld: ", (long)const_fold_val_i(s->expr));
	gen_stmt(s->lhs);
}

void init_stmt_case(stmt *s)
{
	s->f_passable = label_passable;
}
