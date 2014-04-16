#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <assert.h>

#include "../type.h"
#include "../num.h"

#include "val.h"
#include "ctx.h"

#include "blk.h"
#include "impl_jmp.h"

#include "asm.h" /* cc_out */

static void flush_block(out_blk *blk, FILE *f)
{
	char **i;

	if(blk->flushed)
		return;
	blk->flushed = 1;

	fprintf(f, "%s: # %s\n", blk->lbl, blk->desc);

	for(i = blk->insns; i && *i; i++)
		fprintf(f, "%s", *i);

	switch(blk->next.type){
		case BLK_NEXT_NONE:
		case BLK_NEXT_EXPR:
			break;

		case BLK_NEXT_BLOCK:
			impl_jmp(f, blk->next.bits.blk->lbl);

			flush_block(blk->next.bits.blk, f);
			break;

		case BLK_NEXT_COND:
			/* place true jumps first */
			fprintf(f, "\t%s\n", blk->next.bits.cond.insn);
			/* put the if_1 block after so we don't need a jump */
			flush_block(blk->next.bits.cond.if_1_blk, f);
			/* TODO: jump to next block */
			flush_block(blk->next.bits.cond.if_0_blk, f);
			/* TODO: jump to next block */
			/* TODO: emit next block */
			break;

		case BLK_NEXT_BLOCK_UPSCOPE:
			fprintf(f, "\t# jump to %s, upscope\n", blk->next.bits.blk->lbl);
			break;
	}
}

void blk_flushall(out_ctx *octx)
{
	flush_block(octx->first_blk, cc_out[SECTION_TEXT]);
}

void blk_terminate_condjmp(
		out_ctx *octx, char *condinsn,
		out_blk *condto, out_blk *uncondto)
{
	out_blk *current = octx->current_blk;

	assert(current->next.type == BLK_NEXT_NONE);

	current->next.type = BLK_NEXT_COND;

	current->next.bits.cond.insn = condinsn;
	current->next.bits.cond.if_0_blk = condto;
	current->next.bits.cond.if_1_blk = uncondto;

	octx->current_blk = NULL;
}
