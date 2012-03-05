#include <stdlib.h>

#include "ops.h"
#include "stmt_do.h"

const char *str_stmt_do()
{
	return "do";
}

void fold_stmt_do(stmt *s)
{
	fold_stmt_while(s);
}

void gen_stmt_do(stmt *s)
{
	char *lbl_start;

	lbl_start = asm_label_code("do");

	asm_label(lbl_start);
	gen_stmt(s->lhs);

	gen_expr(s->expr, s->symtab);
	asm_pop(ASM_REG_A);
	ASM_TEST(s->expr->tree_type, ASM_REG_A);
	asm_jmp_if_zero(1, lbl_start);

	free(lbl_start);
	asm_label(s->lblfin);
}
