#include "ops.h"
#include "stmt_noop.h"

const char *str_stmt_noop()
{
	return "noop";
}

void fold_stmt_noop(stmt *s)
{
	(void)s;
}

void gen_stmt_noop(stmt *s)
{
	(void)s;
	asm_temp(1, "; noop");
}
