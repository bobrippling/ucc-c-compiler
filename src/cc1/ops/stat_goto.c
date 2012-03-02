#include <stdlib.h>

#include "ops.h"
#include "stat_goto.h"

const char *str_stat_goto()
{
	return "goto";
}

void fold_stat_goto(stat *s)
{
	char *save = s->expr->spel;

	if(!expr_kind(s->expr, identifier))
		die_at(&s->expr->where, "not a label identifier");

	/* else let the assembler check for link errors */
	s->expr->spel = asm_label_goto(s->expr->spel);
	free(save);
}

void gen_stat_goto(stat *s)
{
	asm_temp(1, "jmp %s", s->expr->spel);
}
