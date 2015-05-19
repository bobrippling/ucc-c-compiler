#include <stdlib.h>

#include "../../util/dynarray.h"

#include "ops.h"
#include "stmt_goto.h"
#include "../out/lbl.h"
#include "../label.h"

const char *str_stmt_goto()
{
	return "goto";
}

void fold_stmt_goto(stmt *s)
{
	if(!symtab_func(s->symtab))
		die_at(&s->where, "goto outside of a function");

	if(s->expr){
		FOLD_EXPR(s->expr, s->symtab);
	}else{
		(s->bits.lbl.label =
		 symtab_label_find_or_new(
			 s->symtab, s->bits.lbl.spel, &s->where))
			->uses++;

		dynarray_add(&s->bits.lbl.label->jumpers, s);
	}
}

void gen_stmt_goto(const stmt *s, out_ctx *octx)
{
	if(s->expr){
		/* no idea whether we're leaving scope - don't do anything */

		const out_val *target = gen_expr(s->expr, octx);

		out_ctrl_transfer_exp(octx, target);

	}else{
		out_blk *target;

		gen_scope_leave(s->symtab, s->bits.lbl.label->scope, octx);

		target = label_getblk(s->bits.lbl.label, octx);

		out_ctrl_transfer(octx, target, NULL, NULL);
	}
}

void dump_stmt_goto(const stmt *s, dump *ctx)
{
	dump_desc_stmt(ctx, s->expr ? "computed-goto" : "goto", s);

	dump_inc(ctx);
	if(s->expr){
		dump_expr(s->expr, ctx);
	}else{
		dump_printf(ctx, "label %s\n", s->bits.lbl.spel);
	}
	dump_dec(ctx);
}

void style_stmt_goto(const stmt *s, out_ctx *octx)
{
	stylef("goto ");

	if(s->expr){
		stylef("*");
		IGNORE_PRINTGEN(gen_expr(s->expr, octx));
	}else{
		stylef("%s", s->bits.lbl.spel);
	}

	stylef(";");
}

void init_stmt_goto(stmt *s)
{
	s->f_passable = fold_passable_no;
}
