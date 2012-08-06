#include <stdlib.h>

#include "ops.h"
#include "stmt_while.h"
#include "stmt_if.h"
#include "stmt_for.h"
#include "../out/lbl.h"

const char *str_stmt_while()
{
	return "while";
}

void fold_stmt_while(stmt *s)
{
	symtable *test_symtab;

	test_symtab = fold_stmt_test_init_expr(s, "which");

	s->lbl_break    = out_label_flow("while_break");
	s->lbl_continue = out_label_flow("while_cont");

	fold_expr(s->expr, test_symtab);
	fold_need_expr(s->expr, s->f_str(), 1);

	OPT_CHECK(s->expr, "constant expression in while");

	fold_stmt(s->lhs);
}

void gen_stmt_while(stmt *s)
{
	out_label(s->lbl_continue);

	gen_expr(s->expr, s->symtab);

	out_jz(s->lbl_break);

	gen_stmt(s->lhs);

	out_push_lbl(s->lbl_continue, 0);
	out_jmp();

	out_label(s->lbl_break);
}

int while_passable(stmt *s)
{
	intval val;
	enum constyness k;

	const_fold(s->expr, &val, &k);

	if(k == CONST_WITH_VAL && val.val)
		return fold_code_escapable(s); /* while(1) */

	return 1; /* fold_passable(s->lhs) - doesn't depend on this */
}

void mutate_stmt_while(stmt *s)
{
	s->f_passable = while_passable;
}
