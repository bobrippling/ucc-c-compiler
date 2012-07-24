#include <stdlib.h>

#include "ops.h"
#include "stmt_while.h"
#include "stmt_if.h"
#include "stmt_for.h"

const char *str_stmt_while()
{
	return "while";
}

void fold_stmt_while(stmt *s)
{
	symtable *test_symtab;

	test_symtab = fold_stmt_test_init_expr(s, "which");

	s->lbl_break    = asm_label_flow("while_break");
	s->lbl_continue = asm_label_flow("while_cont");

	fold_expr(s->expr, test_symtab);
	fold_need_expr(s->expr, s->f_str(), 1);

	OPT_CHECK(s->expr, "constant expression in while");

	fold_stmt(s->lhs);
}

void gen_stmt_while(stmt *s)
{
	asm_label(s->lbl_continue);
	gen_expr(s->expr, s->symtab); /* TODO: optimise */
	asm_temp(1, "pop rax");
	asm_temp(1, "test rax, rax");
	asm_temp(1, "jz %s", s->lbl_break);
	gen_stmt(s->lhs);
	asm_temp(1, "jmp %s", s->lbl_continue);
	asm_label(s->lbl_break);
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
