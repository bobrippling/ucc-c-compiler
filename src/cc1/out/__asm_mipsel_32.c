#include <stdio.h>
#include <stdarg.h>

#include "../../util/util.h"

#include "../data_structs.h"
#include "../expr.h"
#include "../tree.h"
#include "../stmt.h"
#include "__asm.h"

void out_constraint_check(where *w, const char *constraint, int output)
{
	ICE("no inline asm() support for MIPS 32");
}

void out_asm_inline(asm_args *cmd, const where *const err_w)
{
	ICE("can't do inline asm() for MIPS 32 yet");
}
