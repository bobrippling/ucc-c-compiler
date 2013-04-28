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
		expr *dinit = fold_for_if_init_decls(s);

		if(!dinit)
			DIE_AT(&s->where, "no initialiser to test in %s", which);

		/* allow s->expr: if(int i = f(), i > 2) ... */
		if(s->expr){
			/* treat as if it's: if(i = f(), i > 2) */
			s->expr = expr_new_comma2(dinit, s->expr);
		}else{
			/* treat it as: if(i = f()) */
			s->expr = dinit;
		}

		return s->flow->for_init_symtab;
	}

	fold_symtab_scope(s->symtab, NULL);

	return s->symtab;
}

void fold_stmt_if(stmt *s)
{
	symtable *test_symtab = fold_stmt_test_init_expr(s, "if");

	FOLD_EXPR(s->expr, test_symtab);

	fold_need_expr(s->expr, s->f_str(), 1);

	fold_stmt(s->lhs);
	if(s->rhs)
		fold_stmt(s->rhs);
}

void gen_stmt_if(stmt *s)
{
	char *lbl_else = out_label_code("else");
	char *lbl_fi   = out_label_code("fi");

	gen_expr(s->expr, s->symtab);

	out_jfalse(lbl_else);

	gen_stmt(s->lhs);
	out_push_lbl(lbl_fi, 0);
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
	return (s->rhs ? fold_passable(s->rhs) : 1) || fold_passable(s->lhs);
}

void mutate_stmt_if(stmt *s)
{
	s->f_passable = if_passable;
}
