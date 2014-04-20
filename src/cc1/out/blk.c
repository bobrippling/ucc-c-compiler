#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <assert.h>

#include "../../util/alloc.h"

#include "../type.h"
#include "../num.h"

#include "val.h"
#include "ctx.h"
#include "out.h" /* out_blk_new() */
#include "lbl.h"

#include "blk.h"
#include "impl_jmp.h"

#include "asm.h" /* cc_out */

#ifdef BLK_DEBUG
struct A
{
	out_blk *blk;
	FILE *f;
};

static void done(struct A *b)
{
	fprintf(b->f, "EXIT %s\n", b->blk->lbl);
}

static void start(struct A *b)
{
	fprintf(b->f, "ENTR %s\n", b->blk->lbl);
}
#endif

static int should_flush(out_blk *blk, int mutate)
{
	if(BLK_IS_MERGE(blk)){
		int is_loopback = blk->preds[0] == blk
		               || blk->preds[1] == blk;

		if(is_loopback){
			int flush_now = !blk->flushed;

			if(mutate && flush_now)
				blk->flushed = 1;

			return flush_now;
		}

		assert(blk->flushed < 2);

		if(mutate)
			blk->flushed++;

		return blk->flushed == 2;
	}else{
		assert(!blk->flushed);
		if(mutate)
			blk->flushed = 1;
		return 1;
	}
}

static void blk_jmpnext(FILE *f, out_blk *to)
{
	if(should_flush(to, 0)){
		fprintf(f, "\t# implicit jump to next line\n");
		return; /* don't bother, fall through to it */
	}
	impl_jmp(f, to->lbl);
}

static void flush_block(out_blk *blk, FILE *f)
{
#ifdef BLK_DEBUG
	struct A x __attribute((cleanup(done))) = {
		.blk = blk, .f = f
	};
#endif
	char **i;

	if(!should_flush(blk, 1))
		return;

	fprintf(f, "%s: # %s\n", blk->lbl, blk->desc);

	for(i = blk->insns; i && *i; i++)
		fprintf(f, "%s", *i);

	switch(blk->type){
		case BLK_UNINIT:
			assert(0 && "uninitialised block type");
		case BLK_TERMINAL:
		case BLK_NEXT_EXPR:
			break;

		case BLK_NEXT_BLOCK:
			blk_jmpnext(f, blk->bits.next);
			flush_block(blk->bits.next, f);
			break;

		case BLK_COND:
			/* place true jumps first */
			fprintf(f, "\t%s\n", blk->bits.cond.insn);
			/* put the if_1 block after so we don't need a jump */
			flush_block(blk->bits.cond.if_1_blk, f);
			flush_block(blk->bits.cond.if_0_blk, f);
			/* flushing if_1 and if_0 flushes their merge block and so on */
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

	assert(current->type == BLK_UNINIT && "overwriting .next?");

	current->type = BLK_COND;

	current->bits.cond.insn = condinsn;
	current->bits.cond.if_0_blk = condto;
	current->bits.cond.if_1_blk = uncondto;

	octx->current_blk = NULL;
}

out_blk *out_blk_new_lbl(out_ctx *octx, const char *lbl)
{
	out_blk *blk = umalloc(sizeof *blk);
	blk->desc = lbl;
	blk->lbl = ustrdup(lbl);
	return blk;
}

out_blk *out_blk_new(out_ctx *octx, const char *desc)
{
	out_blk *blk = umalloc(sizeof *blk);
	blk->desc = desc;
	blk->lbl = out_label_bblock(octx->nblks++);
	return blk;
}
