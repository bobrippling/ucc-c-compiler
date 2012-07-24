#include <string.h>

#include "ops.h"
#include "stmt_case.h"

const char *str_stmt_case()
{
	return "case";
}

void fold_stmt_case(stmt *t)
{
	intval val;
	enum constyness const_type;

	fold_expr(t->expr, t->symtab);

	fold_need_expr(t->expr, "case", 0);

	const_fold(t->expr, &val, &const_type);
	if(const_type == CONST_NO)
		DIE_AT(&t->expr->where, "case expression not constant (%s)", t->expr->f_str());

	t->expr->spel = asm_label_case(CASE_CASE, val.val);

	fold_stmt_and_add_to_curswitch(t);
}

void mutate_stmt_case(stmt *s)
{
	s->f_passable = label_passable;
}

func_gen_stmt *gen_stmt_case = gen_stmt_label;
