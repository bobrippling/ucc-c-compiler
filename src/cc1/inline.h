#ifndef INLINE_H
#define INLINE_H

#include "out/forwards.h"

ucc_nonnull((3, 4, 5, 6))
const out_val *inline_func_try_gen(
		expr *maybe_call_expr, decl *maybe_decl,
		const out_val *fnval,
		const out_val **args,
		out_ctx *octx,
		const char **whynot, const where *call_loc);

void inline_ret_add(out_ctx *octx, const out_val *v);

#endif
