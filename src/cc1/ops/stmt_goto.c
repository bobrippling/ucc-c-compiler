#include <stdlib.h>

#include "../../util/dynarray.h"

#include "ops.h"
#include "stmt_goto.h"
#include "../out/lbl.h"
#include "../label.h"

const char *str_stmt_goto(void)
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

		out_ctrl_transfer(octx, target, NULL, NULL, 0);
	}
}

void dump_stmt_goto(const stmt *s, dump *ctx)
{
	if(s->expr){
		dump_desc_stmt(ctx, "computed-goto", s);

		dump_inc(ctx);
		dump_expr(s->expr, ctx);
		dump_dec(ctx);
	}else{
		dump_desc_stmt_newline(ctx, "goto", s, 0);
		dump_printf_indent(ctx, 0, " %s\n", s->bits.lbl.spel);
	}
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

static int goto_passable(stmt *s, int break_means_passable)
{
	struct label *lbl;
	int passable;

	if(s->expr){
		/* could jump anywhere - assume it's not passable */
		return 0;
	}

	lbl = s->bits.lbl.label;
	if(!lbl)
		return 1; /* safety */

	if(!lbl->next_stmt)
		return 1;

	if(lbl->doing_passable_check){
		/* infinite loop */
		return 0;
	}

	/* this isn't perfect as it doesn't detect things like:
	 * x:
	 * printf("hi\n");
	 * goto x;
	 *
	 * but better to pessimise and warn/insert-main-return-0
	 * than to miss.
	 */

	lbl->doing_passable_check = 1;
	passable = fold_passable(lbl->next_stmt, break_means_passable);
	lbl->doing_passable_check = 0;
	return passable;
}

void init_stmt_goto(stmt *s)
{
	s->f_passable = goto_passable;
}
