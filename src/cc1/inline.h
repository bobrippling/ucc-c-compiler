#ifndef INLINE_H
#define INLINE_H

#include "out/forwards.h"

const out_val *try_gen_inline_func(
		expr *call_expr,
		const out_val *fn, const out_val **args,
		out_ctx *octx);

void inline_ret_add(out_ctx *octx, const out_val *v);

#define INLINE_DEPTH_MAX 5

#endif
