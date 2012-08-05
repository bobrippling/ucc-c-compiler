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
	intval val;

	fold_expr(t->expr, t->symtab);
	fold_need_expr(t->expr, "case", 0);
	const_fold_need_val(t->expr, &val);

	t->expr->spel = out_label_case(CASE_CASE, val.val);

	fold_stmt_and_add_to_curswitch(t);
}

void mutate_stmt_case(stmt *s)
{
	s->f_passable = label_passable;
}

func_gen_stmt *gen_stmt_case = gen_stmt_label;
