#include <stdlib.h>

#include "../type.h"
#include "../sym.h"

#include "out.h" /* this file defs */
#include "val.h"

#include <stdio.h>
#define TODO() fprintf(stderr, "TODO: %s\n", __func__)

out_val *out_new_blk_addr(out_ctx *octx, out_blk *blk)
{
	TODO();
	return 0;
}

out_val *out_new_frame_ptr(out_ctx *octx, int nframes)
{
	TODO();
	return 0;
}

out_val *out_new_l(out_ctx *octx, type *ty, long val)
{
	TODO();
	return 0;
}

out_val *out_new_lbl(out_ctx *octx, const char *s, int pic)
{
	TODO();
	return 0;
}

out_val *out_new_nan(out_ctx *octx, type *ty)
{
	TODO();
	return 0;
}

out_val *out_new_noop(out_ctx *octx)
{
	TODO();
	return 0;
}

out_val *out_new_num(out_ctx *octx, type *t, const numeric *n)
{
	TODO();
	return 0;
}

out_val *out_new_overflow(out_ctx *octx)
{
	TODO();
	return 0;
}

out_val *out_new_reg_save_ptr(out_ctx *octx)
{
	TODO();
	return 0;
}

out_val *out_new_sym(out_ctx *octx, sym *sym)
{
	TODO();
}

out_val *out_new_sym_val(out_ctx *octx, sym *sym)
{
	TODO();
	return 0;
}

out_val *out_new_zero(out_ctx *octx, type *ty)
{
	TODO();
	return 0;
}
