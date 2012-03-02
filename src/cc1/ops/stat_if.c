#include <stdlib.h>

#include "ops.h"
#include "stat_if.h"

const char *str_stat_if()
{
	return "if";
}

void fold_stat_if(stat *s)
{
	fold_expr(s->expr, s->symtab);
	fold_test_expr(s->expr, s->f_str());

	OPT_CHECK(s->expr, "constant expression in if/while");

	fold_stat(s->lhs);
	if(s->rhs)
		fold_stat(s->rhs);
}

void gen_stat_if(stat *s)
{
	char *lbl_else = asm_label_code("else");
	char *lbl_fi   = asm_label_code("fi");

	gen_expr(s->expr, s->symtab);

	asm_temp(1, "pop rax");
	asm_temp(1, "test rax, rax");
	asm_temp(1, "jz %s", lbl_else);
	gen_stat(s->lhs);
	asm_temp(1, "jmp %s", lbl_fi);
	asm_label(lbl_else);
	if(s->rhs)
		gen_stat(s->rhs);
	asm_label(lbl_fi);

	free(lbl_else);
	free(lbl_fi);
}
