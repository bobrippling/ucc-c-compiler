#include <stdlib.h>

#include "ops.h"
#include "stmt_goto.h"
#include "../out/lbl.h"

const char *str_stmt_goto()
{
	return "goto";
}

void fold_stmt_goto(stmt *s)
{
	if(s->expr->expr_computed_goto){
		FOLD_EXPR(s->expr, s->symtab);
	}else{
		char *save, **psp;

		if(!expr_kind(s->expr, identifier))
			die_at(&s->expr->where, "not a label identifier");

		save = *(psp = &s->expr->bits.ident.spel);
		/* else let the assembler check for link errors */
		if(!curdecl_func)
			die_at(&s->where, "goto outside of a function");
		*psp = out_label_goto(curdecl_func->spel, save);
		free(save);
	}
}

basic_blk *gen_stmt_goto(stmt *s, basic_blk *bb)
{
	if(s->expr->expr_computed_goto)
		bb = gen_expr(s->expr, bb);
	else
		out_push_lbl(bb, s->expr->bits.ident.spel, 0);

	out_jmp(bb);
	return bb;
}

basic_blk *style_stmt_goto(stmt *s, basic_blk *bb)
{
	stylef("goto ");

	if(s->expr->expr_computed_goto){
		stylef("*");
		bb = gen_expr(s->expr, bb);
	}else{
		stylef("%s", s->expr->bits.ident.spel);
	}

	stylef(";");

	return bb;
}

void mutate_stmt_goto(stmt *s)
{
	s->f_passable = fold_passable_no;
}
