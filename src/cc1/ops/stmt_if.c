#include <stdlib.h>

#include "ops.h"
#include "stmt_if.h"

const char *str_stmt_if()
{
	return "if";
}

void fold_stmt_if(stmt *s)
{
	fold_expr(s->expr, s->symtab);
	fold_test_expr(s->expr, s->f_str());

	OPT_CHECK(s->expr, "constant expression in if/while");

	fold_stmt(s->lhs);
	if(s->rhs)
		fold_stmt(s->rhs);
}

void gen_stmt_if(stmt *s)
{
	char *lbl_else = asm_label_code("else");
	char *lbl_fi   = asm_label_code("fi");

	gen_expr(s->expr, s->symtab);

	asm_pop(ASM_REG_A);
	ASM_TEST(s->expr->tree_type, ASM_REG_A);
	asm_jmp_if_zero(0, lbl_else);
	gen_stmt(s->lhs);
	asm_jmp(lbl_fi);
	asm_label(lbl_else);
	if(s->rhs)
		gen_stmt(s->rhs);
	asm_label(lbl_fi);

	free(lbl_else);
	free(lbl_fi);
}
