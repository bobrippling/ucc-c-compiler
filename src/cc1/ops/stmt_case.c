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
	integral_t val;

	FOLD_EXPR(t->expr, t->symtab);
	fold_check_expr(t->expr, FOLD_CHK_INTEGRAL | FOLD_CHK_CONST_I, "case");
	val = const_fold_val_i(t->expr);

	t->expr->bits.ident.spel = out_label_case(b_from, CASE_CASE, val);

	fold_stmt_and_add_to_curswitch(t);
}

void mutate_stmt_case(stmt *s)
{
	s->f_passable = label_passable;
}

STMT_LBL_DEFS(case);
