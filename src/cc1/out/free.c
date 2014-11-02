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

static void blk_free_labels(struct out_dbg_lbl ***parray)
{
	struct out_dbg_lbl **dbg_i;

	if(!*parray)
		return;

	for(dbg_i = *parray; *dbg_i; dbg_i++){
		RELEASE(*dbg_i);
	}
	dynarray_free(struct out_dbg_lbl **, *parray, NULL);
}

static void blk_free(out_blk *blk)
{
	char **i;

	blk_free_labels(&blk->labels.start);
	blk_free_labels(&blk->labels.end);

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
	out_blk *b, *next;

	for(b = octx->mem_blk_head; b; b = next){
		next = b->next;
		blk_free(b);
	}

	octx->first_blk =
	octx->second_blk =
	octx->current_blk =
	octx->epilogue_blk =
	octx->last_used_blk =
	octx->mem_blk_head =
		NULL;

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
