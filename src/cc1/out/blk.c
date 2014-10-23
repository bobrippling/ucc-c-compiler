#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include "../../util/alloc.h"
#include "../../util/dynarray.h"

#include "../type.h"
#include "../num.h"
#include "../str.h" /* str_add_escape */

#include "val.h"
#include "ctx.h"
#include "out.h" /* out_blk_new() */
#include "lbl.h"
#include "dbg.h"

#include "blk.h"
#include "impl_jmp.h"

#include "asm.h" /* cc_out */
#include "../cc1.h" /* fopt_mode */

#define JMP_THREAD_LIM 10

struct flush_state
{
	FILE *f;

	/* for jump threading - the block we jump to if not immediately flushing */
	out_blk *jmpto;
};

static void blk_jmpnext(out_blk *to, struct flush_state *st)
{
	assert(!st->jmpto);
	st->jmpto = to;
}

static void blk_jmpthread(struct flush_state *st)
{
	int lim = 0;
	out_blk *to = st->jmpto;

	while(!to->insns && to->type == BLK_NEXT_BLOCK && lim < JMP_THREAD_LIM){
			to = to->bits.next;
			lim++; /* prevent circulars */
	}

	if(lim && fopt_mode & FOPT_VERBOSE_ASM)
		fprintf(st->f, "\t# jump threaded through %d blocks\n", lim);

	impl_jmp(st->f, to->lbl);
}

static void blk_codegen(out_blk *blk, struct flush_state *st)
{
	char **i;

	/* before any instructions, if we have a pending jmpto and
	 * we aren't the target branch, we need to cut off the last
	 * block with a jump to said jmpto */
	if(st->jmpto){
		if(st->jmpto != blk)
			blk_jmpthread(st);
		else if(fopt_mode & FOPT_VERBOSE_ASM)
			fprintf(st->f, "\t# implicit jump to next line\n");
		st->jmpto = NULL;
	}

	fprintf(st->f, "%s: # %s\n", blk->lbl, blk->desc);

	for(i = blk->insns; i && *i; i++)
		fprintf(st->f, "%s", *i);
}

static void bfs_block(out_blk *blk, struct flush_state *st)
{
	if(blk->flush_in_prog)
		return;
	blk->flush_in_prog = 1;

	if(blk->merge_preds){
		out_blk **i;

		for(i = blk->merge_preds; *i; i++){
			bfs_block(*i, st);
		}
	}

	switch(blk->type){
		case BLK_UNINIT:
			assert(0 && "uninitialised block type");

		case BLK_TERMINAL:
		case BLK_NEXT_EXPR:
		case BLK_NEXT_BLOCK:
			blk_codegen(blk, st);

			if(blk->type == BLK_NEXT_BLOCK){
				blk_jmpnext(blk->bits.next, st);
				bfs_block(blk->bits.next, st);
			}
			break;

		case BLK_COND:
			blk_codegen(blk, st);
			fprintf(st->f, "\t%s\n", blk->bits.cond.insn);

			/* we always jump to the true block if the conditional failed */
			blk_jmpnext(blk->bits.cond.if_1_blk, st);

			/* if it's unlikely, we want the false block already in the pipeline */
			if(blk->bits.cond.unlikely){
				bfs_block(blk->bits.cond.if_0_blk, st);
				bfs_block(blk->bits.cond.if_1_blk, st);
			}else{
				bfs_block(blk->bits.cond.if_1_blk, st);
				bfs_block(blk->bits.cond.if_0_blk, st);
			}
			break;
	}
}

void blk_flushall(out_ctx *octx, out_blk *first, char *end_dbg_lbl)
{
	struct flush_state st = { 0 };
	out_blk **must_i;

	st.f = cc_out[SECTION_TEXT];
	bfs_block(first, &st);

	for(must_i = octx->mustgen; must_i && *must_i; must_i++)
		bfs_block(*must_i, &st);

	if(st.jmpto)
		impl_jmp(st.f, st.jmpto->lbl);

	fprintf(st.f, "%s:\n", end_dbg_lbl);
}

void blk_terminate_condjmp(
		out_ctx *octx, char *condinsn,
		out_blk *condto, out_blk *uncondto,
		int unlikely)
{
	out_blk *current = octx->current_blk;

	if(!current){
		free(condinsn);
		return;
	}

	assert(current->type == BLK_UNINIT && "overwriting .next?");

	current->type = BLK_COND;

	current->bits.cond.insn = condinsn;
	current->bits.cond.if_0_blk = condto;
	current->bits.cond.if_1_blk = uncondto;
	current->bits.cond.unlikely = unlikely;

	octx->current_blk = NULL;
}

void blk_terminate_undef(out_blk *b)
{
	if(b)
		b->type = BLK_TERMINAL;
}

static out_blk *blk_new_common(out_ctx *octx, char *lbl, const char *desc)
{
	out_blk *blk = umalloc(sizeof *blk);
	blk->lbl = lbl;
	blk->desc = desc;

	blk->next = octx->mem_blk_head;
	octx->mem_blk_head = blk;

	return blk;
}

out_blk *out_blk_new_lbl(out_ctx *octx, const char *lbl)
{
	return blk_new_common(octx, ustrdup(lbl), lbl);
}

out_blk *out_blk_new(out_ctx *octx, const char *desc)
{
	return blk_new_common(octx, out_label_bblock(octx->nblks++), desc);
}
