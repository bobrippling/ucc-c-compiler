#include <stdlib.h>

#include "ops.h"
#include "stmt_while.h"

const char *str_stmt_while()
{
	return "while";
}

void fold_stmt_while(stmt *s)
{
	stmt *oldflowstat = curstat_flow;
	curstat_flow = s;

	s->lblfin = asm_label_flowfin();

	fold_expr(s->expr, s->symtab);
	fold_test_expr(s->expr, s->f_str());

	OPT_CHECK(s->expr, "constant expression in if/while");

	fold_stmt(s->lhs);

	curstat_flow = oldflowstat;
}

void gen_stmt_while(stmt *s)
{
	char *lbl_start;

	lbl_start = asm_label_code("while");

	asm_label(lbl_start);
	gen_expr(s->expr, s->symtab);
	asm_temp(1, "pop rax");
	asm_temp(1, "test rax, rax");
	asm_temp(1, "jz %s", s->lblfin);
	gen_stmt(s->lhs);
	asm_temp(1, "jmp %s", lbl_start);
	asm_label(s->lblfin);

	free(lbl_start);
}
