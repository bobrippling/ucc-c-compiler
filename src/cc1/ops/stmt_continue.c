#include "ops.h"
#include "stmt_continue.h"
#include "stmt_break.h"

const char *str_stmt_continue()
{
	return "continue";
}

void fold_stmt_continue(stmt *t)
{
	fold_stmt_break_continue(t, t->parent);
}

void gen_stmt_continue(const stmt *s, out_ctx *octx)
{
	gen_scope_leave(s->symtab, s->parent->symtab, octx);
	out_ctrl_transfer(octx, s->parent->blk_continue, NULL, NULL);
}

void gen_ir_stmt_continue(const stmt *s, irctx *ctx)
{
	gen_ir_scope_leave(s->symtab, s->parent->symtab, ctx);

	printf("\tjmp $%u\n", s->parent->blk_continue_ir);
}

void dump_stmt_continue(const stmt *s, dump *ctx)
{
	dump_desc_stmt(ctx, "continue", s);
}

void style_stmt_continue(const stmt *s, out_ctx *octx)
{
	stylef("continue;");
	gen_stmt(s->lhs, octx);
}

void init_stmt_continue(stmt *s)
{
	s->f_passable = fold_passable_no;
}
