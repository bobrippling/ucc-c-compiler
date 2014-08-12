#ifndef INLINE_H
#define INLINE_H

#include "out/forwards.h"

const out_val *inline_func_try_gen(
		expr *call_expr,
		const out_val *fn, const out_val **args,
		out_ctx *octx);

int can_inline_func(expr *call_expr); // do this?

void inline_ret_add(out_ctx *octx, const out_val *v);

#define INLINE_DEPTH_MAX 5
#define INLINE_MAX_STACK_BYTES 256
#define INLINE_MAX_STMTS 10

#endif
