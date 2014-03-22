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
	fold_stmt_and_add_to_curswitch(s, &s->bits.case_lbl);
}

void gen_stmt_default(stmt *s, out_ctx *octx)
{
	out_blk *default_blk = out_blk_new("default");
	out_current_blk(octx, default_blk);
	gen_stmt(s->lhs, octx);
}

void style_stmt_default(stmt *s, out_ctx *octx)
{
	stylef("\ndefault: ");
	gen_stmt(s->lhs, octx);
}

void init_stmt_default(stmt *s)
{
	s->f_passable = label_passable;
}
