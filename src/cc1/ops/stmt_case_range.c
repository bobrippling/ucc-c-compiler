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
	consty lk, rk;

	FOLD_EXPR(s->expr,  s->symtab);
	FOLD_EXPR(s->expr2, s->symtab);

	const_fold_need_val(s->expr,  &lk);
	const_fold_need_val(s->expr2, &rk);

	fold_need_expr(s->expr,  "case", 0);
	fold_need_expr(s->expr2, "case", 0);

	if(lk.bits.iv.val >= rk.bits.iv.val)
		DIE_AT(&s->where, "case range equal or inverse");

	s->expr->spel = out_label_case(CASE_RANGE, lk.bits.iv.val);

	fold_stmt_and_add_to_curswitch(s);
}

void mutate_stmt_case_range(stmt *s)
{
	s->f_passable = label_passable;
}

func_gen_stmt (*gen_stmt_case_range) = gen_stmt_label;
