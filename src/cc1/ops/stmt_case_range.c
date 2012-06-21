#include "ops.h"
#include "stmt_case_range.h"

const char *str_stmt_case_range()
{
	return "case_range";
}

void fold_stmt_case_range(stmt *s)
{
	int l, r;

	fold_expr(s->expr,  s->symtab);
	fold_expr(s->expr2, s->symtab);

	if(const_fold(s->expr) || const_fold(s->expr2))
		DIE_AT(&s->where, "case range not constant");

	fold_test_expr(s->expr,  "case");
	fold_test_expr(s->expr2, "case");

	l = s->expr->val.iv.val;
	r = s->expr2->val.iv.val;

	if(l >= r)
		DIE_AT(&s->where, "case range equal or inverse");

	s->expr->spel = asm_label_case(CASE_RANGE, l);

	fold_stmt_and_add_to_curswitch(s);
}

void mutate_stmt_case_range(stmt *s)
{
	(void)s;
}

func_gen_stmt (*gen_stmt_case_range) = gen_stmt_label;
