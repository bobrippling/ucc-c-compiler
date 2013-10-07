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
	if(!curdecl_func)
		die_at(&s->where, "goto outside of a function");

	if(s->expr->expr_computed_goto){
		FOLD_EXPR(s->expr, s->symtab);
	}else{
		char *ident;
		label *lbl;

		if(!expr_kind(s->expr, identifier))
			die_at(&s->expr->where, "not a label identifier");

		ident = s->expr->bits.ident.spel;

		lbl = symtab_label_find(s->symtab, ident);
		if(!lbl){
			/* forward decl */
			lbl = label_new(&s->where, ident, 0);
			symtab_label_add(s->symtab, lbl);
		}
		s->bits.goto_target = lbl;
	}
}

void gen_stmt_goto(stmt *s)
{
	if(s->expr->expr_computed_goto)
		gen_expr(s->expr);
	else
		out_push_lbl(s->expr->bits.ident.spel, 0);

	out_jmp();
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

void init_stmt_goto(stmt *s)
{
	s->f_passable = fold_passable_no;
}
