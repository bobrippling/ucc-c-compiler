#include "ops.h"
#include "stat_label.h"
const char *str_stat_label()
{
	return "label";
}

void fold_stat_label(stat *s)
{
	fold_stat_goto(s);
	fold_stat(s->lhs); /* compound */
}

void gen_stat_label(stat *s)
{
	asm_label(s->expr->spel);
	gen_stat(s->lhs); /* the code-part of the compound statement */
}
