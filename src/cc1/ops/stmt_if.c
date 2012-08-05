#include <stdlib.h>

#include "ops.h"
#include "stmt_if.h"
#include "stmt_for.h"
#include "../out/lbl.h"

const char *str_stmt_if()
{
	return "if";
}

symtable *fold_stmt_test_init_expr(stmt *s, const char *which)
{
	if(s->flow){
		/* if(char *x = ...) */
		expr *dinit;

		dinit = fold_for_if_init_decls(s);

		if(!dinit)
			DIE_AT(&s->where, "no initialiser to test in %s", which);

		UCC_ASSERT(!s->expr, "%s-expr in c99_ucc %s-init mode", which, which);

		s->expr = dinit;
		return s->flow->for_init_symtab;
	}

	return s->symtab;
}

void fold_stmt_if(stmt *s)
{
	symtable *test_symtab = fold_stmt_test_init_expr(s, "if");

	fold_expr(s->expr, test_symtab);

	fold_need_expr(s->expr, s->f_str(), 1);
	OPT_CHECK(s->expr, "constant expression in if");

	fold_stmt(s->lhs);
	if(s->rhs)
		fold_stmt(s->rhs);
}

void gen_stmt_if(stmt *s)
{
	char *lbl_else = out_label_code("else");
	char *lbl_fi   = out_label_code("fi");

	gen_expr(s->expr, s->symtab);

	out_jz(lbl_else);

	gen_stmt(s->lhs);
	out_push_lbl(lbl_fi);
	out_jmp();

	out_label(lbl_else);
	if(s->rhs)
		gen_stmt(s->rhs);
	out_label(lbl_fi);

	free(lbl_else);
	free(lbl_fi);
}

static int if_passable(stmt *s)
{
	return fold_passable(s->lhs) || (s->rhs ? fold_passable(s->rhs) : 0);
}

void mutate_stmt_if(stmt *s)
{
	s->f_passable = if_passable;
}
