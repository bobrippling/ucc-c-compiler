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
	char *begin = asm_label_flow("do_start");

	asm_label(begin);
	gen_stmt(s->lhs);

	asm_label(s->lbl_continue);
	gen_expr(s->expr, s->symtab);

	asm_temp(1, "pop rax");
	asm_temp(1, "test rax, rax");
	asm_temp(1, "jnz %s", begin);

	asm_label(s->lbl_break);

	free(begin);
}

void mutate_stmt_do(stmt *s)
{
	s->f_passable = while_passable;
}
