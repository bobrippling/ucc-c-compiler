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
			DIE_AT(&s->expr->where, "not a label identifier");

		save = *(psp = &s->expr->bits.ident.spel);
		/* else let the assembler check for link errors */
		*psp = out_label_goto(save);
		free(save);
	}
}

void gen_stmt_goto(stmt *s)
{
	if(s->expr->expr_computed_goto)
		gen_expr(s->expr, s->symtab);
	else
		out_push_lbl(s->expr->bits.ident.spel, 0);

	out_jmp();
}

void mutate_stmt_goto(stmt *s)
{
	s->f_passable = fold_passable_no;
}
