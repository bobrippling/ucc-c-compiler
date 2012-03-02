#include "ops.h"
#include "stat_noop.h"

const char *str_stat_noop()
{
	return "noop";
}

void fold_stat_noop(stat *s)
{
	(void)s;
}

void gen_stat_noop(stat *s)
{
	(void)s;
	asm_temp(1, "; noop");
}
