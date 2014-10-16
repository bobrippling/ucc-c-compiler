#include <stdio.h>
#include <stddef.h>

#include "../../util/dynarray.h"
#include "../../util/alloc.h"

#include "dbg_lbl.h"

/* deps */
#include "../type.h"

/* needed defs */
#include "val.h"
#include "ctx.h"

/* functions */
#include "blk.h"

static void add_lbl_to_blk(struct out_dbg_lbl *lbl, out_blk *blk)
{
	RETAIN(lbl);
	dynarray_add(&blk->labels, lbl);
}

static void out_dbg_label_free(struct out_dbg_lbl *lbl)
{
	free(lbl->lbl);
	free(lbl);
}

void out_dbg_label_push(
		out_ctx *octx,
		char *lbl[ucc_static_param 2],
		struct out_dbg_lbl **pushed)
{
	out_blk *blk = octx->current_blk;
	struct out_dbg_lbl *attached_lbl;

	if(!blk)
		blk = octx->last_used_blk;

	blk_add_insn(blk, ustrprintf("%s:\n", lbl[0]));
	free(lbl[0]);

	attached_lbl = umalloc(sizeof *attached_lbl);
	RETAIN_INIT(attached_lbl, &out_dbg_label_free);

	attached_lbl->lbl = lbl[1];

	/* add to octx - octx emits this label at the end of the block gen if the
	 * block containing it (set in out_dbg_label_pop()) hasn't been emitted */
	dynarray_add(&octx->pending_lbls, attached_lbl);

	/* out param */
	*pushed = attached_lbl;
}

void out_dbg_label_pop(out_ctx *octx, struct out_dbg_lbl *to_pop)
{
	out_blk *blk = octx->current_blk;
	if(!blk)
		blk = octx->last_used_blk;

	/* this is the block we should emit the label from */
	add_lbl_to_blk(to_pop, blk);
}

int out_dbg_label_shouldemit(struct out_dbg_lbl *lbl, const char **out_lbl)
{
	if(out_lbl)
		*out_lbl = lbl->lbl;

	return !lbl->emitted;
}

static void emit_lbl(FILE *f, struct out_dbg_lbl *lbl)
{
	/* if we haven't emitted the label yet, and its
	 * pair start label/start block was emitted */
	if(out_dbg_label_shouldemit(lbl, NULL)){
		fprintf(f, "%s:\n", lbl->lbl);
		lbl->emitted = 1;
	}
}

void out_dbg_labels_emit_v(FILE *f, struct out_dbg_lbl **v)
{
	for(; v && *v; v++)
		emit_lbl(f, *v);
}
