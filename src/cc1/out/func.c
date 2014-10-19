#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include "../type.h"
#include "../type_nav.h"
#include "../type_is.h"
#include "../pack.h"

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
#include "../../util/dynarray.h"

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
		struct vreg *cs, const out_val *stack_pos,
		type *voidpp, int reg2mem)
{
	out_current_blk(octx, in_blk);
	{
		const out_val *stk = out_val_retain(octx, stack_pos);

		stk = out_change_type(octx, stk, voidpp);

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
	type *voidp, *voidpp, *arithty;
	const out_val *stack_locn;

	voidp = type_ptr_to(type_nav_btype(cc1_type_nav, type_void));
	voidpp = type_ptr_to(voidp);
	voidpsz = type_size(voidp, NULL);
	arithty = type_nav_btype(cc1_type_nav, type_intptr_t);

	for(i = octx->used_callee_saved; i && i->is_float != 2; i++)
		stack_n += voidpsz;

	if(!stack_n)
		return;

	stack_locn = out_aalloc(octx, stack_n, /*align*/voidpsz, voidp);

	stack_locn = out_change_type(octx, stack_locn, voidp);

	restore_blk = octx->epilogue_blk;

	for(i = octx->used_callee_saved; i && i->is_float != 2; i++){
		callee_save_or_restore_1(octx, spill_blk, i, stack_locn, voidpp, 1);
		callee_save_or_restore_1(octx, restore_blk, i, stack_locn, voidpp, 0);

		stack_locn = out_op(octx, op_plus, stack_locn,
				out_new_l(octx, arithty, voidpsz));
	}

	out_val_release(octx, stack_locn);
}

void out_func_epilogue(
		out_ctx *octx, type *ty, char *end_dbg_lbl,
		int *out_usedstack)
{
	out_blk *call_save_spill_blk = NULL;
	out_blk *to_flush;

	if(octx->current_blk && octx->current_blk->type == BLK_UNINIT)
		out_ctrl_transfer(octx, octx->epilogue_blk, NULL, NULL);

	assert(octx->alloca_count == 0 && "allocas after func gen?");

	/* must generate callee saves/restores before the
	 * epilogue or prologue blocks */
	if(octx->used_callee_saved){
		call_save_spill_blk = out_blk_new(octx, "call_save");

		/* ensure callee save doesn't overlap other parts of
		 * the stack, namely arguments. this is needed because
		 * even though cur_stack_sz is zero, we insert the callee
		 * save basic-block after argument handling, where cur_stack_sz
		 * is non-zero
		 */
		octx->in_prologue = 1;
		{
			octx->cur_stack_sz = octx->max_stack_sz;
			callee_save_or_restore(octx, call_save_spill_blk);
		}
		octx->in_prologue = 0;
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
			v_stackt stack_adj;

			/* must have more or equal stack to the alloc_n, because alloc_n will
			 * always add to {var,max}_stack_sz with possible padding,
			 * and that same value (minus padding) to stack_n_alloc */
			assert(octx->max_stack_sz >= octx->stack_n_alloc);

			out_comment(octx,
					"stack_sz{cur=%lu,max=%lu} stack_n_alloc=%lu (total=%lu) max_align=%u",
					octx->cur_stack_sz, octx->max_stack_sz, octx->stack_n_alloc,
					octx->cur_stack_sz + octx->stack_n_alloc,
					octx->max_align);

			if(octx->max_align){
				/* must align max_stack_sz,
				 * not the resultant after subtracting stack_n_alloc */
				octx->max_stack_sz = pack_to_align(octx->max_stack_sz, octx->max_align);
			}
			stack_adj = octx->max_stack_sz - octx->stack_n_alloc;

			v_stack_adj(octx, stack_adj, /*sub:*/1);

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

	dynarray_free(
			struct out_dbg_lbl **,
			octx->pending_lbls,
			NULL);

	octx->initial_stack_sz =
		octx->cur_stack_sz =
		octx->max_stack_sz =
		octx->max_align =
		octx->stack_n_alloc = 0;
}

static void stack_realign(out_ctx *octx, unsigned align)
{
	type *const arithty = type_nav_btype(cc1_type_nav, type_intptr_t);
	const unsigned new_sz = pack_to_align(octx->cur_stack_sz, align);
	unsigned to_add = new_sz - octx->cur_stack_sz;
	const out_val *sp = v_new_sp(octx, NULL);

	if(to_add == 0)
		to_add = align;

	sp = out_op(octx, op_minus,
			sp, out_new_l(octx, arithty, to_add));

	sp = out_op(octx, op_and, sp, out_new_l(octx, arithty, align - 1));

	out_val_release(octx, sp);
	assert(sp->retains == 0);

	out_comment(octx, "stack aligned to %u bytes", align);

	v_set_cur_stack_sz(octx, new_sz);
}

void out_func_prologue(
		out_ctx *octx, const char *sp,
		type *fnty,
		int nargs, int variadic,
		const out_val *argvals[])
{
	out_blk *post_prologue = out_blk_new(octx, "post_prologue");

	octx->current_fnty = fnty;

	assert(octx->cur_stack_sz == 0 && "non-empty stack for new func");
	assert(octx->alloca_count == 0 && "allocas left over?");

	octx->check_flags = 1;

	octx->in_prologue = 1;
	{
		assert(!octx->current_blk);
		octx->first_blk = out_blk_new_lbl(octx, sp);
		octx->epilogue_blk = out_blk_new(octx, "epilogue");

		out_current_blk(octx, octx->first_blk);

		impl_func_prologue_save_fp(octx);

		if(mopt_mode & MOPT_STACK_REALIGN)
			stack_realign(octx, cc1_mstack_align);

		impl_func_prologue_save_call_regs(octx, fnty, nargs, argvals);

		if(variadic) /* save variadic call registers */
			impl_func_prologue_save_variadic(octx, fnty);

		/* setup "pointers" to the right place in the stack */
		octx->stack_variadic_offset = octx->cur_stack_sz - platform_word_size();
		octx->initial_stack_sz = octx->cur_stack_sz;
	}
	octx->in_prologue = 0;

	octx->used_stack = 0;

	/* keep the end of the prologue block clear for a stack pointer adjustment,
	 * in case any spills are needed */
	octx->second_blk = post_prologue;
	out_current_blk(octx, post_prologue);
}
