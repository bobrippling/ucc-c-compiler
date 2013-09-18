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

	fold_check_expr(s->expr,
			FOLD_CHK_INTEGRAL | FOLD_CHK_CONST_I,
			"case-range");
	lv = const_fold_val_i(s->expr);

	fold_check_expr(s->expr2,
			FOLD_CHK_INTEGRAL | FOLD_CHK_CONST_I,
			"case-range");
	rv = const_fold_val_i(s->expr2);

	if(lv >= rv)
		die_at(&s->where, "case range equal or inverse");

	s->expr->bits.ident.spel = out_label_case(b_from, CASE_RANGE, lv);

	fold_stmt_and_add_to_curswitch(s);
}

void mutate_stmt_case_range(stmt *s)
{
	s->f_passable = label_passable;
}

STMT_LBL_DEFS(case_range);
