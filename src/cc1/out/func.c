#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include "../type.h"

#include "val.h"
#include "out.h" /* this file defs */
#include "asm.h"
#include "impl.h"
#include "ctx.h"
#include "virt.h"
#include "blk.h"
#include "dbg.h"

#include "../cc1.h" /* mopt_mode */
#include "../../util/platform.h"

const out_val *out_call(out_ctx *octx,
		const out_val *fn, const out_val **args,
		type *fnty)
{
	return impl_call(octx, fn, args, fnty);
}

void out_func_epilogue(out_ctx *octx, type *ty, char *end_dbg_lbl)
{
	if(octx->current_blk && octx->current_blk->type == BLK_UNINIT)
		out_ctrl_transfer(octx, octx->epilogue_blk, NULL);

	out_current_blk(octx, octx->epilogue_blk);
	{
		impl_func_epilogue(octx, ty);
		out_dbg_label(octx, end_dbg_lbl);
		/* terminate here without an insn */
		assert(octx->current_blk->type == BLK_UNINIT);
		octx->current_blk->type = BLK_TERMINAL;
	}
	octx->current_blk = NULL;

	blk_flushall(octx);

	/* TODO: flush/free octx->first_blk */

	octx->stack_local_offset = octx->stack_sz = 0;
}

void out_func_prologue(
		out_ctx *octx, const char *sp,
		type *rf,
		int stack_res, int nargs, int variadic,
		int arg_offsets[], int *local_offset)
{
	assert(octx->stack_sz == 0 && "non-empty stack for new func");

	assert(!octx->current_blk);
	octx->first_blk = out_blk_new_lbl(octx, sp);
	octx->epilogue_blk = out_blk_new(octx, "epilogue");

	out_current_blk(octx, octx->first_blk);

	impl_func_prologue_save_fp(octx);

	if(mopt_mode & MOPT_STACK_REALIGN)
		v_stack_align(octx, cc1_mstack_align, 1);

	impl_func_prologue_save_call_regs(octx, rf, nargs, arg_offsets);

	if(variadic) /* save variadic call registers */
		impl_func_prologue_save_variadic(octx, rf);

	/* setup "pointers" to the right place in the stack */
	octx->stack_variadic_offset = octx->stack_sz - platform_word_size();
	octx->stack_local_offset = octx->stack_sz;
	*local_offset = octx->stack_local_offset;

	if(stack_res)
		v_alloc_stack(octx, stack_res, "local variables");
}
