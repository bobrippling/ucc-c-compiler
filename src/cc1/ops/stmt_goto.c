#include <stdlib.h>

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

		fold_check_scope_entry(&s->where, s->symtab, s->bits.lbl.label->scope);
	}
}

void gen_stmt_goto(stmt *s, out_ctx *octx)
{
	if(s->expr){
		/* no idea whether we're leaving scope - don't do anything */

		const out_val *target = gen_expr(s->expr, octx);

		out_ctrl_transfer_exp(octx, target);

	}else{
		gen_scope_leave(s->symtab, s->bits.lbl.label->scope, octx);

		label_makeblk(s->bits.lbl.label, octx);

		out_ctrl_transfer(octx, s->bits.lbl.label->bblock, NULL, NULL);
	}
}

void style_stmt_goto(stmt *s, out_ctx *octx)
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
