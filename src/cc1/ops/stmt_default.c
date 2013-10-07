#include <string.h>

#include "ops.h"
#include "stmt_default.h"
#include "../out/lbl.h"

const char *str_stmt_default()
{
	return "default";
}

void fold_stmt_default(stmt *s)
{
	char *lbl = out_label_case(CASE_DEF, 0);

	s->expr = expr_new_identifier(lbl);
	memcpy_safe(&s->expr->where, &s->where);

	s->expr->expr_is_default = 1;

	fold_stmt_and_add_to_curswitch(s);
}

void init_stmt_default(stmt *s)
{
	s->f_passable = label_passable;
}

STMT_LBL_DEFS(default);
