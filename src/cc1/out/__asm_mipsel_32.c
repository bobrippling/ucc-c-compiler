#include <stdio.h>
#include <stdarg.h>

#include "../../util/util.h"

#include "../expr.h"
#include "../stmt.h"

void out_constraint_check(where *w, const char *constraint, int output)
{
	ICE("no inline asm() support for MIPS 32");
}

void out_asm_inline(asm_args *cmd, const where *const err_w)
{
	ICE("can't do inline asm() for MIPS 32 yet");
}
