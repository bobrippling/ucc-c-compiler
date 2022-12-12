#include "../../util/str.h"

#include "ops.h"
#include "stmt_noop.h"

const char *str_stmt_noop(void)
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

void dump_stmt_noop(const stmt *s, dump *ctx)
{
	char buf[24];
	xsnprintf(
			buf, sizeof(buf),
			"no-op%s", s->bits.noop.is_fallthrough ? " <fallthrough>" : "");
	dump_desc_stmt(ctx, buf, s);
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
