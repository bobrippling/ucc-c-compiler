#include <stddef.h>
#include <assert.h>

#include "../type.h"
#include "../num.h"

#include "val.h"
#include "ctx.h"

#include "blk.h"

void blk_terminate_condjmp(
		out_ctx *octx,
		char *condinsn, out_blk *condto,
		char *uncondjmp, out_blk *uncondto)
{
	out_blk *current = octx->current_blk;

	assert(current->next.type == BLK_NEXT_NONE);

	current->next.type = BLK_NEXT_COND;

	current->next.bits.cond.if_0.insn = condinsn;
	current->next.bits.cond.if_0.blk = condto;
	current->next.bits.cond.if_1.insn = uncondjmp;
	current->next.bits.cond.if_1.blk = uncondto;
}
