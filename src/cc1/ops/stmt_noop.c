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

basic_blk *gen_stmt_noop(stmt *s, basic_blk *bb)
{
	(void)s;
	out_comment(bb, "noop");
	return bb;
}

basic_blk *style_stmt_noop(stmt *s, basic_blk *bb)
{
	(void)s;
	stylef(";");
	return bb;
}

void mutate_stmt_noop(stmt *s)
{
	s->f_passable = fold_passable_yes;
}
