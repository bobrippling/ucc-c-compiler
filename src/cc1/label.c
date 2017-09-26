#include <stdlib.h>
#include <stdarg.h>

#include "../util/where.h"
#include "../util/alloc.h"
#include "../util/dynarray.h"

#include "sym.h"
#include "gen_ir_internal.h"

#include "label.h"

#include "out/out.h"
#include "cc1_out_ctx.h"

label *label_new(where *w, char *id, int complete, symtable *scope)
{
	label *l = umalloc(sizeof *l);
	memcpy_safe(&l->where, w);
	l->spel = id;
	l->complete = complete;
	l->scope = scope;
	return l;
}

static unsigned label_hash(const label *l)
{
	return dynmap_strhash(l->spel);
}

out_blk *label_getblk_octx(label *l, out_ctx *octx)
{
	struct cc1_out_ctx *cc1_octx = cc1_out_ctx_or_new(octx);
	out_blk *blk;

	if(!cc1_octx->label_to_blk)
		cc1_octx->label_to_blk = dynmap_new(label *, NULL, label_hash);

	blk = dynmap_get(
			label *, out_blk *,
			cc1_octx->label_to_blk,
			l);

	if(!blk){
		blk = out_blk_new(octx, l->spel);

		(void)dynmap_set(
				label *, out_blk *,
				cc1_octx->label_to_blk,
				l, blk);
	}

	return blk;
}

unsigned label_getblk_irctx(label *l, struct irctx *ctx)
{
	if(!l->blk)
		l->blk = 1 + ctx->curlbl++;

	return l->blk - 1;
}

void label_cleanup(out_ctx *octx)
{
	struct cc1_out_ctx *cc1_octx = *cc1_out_ctx(octx);
	if(!cc1_octx)
		return;

	dynmap_free(cc1_octx->label_to_blk), cc1_octx->label_to_blk = NULL;
}

void label_free(label *l)
{
	dynarray_free(struct stmt **, l->jumpers, NULL);
	free(l->mustgen_spel);
	free(l);
}
