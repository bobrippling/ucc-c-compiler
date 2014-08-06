#ifndef INLINE_H
#define INLINE_H

#include "out/forwards.h"

const out_val *try_gen_inline_func(
		expr *call_expr,
		const out_val *fn, const out_val **args,
		out_ctx *octx);

#endif
