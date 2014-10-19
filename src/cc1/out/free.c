#include <stdio.h>
#include <stdlib.h>

#include "../../util/dynmap.h"
#include "../../util/dynarray.h"

#include "../type.h"

#include "forwards.h"
#include "val.h"
#include "blk.h"
#include "ctx.h"
#include "out.h"
#include "dbg_lbl.h"

static unsigned blk_hash(const out_blk *b)
{
	return b->type
		^ (unsigned)(unsigned long)b->desc
		^ dynarray_count(b->insns);
}

static void out_blk_uniq(out_blk *blk, dynmap *uniq_blks)
{
	if(dynmap_exists(out_blk *, uniq_blks, blk))
		return;

	dynmap_set(out_blk *, void *, uniq_blks, blk, (void *)0);

	switch(blk->type){
		case BLK_UNINIT:
		case BLK_TERMINAL:
		case BLK_NEXT_EXPR:
			break;
		case BLK_NEXT_BLOCK:
			out_blk_uniq(blk->bits.next, uniq_blks);
			break;
		case BLK_COND:
			out_blk_uniq(blk->bits.cond.if_0_blk, uniq_blks);
			out_blk_uniq(blk->bits.cond.if_1_blk, uniq_blks);
			break;
	}
}

static void blk_free(out_blk *blk)
{
	char **i;

	if(blk->labels){
		struct out_dbg_lbl **dbg_i;

		for(dbg_i = blk->labels; *dbg_i; dbg_i++){
			RELEASE(*dbg_i);
		}
		dynarray_free(struct out_dbg_lbl *, blk->labels, NULL);
	}

	free(blk->lbl);

	for(i = blk->insns; i && *i; i++)
		free(*i);
	free(blk->insns);

	if(blk->type == BLK_COND)
		free(blk->bits.cond.insn);

	dynarray_free(out_blk **, blk->merge_preds, NULL);

	/* .phi_val is handled by the out_val code */

	free(blk);
}

static void out_wipe_blks(out_ctx *octx)
{
	dynmap *uniq_blks;
	out_blk **bi, *b;
	size_t i;

	if(!octx->first_blk)
		return;

	uniq_blks = dynmap_new(out_blk *, NULL, blk_hash);

	for(bi = octx->mustgen; bi && *bi; bi++)
		out_blk_uniq(*bi, uniq_blks);

	out_blk_uniq(octx->epilogue_blk, uniq_blks);
	out_blk_uniq(octx->first_blk, uniq_blks);

	/* add in case we don't flow from .first_blk: */
	out_blk_uniq(octx->second_blk, uniq_blks);

	for(i = 0; (b = dynmap_key(out_blk *, uniq_blks, i)); i++)
		blk_free(b);

	dynmap_free(uniq_blks);

	octx->first_blk =
	octx->second_blk =
	octx->current_blk =
	octx->epilogue_blk =
	octx->last_used_blk = NULL;

	dynarray_free(out_blk **, octx->mustgen, NULL);
}

static void out_wipe_vals(out_ctx *octx)
{
	struct out_val_list *l, *next;

	for(l = octx->val_head; l; l = next){
		next = l->next;
		/* nothing inside l->val to free - either primitives or type * */
		free(l);
	}

	octx->val_head = octx->val_tail = NULL;
}

void out_ctx_wipe(out_ctx *octx)
{
	out_wipe_blks(octx);
	out_wipe_vals(octx);
}

void out_ctx_end(out_ctx *octx)
{
	out_ctx_wipe(octx);
	free(octx->reserved_regs);
	free(octx);
}
