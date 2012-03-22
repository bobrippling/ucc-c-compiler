#include <stdlib.h>

#include "ops.h"
#include "stmt_while.h"

const char *str_stmt_while()
{
	return "while";
}

void fold_stmt_while(stmt *s)
{
	stmt *oldflowstat = curstmt_flow;
	curstmt_flow = s;

	s->lbl_break    = asm_label_flow("while_break");
	s->lbl_continue = asm_label_flow("while_cont");

	fold_expr(s->expr, s->symtab);
	fold_test_expr(s->expr, s->f_str());

	OPT_CHECK(s->expr, "constant expression in if/while");

	fold_stmt(s->lhs);

	curstmt_flow = oldflowstat;
}

void gen_stmt_while(stmt *s)
{
	asm_label(s->lbl_continue);

	gen_expr(s->expr, s->symtab);

	asm_pop( s->expr->tree_type, ASM_REG_A);
	ASM_TEST(s->expr->tree_type, ASM_REG_A);
	asm_jmp_if_zero(0, s->lbl_break);

	gen_stmt(s->lhs);

	asm_jmp(s->lbl_continue);
	asm_label(s->lbl_break);
}
