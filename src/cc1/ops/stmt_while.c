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
	fold_test_expr(s->expr, s->f_str());

	OPT_CHECK(s->expr, "constant expression in if/while");

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
	if(const_expr_is_const(s->expr) && const_expr_value(s->expr))
		return fold_code_escapable(s); /* while(1) */

	return fold_passable(s->lhs);
}

void mutate_stmt_while(stmt *s)
{
	s->f_passable = while_passable;
}
