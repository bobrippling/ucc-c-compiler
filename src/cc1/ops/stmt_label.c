#include "ops.h"
#include "stmt_label.h"
#include "../label.h"
#include "../out/lbl.h"

const char *str_stmt_label()
{
	return "label";
}

void fold_stmt_label(stmt *s)
{
	label *l = symtab_label_find_or_new(
			s->symtab, s->bits.lbl.spel, &s->where);

	/* update its where */
	l->pw = &s->where;
	/* update its scope */
	l->scope = s->symtab;

	if(l->complete)
		die_at(&s->where, "duplicate label '%s'", s->bits.lbl.spel);
	else
		l->complete = 1;

	s->bits.lbl.label = l;

	l->unused = s->bits.lbl.unused;

	fold_stmt(s->lhs); /* compound */
}

void gen_stmt_label(const stmt *s, out_ctx *octx)
{
	out_blk *thisblk = label_getblk(s->bits.lbl.label, octx);

	/* explicit fall through */
	out_ctrl_transfer_make_current(octx, thisblk);
	gen_stmt(s->lhs, octx); /* the code-part of the compound statement */
}

void dump_stmt_label(const stmt *s, dump *ctx)
{
	dump_desc_stmt(ctx, "label", s);

	dump_inc(ctx);
	dump_printf(ctx, "label %s\n", s->bits.lbl.spel);
	dump_dec(ctx);
}

void style_stmt_label(const stmt *s, out_ctx *octx)
{
	stylef("\n%s: ", s->bits.lbl.spel);
	gen_stmt(s->lhs, octx);
}

int label_passable(stmt *s)
{
	return fold_passable(s->lhs);
}

void init_stmt_label(stmt *s)
{
	s->f_passable = label_passable;
}
