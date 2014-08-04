#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include "../type.h"
#include "../type_nav.h"
#include "../type_is.h"

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
	/* must generate a prologue/epilogue if we aren't a leaf function,
	 * since the stack must be aligned correctly for a call
	 * (i.e. at least a pushq %rbp to bring it up to 16) */
	octx->used_stack = 1;

	return impl_call(octx, fn, args, fnty);
}

static void callee_save_or_restore_1(
		out_ctx *octx, out_blk *in_blk,
		struct vreg *cs, long stack_pos,
		type *voidpp, int reg2mem)
{
	out_current_blk(octx, in_blk);
	{
		const out_val *stk = v_new_bp3_below(octx, NULL, voidpp, stack_pos);

		if(reg2mem){
			const out_val *reg = v_new_reg(octx, NULL, type_is_ptr(voidpp), cs);
			out_store(octx, stk, reg);
		}else{
			out_flush_volatile(octx, impl_deref(octx, stk, cs));
		}
	}
}

static void callee_save_or_restore(
		out_ctx *octx, out_blk *spill_blk)
{
	struct vreg *i;
	long stack_n = 0;
	out_blk *restore_blk;
	unsigned voidpsz;
	type *voidp, *voidpp;

	voidp = type_ptr_to(type_nav_btype(cc1_type_nav, type_void));
	voidpp = type_ptr_to(voidp);
	voidpsz = type_size(voidp, NULL);

	for(i = octx->used_callee_saved; i && i->is_float != 2; i++)
		stack_n += voidpsz;

	if(!stack_n)
		return;

	v_alloc_stack(octx, stack_n, "callee-save");

	stack_n = octx->var_stack_sz; /* may be different - variadic functions */
	restore_blk = octx->epilogue_blk;

	for(i = octx->used_callee_saved; i && i->is_float != 2; i++){
		callee_save_or_restore_1(octx, spill_blk, i, stack_n, voidpp, 1);
		callee_save_or_restore_1(octx, restore_blk, i, stack_n, voidpp, 0);

		stack_n -= voidpsz;
	}
}

void out_func_epilogue(
		out_ctx *octx, type *ty, char *end_dbg_lbl,
		int *out_usedstack)
{
	out_blk *call_save_spill_blk = NULL;
	out_blk *to_flush;

	if(octx->current_blk && octx->current_blk->type == BLK_UNINIT)
		out_ctrl_transfer(octx, octx->epilogue_blk, NULL, NULL);

	/* must generate callee saves/restores before the
	 * epilogue or prologue blocks */
	if(octx->used_callee_saved){
		call_save_spill_blk = out_blk_new(octx, "call_save");

		callee_save_or_restore(octx, call_save_spill_blk);
	}

	out_current_blk(octx, octx->epilogue_blk);
	{
		impl_func_epilogue(octx, ty, octx->used_stack);
		/* terminate here without an insn */
		assert(octx->current_blk->type == BLK_UNINIT);
		octx->current_blk->type = BLK_TERMINAL;
	}

	*out_usedstack = octx->used_stack;

	/* space for spills */
	if(octx->used_stack){
		to_flush = octx->first_blk;
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

			if(call_save_spill_blk){
				out_ctrl_transfer(octx, call_save_spill_blk, NULL, NULL);
				out_current_blk(octx, call_save_spill_blk);
			}
			out_ctrl_transfer(octx, octx->second_blk, NULL, NULL);
		}
	}else{
		to_flush = octx->second_blk;

		/* need to attach the label to second_blk */
		free(to_flush->lbl);
		to_flush->lbl = octx->first_blk->lbl;
		octx->first_blk->lbl = NULL;
	}
	octx->current_blk = NULL;

	blk_flushall(octx, to_flush, end_dbg_lbl);

	free(octx->used_callee_saved), octx->used_callee_saved = NULL;

	octx->stack_local_offset =
		octx->stack_sz_initial =
		octx->var_stack_sz =
		octx->max_stack_sz =
		octx->stack_n_alloc = 0;
}

void out_func_prologue(
		out_ctx *octx, const char *sp,
		type *fnty,
		int stack_res, int nargs, int variadic,
		int arg_offsets[], int *local_offset)
{
	out_blk *post_prologue = out_blk_new(octx, "post_prologue");

	octx->current_fnty = fnty;

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

		impl_func_prologue_save_call_regs(octx, fnty, nargs, arg_offsets);

		if(variadic) /* save variadic call registers */
			impl_func_prologue_save_variadic(octx, fnty);

		/* setup "pointers" to the right place in the stack */
		octx->stack_variadic_offset = octx->var_stack_sz - platform_word_size();
		octx->stack_local_offset = octx->var_stack_sz;
		*local_offset = octx->stack_local_offset;

		if(stack_res)
			v_alloc_stack(octx, stack_res, "local variables");

		octx->stack_sz_initial = octx->var_stack_sz;
	}
	octx->in_prologue = 0;

	octx->used_stack = 0;

	/* keep the end of the prologue block clear for a stack pointer adjustment,
	 * in case any spills are needed */
	octx->second_blk = post_prologue;
	out_current_blk(octx, post_prologue);
}
