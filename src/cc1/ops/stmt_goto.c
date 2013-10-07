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

	if(s->expr){
		FOLD_EXPR(s->expr, s->symtab);
	}else{
		char *ident = s->bits.lbl.spel;
		label *lbl = symtab_label_find(s->symtab, ident);

		if(!lbl){
			/* forward decl */
			lbl = label_new(&s->where, ident, 0);
			symtab_label_add(s->symtab, lbl);
		}
		s->bits.lbl.label = lbl;
	}
}

void gen_stmt_goto(stmt *s)
{
	if(s->expr)
		gen_expr(s->expr);
	else
		out_push_lbl(s->bits.lbl.spel, 0);

	out_jmp();
}

void style_stmt_goto(stmt *s)
{
	stylef("goto ");

	if(s->expr){
		stylef("*");
		gen_expr(s->expr);
	}else{
		stylef("%s", s->bits.lbl.spel);
	}

	stylef(";");
}

void init_stmt_goto(stmt *s)
{
	s->f_passable = fold_passable_no;
}
