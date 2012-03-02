#include <stdlib.h>

#include "ops.h"
#include "stat_do.h"

const char *str_stat_do()
{
	return "do";
}

void fold_stat_do(stat *s)
{
	fold_stat_while(s);
}

void gen_stat_do(stat *s)
{
	char *lbl_start;

	lbl_start = asm_label_code("do");

	asm_label(lbl_start);
	gen_stat(s->lhs);

	gen_expr(s->expr, s->symtab);
	asm_temp(1, "pop rax");
	asm_temp(1, "test rax, rax");
	asm_temp(1, "jnz %s", lbl_start);

	free(lbl_start);
	asm_label(s->lblfin);
}
