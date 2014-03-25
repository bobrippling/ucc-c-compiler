#include <stdlib.h>

#include "../type.h"
#include "../sym.h"

#include "out.h" /* this file defs */
#include "val.h"

out_val *out_new_blk_addr(out_ctx *octx, out_blk *blk)
{
	return 0;
}

out_val *out_new_frame_ptr(out_ctx *octx, int nframes)
{
	return 0;
}

out_val *out_new_l(out_ctx *octx, type *ty, long val)
{
	return 0;
}

out_val *out_new_lbl(out_ctx *octx, const char *s, int pic)
{
	return 0;
}

out_val *out_new_nan(out_ctx *octx, type *ty)
{
	return 0;
}

out_val *out_new_noop(out_ctx *octx)
{
	return 0;
}

out_val *out_new_num(out_ctx *octx, type *t, const numeric *n)
{
	return 0;
}

out_val *out_new_overflow(out_ctx *octx)
{
	return 0;
}

out_val *out_new_reg_save_ptr(out_ctx *octx)
{
	return 0;
}

out_val *out_new_sym(out_ctx *octx, sym *sym)
{
	return 0;
}

out_val *out_new_sym_val(out_ctx *octx, sym *sym)
{
	return 0;
}

out_val *out_new_zero(out_ctx *octx, type *ty)
{
	return 0;
}
