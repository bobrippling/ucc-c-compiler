#ifndef OUT_USER_CTX_H
#define OUT_USER_CTX_H

#include "out/forwards.h"

struct cc1_out_ctx
{
	struct dynmap *vlamap;
	struct cc1_dbg_ctx
	{
		struct DIE_compile_unit *compile_unit;
		struct DIE *current_scope;
	} dbg;
};

#define cc1_out_ctx(octx) ((struct cc1_out_ctx **)out_user_ctx(octx))

struct cc1_out_ctx *cc1_out_ctx_or_new(out_ctx *octx);
void cc1_out_ctx_free(out_ctx *);

#endif
