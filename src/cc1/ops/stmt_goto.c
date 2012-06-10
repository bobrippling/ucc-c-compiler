#include <stdlib.h>

#include "ops.h"
#include "stmt_goto.h"

const char *str_stmt_goto()
{
	return "goto";
}

void fold_stmt_goto(stmt *s)
{
	char *save = s->expr->spel;

	if(!expr_kind(s->expr, identifier))
		DIE_AT(&s->expr->where, "not a label identifier");

	/* else let the assembler check for link errors */
	s->expr->spel = asm_label_goto(s->expr->spel);
	free(save);
}

void gen_stmt_goto(stmt *s)
{
	asm_temp(1, "jmp %s", s->expr->spel);
}
