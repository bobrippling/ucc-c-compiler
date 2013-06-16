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
	intval lv, rv;

	FOLD_EXPR(s->expr,  s->symtab);
	FOLD_EXPR(s->expr2, s->symtab);

	fold_need_expr(s->expr,  "case", 0);
	fold_need_expr(s->expr2, "case", 0);

	const_fold_need_val(s->expr,  &lv);
	const_fold_need_val(s->expr2, &rv);

	if(lv.val >= rv.val)
		DIE_AT(&s->where, "case range equal or inverse");

	s->expr->bits.ident.spel = out_label_case(CASE_RANGE, lv.val);

	fold_stmt_and_add_to_curswitch(s);
}

void mutate_stmt_case_range(stmt *s)
{
	s->f_passable = label_passable;
}

STMT_LBL_DEFS(case_range);
