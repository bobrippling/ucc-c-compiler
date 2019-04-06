#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include "../type.h"
#include "../type_nav.h"
#include "../type_is.h"
#include "../pack.h"
#include "../mangle.h"
#include "../funcargs.h"

#include "val.h"
#include "out.h" /* this file defs */
#include "asm.h"
#include "impl.h"
#include "ctx.h"
#include "virt.h"
#include "blk.h"
#include "dbg.h"
#include "stack_protector.h"

#include "../cc1.h" /* mopt_mode */
#include "../fopt.h"
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
		type *voidpp, int reg2mem,
		const where *fnstart, const where *fnend)
{
	out_current_blk(octx, in_blk);
	{
		const out_val *stk = out_val_retain(octx, stack_pos);

		stk = out_change_type(octx, stk, voidpp);

		if(reg2mem){
			const out_val *reg;
			out_dbg_where(octx, fnstart);
			reg = v_new_reg(octx, NULL, type_is_ptr(voidpp), cs);
			out_store(octx, stk, reg);
		}else{
			out_dbg_where(octx, fnend);
			out_flush_volatile(octx, impl_deref(octx, stk, cs, NULL));
		}
	}
}

static void callee_save_or_restore(
		out_ctx *octx, out_blk *spill_blk,
		const where *fnstart, const where *fnend)
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
		callee_save_or_restore_1(octx, spill_blk, i, stack_locn, voidpp, 1, fnstart, fnend);
		callee_save_or_restore_1(octx, restore_blk, i, stack_locn, voidpp, 0, fnstart, fnend);

		stack_locn = out_op(octx, op_plus, stack_locn,
				out_new_l(octx, arithty, voidpsz));
	}

	out_val_release(octx, stack_locn);
}

void out_func_epilogue(out_ctx *octx, type *ty, const where *func_begin, char *end_dbg_lbl)
{
	int clean_stack;
	out_blk *call_save_spill_blk = NULL;
	out_blk *flush_root;

	if(octx->current_blk && octx->current_blk->type == BLK_UNINIT)
		out_ctrl_transfer(octx, octx->epilogue_blk, NULL, NULL, 0);

	assert(octx->alloca_count == 0 && "allocas after func gen?");

	/* must generate callee saves/restores before the
	 * epilogue or prologue blocks */
	if(octx->used_callee_saved){
		call_save_spill_blk = out_blk_new(octx, "call_save");

		/* Ensure callee save doesn't overlap other parts of the stack, namely
		 * arguments (i.e. things stored closest to the base, aka offset-zero).
		 *
		 * This is needed because even though cur_stack_sz is zero at this point
		 * in code, we insert the callee save basic-block after argument handling,
		 * where cur_stack_sz is non-zero (due to stored arguments, etc).
		 *
		 * Additionally, we don't want to stash the callee saved arguments at
		 * the bottom of the stack, because that's where arguments for
		 * many-parameter-functions live, so the callee saves need to be just
		 * before that.
		 *
		 * <extra arguments>
		 * <saved ret>
		 * <saved rsp>
		 * <saved arguments>
		 * <local variables>
		 * <callee saves> <---- this (or similar) is what we want, with no overlap
		 *                     \ either way
		 * <spill space / overflow arg space for child calls and vlas>
		 *
		 * this is what we get, which works too:
		 * <local variables>
		 * <spill space>
		 * <callee saves>
		 * <overflow arg space for child calls and vlas>
		 */
		octx->in_prologue = 1;
		{
			/* Here, we bring cur_stack_sz up to max_stack_sz (which is the maximum
			 * stack usage for the current function), less the volatile
			 * spill/stack_callspace.
			 *
			 * We then perform callee_save_or_restore(), which allocates space,
			 * adjusting max_stack_sz in the process, from our offset.
			 *
			 * If we ignore stack_callspace, then this works fine, because
			 * max_stack_sz is now the function's stack size, plus that for
			 * callee save register.
			 *
			 * However, when stack_callspace is taken into account, we suddenly
			 * allocate our callee save registers in the same space as the temporary
			 * space used for stack_callspace (or whatever else uses this space
			 * after stack-reclaimation, such as spills).
			 *
			 * So we want to insert our callee-save registers, not at the bottom of
			 * the stack (because this would unconditionally collide them with the
			 * stack_callspace area, or the vla expansion area), but before this.
			 * However, we can't just set
			 *   cur_stack_sz = max_stack_sz - stack_callspace
			 * ... because then we overlap the stack_callspace with callee-save
			 * registers. Unfortuantely it's slightly chicken-and-egg, in that
			 * the vla-space, spill code and stack_callspace code has already run.
			 *
			 * Fortunately, similar problems have been encountered before, and
			 * both the vla-space and stack_callspace logic offset themselves
			 * based on the runtime-bottom of the stack (%rsp) instead of the stack
			 * base (%rbp). So all that remains for us to do, is increase the
			 * runtime-bottom of the stack by the difference that the callee
			 * saves take up, done below.
			 */
			const v_stackt old_max_stack_sz = octx->max_stack_sz;
			long callee_stack_diff;
			where func_end;
			memcpy_safe(&func_end, &octx->dbg.where);

			octx->cur_stack_sz = octx->max_stack_sz;

			callee_save_or_restore(octx, call_save_spill_blk, func_begin, &func_end);

			callee_stack_diff = octx->max_stack_sz - old_max_stack_sz;
			if(callee_stack_diff){
				assert(callee_stack_diff > 0);
				/*
				 * We are now in this situation:
				 *
				 * <extra arguments>
				 * <saved ret>
				 * <saved rsp>
				 * <saved arguments>
				 * <local variables>
				 * <
				 *  callee saves
				 *  AND
				 *  spill space / overflow arg space for child calls and vlas
				 *  (shared because of stack-reclaim)
				 * >
				 *
				 * Now the spill-space logic saves values at fixes offsets from
				 * the stack base, so we can't place our callee-saves there.
				 * So we bump the max-stack to end up like so:
				 *
				 * ...
				 * <local variables>
				 * <spill space>
				 * <callee saves>
				 * <overflow arg space for child calls and vlas>
				 *
				 * However, there is still overlap between the callee-saves and the
				 * overflow arg space / vla space, because this was previously shared
				 * between that and the spills. We now can't make use of this sharing
				 * any more, so must ensure the stack-callspace is entirely separate
				 * from the callee-saves (as it overlaps them by the amount it shared
				 * with the spill space).
				 *
				 * Hence, max-stack-sz += callee_stack_diff + stack-callspace;
				 * This is a little wasteful, but to fix, we would have to move the
				 * callee saves to before the spill space, which would require a larger
				 * restructure.
				 */
				octx->max_stack_sz += callee_stack_diff + octx->stack_callspace;
				octx->stack_calleesave_space = callee_stack_diff;
			}
		}
		octx->in_prologue = 0;
	}

	clean_stack = octx->used_stack || !cc1_fopt.omit_frame_pointer;

	out_current_blk(octx, octx->epilogue_blk);
	{
		out_check_stack_canary(octx);

		impl_func_epilogue(octx, ty, clean_stack);
		/* terminate here without an insn */
		assert(octx->current_blk->type == BLK_UNINIT);
		octx->current_blk->type = BLK_TERMINAL;
	}

	/* space for spills */
	if(clean_stack){
		flush_root = octx->entry_blk;
		out_current_blk(octx, octx->entry_blk);
		out_ctrl_transfer_make_current(octx, octx->stacksub_blk);
		{
			v_stackt stack_adj;

			/* must have more or equal stack to the alloc_n, because alloc_n will
			 * always add to {var,max}_stack_sz with possible padding,
			 * and that same value (minus padding) to stack_n_alloc */
			assert(octx->max_stack_sz >= octx->stack_n_alloc);

			out_comment(octx,
					"stack_sz{cur=%lu,max=%lu} n_alloc=%lu call_spc=%lu calleesve=%lu max_align=%u",
					octx->cur_stack_sz,
					octx->max_stack_sz,
					octx->stack_n_alloc,
					octx->stack_callspace,
					octx->stack_calleesave_space,
					octx->max_align);

			if(octx->max_align){
				/* must align max_stack_sz,
				 * not the resultant after subtracting stack_n_alloc */
				octx->max_stack_sz = pack_to_align(octx->max_stack_sz, octx->max_align);
			}
			stack_adj = octx->max_stack_sz - octx->stack_n_alloc;

			v_stack_adj(octx, stack_adj, /*sub:*/1);
		}

		out_ctrl_transfer(octx, octx->argspill_begin_blk, NULL, NULL, 0);
		out_current_blk(octx, octx->argspill_done_blk);
		if(call_save_spill_blk)
			out_ctrl_transfer_make_current(octx, call_save_spill_blk);
		out_ctrl_transfer(octx, octx->postprologue_blk, NULL, NULL, 0);
	}else{
		flush_root = octx->postprologue_blk;

		/* need to attach the label to prologue_postjoin_blk */
		free(flush_root->lbl);
		flush_root->lbl = octx->entry_blk->lbl;
		octx->entry_blk->lbl = NULL;
	}
	octx->current_blk = NULL;

	blk_flushall(octx, flush_root, end_dbg_lbl);

	free(octx->used_callee_saved), octx->used_callee_saved = NULL;

	if(octx->current_stret){
		out_val_release(octx, octx->current_stret);
		octx->current_stret = NULL;
	}

	dynarray_free(
			struct out_dbg_lbl **,
			octx->pending_lbls,
			NULL);

	octx->initial_stack_sz =
		octx->cur_stack_sz =
		octx->max_stack_sz =
		octx->max_align =
		octx->stack_callspace =
		octx->stack_calleesave_space =
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
		int nargs, int variadic, int stack_protector,
		const out_val *argvals[])
{
	octx->current_fnty = fnty;

	assert(octx->cur_stack_sz == 0 && "non-empty stack for new func");
	assert(octx->alloca_count == 0 && "allocas left over?");
	octx->check_flags = 1;

	assert(!octx->current_blk);
	octx->entry_blk = out_blk_new_lbl(octx, sp);
	octx->stacksub_blk = out_blk_new(octx, "stacksub");
	octx->argspill_begin_blk = out_blk_new(octx, "argspill");
	octx->postprologue_blk = out_blk_new(octx, "post_prologue");
	octx->epilogue_blk = out_blk_new(octx, "epilogue");

	octx->in_prologue = 1;
	{
		const out_val *stack_prot_slot = NULL;

		out_current_blk(octx, octx->entry_blk);
		{
			impl_func_prologue_save_fp(octx);

			if(stack_protector){
				type *intptr_ty = type_nav_btype(cc1_type_nav, type_intptr_t);

				stack_prot_slot = out_aalloct(octx, type_ptr_to(type_ptr_to(intptr_ty)));
			}

			if(mopt_mode & MOPT_STACK_REALIGN)
				stack_realign(octx, cc1_mstack_align);
		}

		out_current_blk(octx, octx->argspill_begin_blk);
		{
			impl_func_prologue_save_call_regs(octx, fnty, nargs, argvals);

			if(variadic) /* save variadic call registers */
				impl_func_prologue_save_variadic(octx, fnty);

			octx->argspill_done_blk = octx->current_blk;

			/* setup "pointers" to the right place in the stack */
			octx->stack_variadic_offset = octx->cur_stack_sz;
			octx->initial_stack_sz = octx->cur_stack_sz;

			if(stack_prot_slot){
				/* now all registers are safe, init stack protector */
				out_init_stack_canary(octx, stack_prot_slot);
			}
		}
	}
	octx->in_prologue = 0;

	octx->used_stack = 0;

	out_current_blk(octx, octx->postprologue_blk);
}

unsigned out_current_stack(out_ctx *octx)
{
	return octx->max_stack_sz;
}
