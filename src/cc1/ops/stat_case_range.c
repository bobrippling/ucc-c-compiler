#include "ops.h"
#include "stat_case_range.h"

const char *str_stat_case_range()
{
	return "case_range";
}

void fold_stat_case_range(stat *s)
{
	int l, r;

	fold_expr(s->expr,  s->symtab);
	fold_expr(s->expr2, s->symtab);

	if(const_fold(s->expr) || const_fold(s->expr2))
		die_at(&s->where, "case range not constant");

	fold_test_expr(s->expr,  "case");
	fold_test_expr(s->expr2, "case");

	l = s->expr->val.iv.val;
	r = s->expr2->val.iv.val;

	if(l >= r)
		die_at(&s->where, "case range equal or inverse");

	s->expr->spel = asm_label_case(CASE_RANGE, l);

	fold_stat_and_add_to_curswitch(s);
}

func_gen_stat (*gen_stat_case_range) = gen_stat_label;
