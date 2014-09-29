#ifndef OUT_USER_CTX_H
#define OUT_USER_CTX_H

#include "out/forwards.h"

struct cc1_out_ctx
{
	struct dynmap *vlamap;
	struct dynmap *sym_inline_map;

	struct dynmap *generated_decls; /* decl* => NULL */

	struct dynmap *spel_to_fndecl;

	struct cc1_inline
	{
		struct out_blk *phi, **rets;
		unsigned depth;
	} inline_;
};

#define cc1_out_ctx(octx) ((struct cc1_out_ctx **)out_user_ctx(octx))

struct cc1_out_ctx *cc1_out_ctx_or_new(out_ctx *octx);
void cc1_out_ctx_free(out_ctx *);

#endif
