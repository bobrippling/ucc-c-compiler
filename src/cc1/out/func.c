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
		out_ctrl_transfer(octx, octx->epilogue_blk, NULL, NULL);

	out_current_blk(octx, octx->epilogue_blk);
	{
		impl_func_epilogue(octx, ty);
		/* terminate here without an insn */
		assert(octx->current_blk->type == BLK_UNINIT);
		octx->current_blk->type = BLK_TERMINAL;
	}

	/* space for spills */
	out_current_blk(octx, octx->first_blk);
	{
		if(fopt_mode & FOPT_VERBOSE_ASM){
			out_comment(octx, "spill space %u, alloc_n space %u",
					octx->max_stack_sz - octx->stack_sz_initial,
					octx->stack_n_alloc);
		}

		/* must have more or equal stack to the alloc_n, because alloc_n will
		 * always add to {var,max}_stack_sz with possible padding,
		 * and that same value (minus padding) to stack_n_alloc */
		assert(octx->max_stack_sz >= octx->stack_n_alloc);

		v_stack_adj(octx, octx->max_stack_sz - octx->stack_n_alloc, /*sub:*/1);
	}
	octx->current_blk = NULL;

	blk_flushall(octx, end_dbg_lbl);

	octx->stack_local_offset =
		octx->stack_sz_initial =
		octx->var_stack_sz =
		octx->max_stack_sz =
		octx->stack_n_alloc = 0;
}

void out_func_prologue(
		out_ctx *octx, const char *sp,
		type *rf,
		int stack_res, int nargs, int variadic,
		int arg_offsets[], int *local_offset)
{
	out_blk *post_prologue = out_blk_new(octx, "post_prologue");

	assert(octx->var_stack_sz == 0 && "non-empty stack for new func");

	octx->in_prologue = 1;
	{
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
		octx->stack_variadic_offset = octx->var_stack_sz - platform_word_size();
		octx->stack_local_offset = octx->var_stack_sz;
		*local_offset = octx->stack_local_offset;

		if(stack_res)
			v_alloc_stack(octx, stack_res, "local variables");

		octx->stack_sz_initial = octx->var_stack_sz;
	}
	octx->in_prologue = 0;

	/* keep the end of the prologue block clear for a stack pointer adjustment,
	 * in case any spills are needed */
	out_ctrl_transfer_make_current(octx, post_prologue);
}
