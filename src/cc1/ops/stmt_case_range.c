#include "ops.h"
#include "stmt_case_range.h"

const char *str_stmt_case_range()
{
	return "case_range";
}

void fold_stmt_case_range(stmt *s)
{
	intval lval, rval;
	enum constyness lk, rk;

	fold_expr(s->expr,  s->symtab);
	fold_expr(s->expr2, s->symtab);

	const_fold(s->expr,  &lval, &lk);
	const_fold(s->expr2, &rval, &rk);

	if(lk == CONST_NO || rk == CONST_NO)
		DIE_AT(&s->where, "case range not constant");

	fold_need_expr(s->expr,  "case", 0);
	fold_need_expr(s->expr2, "case", 0);

	if(lval.val >= rval.val)
		DIE_AT(&s->where, "case range equal or inverse");

	s->expr->spel = asm_label_case(CASE_RANGE, lval.val);

	fold_stmt_and_add_to_curswitch(s);
}

void mutate_stmt_case_range(stmt *s)
{
	s->f_passable = label_passable;
}

func_gen_stmt (*gen_stmt_case_range) = gen_stmt_label;
