#include <stddef.h>
#include <assert.h>

#include "cc1_out_ctx.h"

#include "../util/alloc.h"
#include "out/out.h"

struct cc1_out_ctx *cc1_out_ctx_or_new(out_ctx *octx)
{
	struct cc1_out_ctx **cc1_octx = cc1_out_ctx(octx);

	if(!*cc1_octx)
		*cc1_octx = umalloc(sizeof **cc1_octx);

	return *cc1_octx;
}

void cc1_out_ctx_free(out_ctx *octx)
{
	struct cc1_out_ctx **cc1_octx = cc1_out_ctx(octx);

	if(!*cc1_octx)
		return;

	dynmap_free((*cc1_octx)->sym_inline_map);
	dynmap_free((*cc1_octx)->vlamap);
	dynmap_free((*cc1_octx)->generated_decls);
	dynmap_free((*cc1_octx)->spel_to_fndecl);

	assert(!(*cc1_octx)->label_to_blk
			&& "label_to_blk should be handled elsewhere");

	free(*cc1_octx);
}
