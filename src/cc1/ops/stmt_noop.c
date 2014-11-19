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

void gen_stmt_noop(const stmt *s, out_ctx *octx)
{
	(void)s;
	(void)octx;
}

void style_stmt_noop(const stmt *s, out_ctx *octx)
{
	(void)s;
	(void)octx;
	stylef(";");
}

void init_stmt_noop(stmt *s)
{
	s->f_passable = fold_passable_yes;
}
