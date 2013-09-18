#include "ops.h"
#include "stmt_noop.h"

const char *str_stmt_noop()
{
	return "noop";
}

void fold_stmt_noop(stmt *s)
{
	(void)s;
}

void gen_stmt_noop(stmt *s)
{
	(void)s;
	out_comment(b_from, "noop");
}

void style_stmt_noop(stmt *s)
{
	(void)s;
	stylef(";");
}

void mutate_stmt_noop(stmt *s)
{
	s->f_passable = fold_passable_yes;
}
