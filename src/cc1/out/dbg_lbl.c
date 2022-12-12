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
#include "asm.h"

static void add_lbl_to_blk(
		struct out_dbg_lbl *lbl,
		struct out_dbg_lbl ***parray)
{
	(void)RETAIN(lbl);
	dynarray_add(parray, lbl);
}

static void out_dbg_label_free(struct out_dbg_lbl *lbl)
{
	free(lbl->lbl);
	free(lbl);
}

static struct out_dbg_lbl *dbg_lbl_new(char *strlbl)
{
	struct out_dbg_lbl *lbl = umalloc(sizeof *lbl);
	RETAIN_INIT(lbl, &out_dbg_label_free);
	lbl->lbl = strlbl;
	return lbl;
}

void out_dbg_label_push(
		out_ctx *octx,
		char *lbl[ucc_static_param 2],
		struct out_dbg_lbl **out_startlbl,
		struct out_dbg_lbl **out_endlbl)
{
	out_blk *blk = octx->current_blk;
	struct out_dbg_lbl *startlbl, *endlbl;

	if(!blk)
		blk = octx->last_used_blk;

	startlbl = dbg_lbl_new(lbl[0]);
	endlbl = dbg_lbl_new(lbl[1]);

	/* add to current block */
	add_lbl_to_blk(startlbl, &blk->labels.start);

	/* add to octx - octx emits this label at the end of the block gen if the
	 * block containing it (set in out_dbg_label_pop()) hasn't been emitted */
	dynarray_add(&octx->pending_lbls, RETAIN(endlbl));

	/* out params */
	*out_startlbl = startlbl;
	*out_endlbl = endlbl;
}

void out_dbg_label_pop(out_ctx *octx, struct out_dbg_lbl *to_pop)
{
	out_blk *blk = octx->current_blk;
	if(!blk)
		blk = octx->last_used_blk;

	/* this is the block we should emit the label from */
	add_lbl_to_blk(to_pop, &blk->labels.end);

	/* release the caller's hold / hold returned by out_dbg_label_push() */
	RELEASE(to_pop);
}

int out_dbg_label_emitted(struct out_dbg_lbl *lbl, const char **out_lbl)
{
	if(out_lbl)
		*out_lbl = lbl->lbl;

	return lbl->emitted;
}

int out_dbg_label_shouldemit(struct out_dbg_lbl *lbl, const char **out_lbl)
{
	return !out_dbg_label_emitted(lbl, out_lbl);
}

static void emit_lbl(struct out_dbg_lbl *lbl, const struct section *sec)
{
	/* if we haven't emitted the label yet, and its
	 * pair start label/start block was emitted */
	if(out_dbg_label_shouldemit(lbl, NULL)){
		asm_out_section(sec, "%s:\n", lbl->lbl);
		lbl->emitted = 1;
	}
	RELEASE(lbl);
}

void out_dbg_labels_emit_release_v(struct out_dbg_lbl ***pv, const struct section *sec)
{
	struct out_dbg_lbl **v = *pv;

	for(; v && *v; v++)
		emit_lbl(*v, sec);

	dynarray_free(struct out_dbg_lbl **, *pv, NULL);
}
