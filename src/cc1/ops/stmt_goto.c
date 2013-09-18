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
		*psp = out_label_goto(b_from, curdecl_func->spel, save);
		free(save);
	}
}

void gen_stmt_goto(stmt *s)
{
	if(s->expr->expr_computed_goto)
		gen_expr(s->expr);
	else
		out_push_lbl(b_from, s->expr->bits.ident.spel, 0);

	out_jmp(b_from);
}

void style_stmt_goto(stmt *s)
{
	stylef("goto ");

	if(s->expr->expr_computed_goto){
		stylef("*");
		gen_expr(s->expr);
	}else{
		stylef("%s", s->expr->bits.ident.spel);
	}

	stylef(";");
}

void mutate_stmt_goto(stmt *s)
{
	s->f_passable = fold_passable_no;
}
