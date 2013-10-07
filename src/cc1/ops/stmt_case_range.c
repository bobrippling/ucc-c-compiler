#include <string.h>

#include "ops.h"
#include "stmt_case_range.h"
#include "../out/lbl.h"

const char *str_stmt_case_range()
{
	return "case_range";
}

void fold_stmt_case_range(stmt *s)
{
	intval_t lv, rv;

	FOLD_EXPR(s->expr,  s->symtab);
	FOLD_EXPR(s->expr2, s->symtab);

	fold_need_expr(s->expr,  "case", 0);
	fold_need_expr(s->expr2, "case", 0);

	lv = const_fold_val(s->expr);
	rv = const_fold_val(s->expr2);

	if(lv >= rv)
		die_at(&s->where, "case range equal or inverse");

	s->expr->bits.ident.spel = out_label_case(CASE_RANGE, lv);

	fold_stmt_and_add_to_curswitch(s);
}

void gen_stmt_case_range(stmt *s)
{
	out_label(s->expr->bits.ident.spel);
	gen_stmt(s->lhs);
}

void style_stmt_case_range(stmt *s)
{
	stylef("\ncase %ld ... %ld: ",
			(long)const_fold_val(s->expr),
			(long)const_fold_val(s->expr2));

	gen_stmt(s->lhs);
}

void init_stmt_case_range(stmt *s)
{
	s->f_passable = label_passable;
}
