#ifndef INLINE_H
#define INLINE_H

#include "out/forwards.h"

const out_val *inline_func_try_gen(
		expr *call_expr,
		const out_val *fn, const out_val **args,
		out_ctx *octx);

int inline_func_possible(expr *call_expr, int nargs, const char **why);

void inline_ret_add(out_ctx *octx, const out_val *v);

#endif
