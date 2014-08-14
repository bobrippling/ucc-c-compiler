#ifndef INLINE_H
#define INLINE_H

#include "out/forwards.h"

ucc_nonnull()
const out_val *inline_func_try_gen(
		expr *call_expr,
		const out_val *fn, const out_val **args,
		out_ctx *octx, const char **whynot);

void inline_ret_add(out_ctx *octx, const out_val *v);

#endif
