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

	s->lbl_break = lbl;
	s->stmt_is_default = 1;

	fold_stmt_and_add_to_curswitch(s);
}

void gen_stmt_default(stmt *s)
{
	out_label(s->lbl_break);
	gen_stmt(s->lhs);
}

void style_stmt_default(stmt *s)
{
	stylef("\ndefault: ");
	gen_stmt(s->lhs);
}

void init_stmt_default(stmt *s)
{
	s->f_passable = label_passable;
}
