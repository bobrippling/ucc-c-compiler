#include <stdio.h>
#include <stddef.h>
#include <assert.h>

#include "../type.h"
#include "../num.h"

#include "val.h"
#include "ctx.h"

#include "blk.h"

#include "asm.h" /* cc_out */

static void flush_block(out_blk *blk, FILE *f)
{
	char **i;

	if(blk->flushed)
		return;
	blk->flushed = 1;

	for(i = blk->insns; i && *i; i++)
		fprintf(f, "%s", *i);

	switch(blk->next.type){
		case BLK_NEXT_NONE:
		case BLK_NEXT_EXPR:
			break;

		case BLK_NEXT_BLOCK:
			flush_block(blk->next.bits.blk, f);
			break;

		case BLK_NEXT_COND:
			/* place true jumps first */
			flush_block(blk->next.bits.cond.if_1.blk, f);
			flush_block(blk->next.bits.cond.if_0.blk, f);
			break;
	}
}

void blk_flushall(out_ctx *octx)
{
	flush_block(octx->first_blk, cc_out[SECTION_TEXT]);
}

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

	octx->current_blk = NULL;
}
