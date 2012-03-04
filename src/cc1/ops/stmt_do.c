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
	asm_temp(1, "pop rax");
	asm_temp(1, "test rax, rax");
	asm_temp(1, "jnz %s", lbl_start);

	free(lbl_start);
	asm_label(s->lblfin);
}
