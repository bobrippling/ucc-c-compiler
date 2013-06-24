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
	integral_t lv, rv;

	FOLD_EXPR(s->expr,  s->symtab);
	FOLD_EXPR(s->expr2, s->symtab);

	fold_check_expr(s->expr,  FOLD_CHK_EXP, "case");
	fold_check_expr(s->expr2, FOLD_CHK_EXP, "case");

	lv = const_fold_val_i(s->expr);
	rv = const_fold_val_i(s->expr2);

	if(lv >= rv)
		DIE_AT(&s->where, "case range equal or inverse");

	s->expr->bits.ident.spel = out_label_case(CASE_RANGE, lv);

	fold_stmt_and_add_to_curswitch(s);
}

void mutate_stmt_case_range(stmt *s)
{
	s->f_passable = label_passable;
}

STMT_LBL_DEFS(case_range);
