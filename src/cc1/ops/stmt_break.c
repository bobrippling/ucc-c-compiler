#include <string.h>

#include "ops.h"
#include "stmt_break.h"
#include "../type_is.h"
#include "../type_nav.h"

const char *str_stmt_break(void)
{
	return "break";
}

void fold_stmt_break_continue(stmt *t, stmt *parent)
{
	if(!parent)
		die_at(&t->where, "%s outside a flow-control statement", t->f_str());
}

void fold_stmt_break(stmt *t)
{
	fold_stmt_break_continue(t, t->parent);
}

void gen_stmt_break(const stmt *s, out_ctx *octx)
{
	gen_scope_leave(s->symtab, s->parent->symtab, octx);

	out_ctrl_transfer(octx, s->parent->blk_break, NULL, NULL, 0);
}

void dump_stmt_break(const stmt *s, dump *ctx)
{
	dump_desc_stmt(ctx, "break", s);
}

void style_stmt_break(const stmt *s, out_ctx *octx)
{
	stylef("break;");
	gen_stmt(s->lhs, octx);
}

static int break_passable(stmt *s, int break_means_passable)
{
	(void)s;
	return break_means_passable;
}

void init_stmt_break(stmt *s)
{
	s->f_passable = break_passable;
}
