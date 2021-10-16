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
#include "func.h"

#include "../cc1.h" /* mopt_mode */
#include "../fopt.h"
#include "../../util/platform.h"
#include "../../util/dynarray.h"
#include "../../util/alloc.h"

const out_val *out_call(out_ctx *octx,
		const out_val *fn, const out_val **args,
		type *fnty)
{
	type *const retty = type_called(type_is_ptr_or_block(fnty), NULL);
	type *const arithty = type_nav_btype(cc1_type_nav, type_intptr_t);
	const unsigned pws = platform_word_size();
	const unsigned nargs = dynarray_count(args);
	char *const float_arg = umalloc(nargs);
	const out_val **local_args = NULL;
	const out_val *retval_stret;
	const out_val *stret_spill;

	const struct vreg *call_iregs;
	unsigned n_call_iregs;

	struct
	{
		v_stackt bytesz;
		const out_val *vptr;
		enum
		{
			CLEANUP_NO,
			CLEANUP_RESTORE_PUSHBACK = 1 << 0,
			CLEANUP_POP = 1 << 1
		} need_cleanup;
	} arg_stack = { 0 };

	unsigned nfloats = 0, nints = 0;
	unsigned i;
	enum stret stret_kind;
	unsigned stret_stack = 0;

	/* must generate a prologue/epilogue if we aren't a leaf function,
	 * since the stack must be aligned correctly for a call
	 * (i.e. at least a pushq %rbp to bring it up to 16) */
	octx->used_stack = 1;
	octx->had_call = 1;

	dynarray_add_array(&local_args, args);

	impl_func_call_regs(fnty, &n_call_iregs, &call_iregs);

	/* pre-scan of arguments - eliminate flags
	 * (should only be one, since we can only have one flag at a time)
	 *
	 * also count floats and ints
	 */
	for(i = 0; i < nargs; i++){
		assert(local_args[i]->retains > 0);

		if(local_args[i]->type == V_FLAG)
			local_args[i] = v_to_reg(octx, local_args[i]);

		float_arg[i] = type_is_floating(v_get_type(local_args[i]));

		if(float_arg[i])
			nfloats++;
		else
			nints++;
	}

	/* hidden stret argument */
	switch((stret_kind = impl_func_stret(retty))){
		case stret_memcpy:
			nints++; /* only an extra pointer arg for stret_memcpy */
			/* fall */
		case stret_regs:
			/* We unconditionally want to spill rdx:rax to the stack on return.
			 * This could be optimised in the future
			 * (in a similar vein as long long on x86/32-bit)
			 * so that we can handle vtops with structure/union type
			 * and multiple registers.
			 *
			 * Hence, space needed for both reg and memcpy returns
			 */
			stret_stack = type_size(retty, NULL);
			stret_spill = out_aalloc(octx, stret_stack, type_align(retty, NULL), retty, NULL);

		case stret_scalar:
			break;
	}

	/* do we need to do any stacking? */
	if(nints > n_call_iregs)
		arg_stack.bytesz += pws * (nints - n_call_iregs);

	if(nfloats > N_CALL_REGS_F)
		arg_stack.bytesz += pws * (nfloats - N_CALL_REGS_F);

	/* need to save regs before pushes/call */
	v_save_regs(octx, fnty, local_args, fn);

	impl_func_alignstack(octx);

	if(arg_stack.bytesz > 0){
		out_comment(octx, "stack space for %lu arguments",
				arg_stack.bytesz / pws);

		if(impl_func_caller_cleanup(fnty))
			arg_stack.need_cleanup = CLEANUP_NO;
		else
			arg_stack.need_cleanup |= CLEANUP_RESTORE_PUSHBACK;

		/* see comment about stack_iter below */
		if(octx->alloca_count){
			arg_stack.bytesz = pack_to_align(arg_stack.bytesz, octx->max_align);

			v_stack_adj(octx, arg_stack.bytesz, /*sub:*/1);
			arg_stack.vptr = NULL;

			arg_stack.need_cleanup |= CLEANUP_POP;

		}else{
			/* this aligns the stack-ptr and returns arg_stack padded */
			arg_stack.vptr = out_aalloc(octx, arg_stack.bytesz, pws, arithty, NULL);

			if(octx->stack_callspace < arg_stack.bytesz)
				octx->stack_callspace = arg_stack.bytesz;
		}
	}

	if(arg_stack.bytesz > 0){
		const out_val *stack_iter;

		nints = nfloats = 0;

		/* Rather than spilling the registers based on %rbp, we spill
		 * them based as offsets from %rsp, that way they're always
		 * at the bottom of the stack, regardless of future changes
		 * to octx->cur_stack_sz
		 *
		 * VLAs (and alloca()) unfortunately break this.
		 * For this we special case and don't reuse existing stack.
		 * Instead, we allocate stack explicitly, use it for the call,
		 * then free it (done above).
		 */
		stack_iter = v_new_sp(octx, NULL);

		/* save in order */
		for(i = 0; i < nargs; i++){
			const int stack_this = float_arg[i]
				? nfloats++ >= N_CALL_REGS_F
				: nints++ >= n_call_iregs;

			if(stack_this){
				type *storety = type_ptr_to(local_args[i]->t);

				assert(stack_iter->retains > 0);

				stack_iter = out_change_type(octx, stack_iter, storety);
				out_val_retain(octx, stack_iter);
				local_args[i] = v_to_stack_mem(octx, local_args[i], stack_iter, V_REGOFF);

				assert(local_args[i]->retains > 0);

				stack_iter = out_op(octx, op_plus,
						out_change_type(octx, stack_iter, arithty),
						out_new_l(octx, arithty, pws));
			}
		}

		out_val_release(octx, stack_iter);
	}

	/* must be set before stret pointer argument */
	nints = nfloats = 0;

	/* setup hidden stret pointer argument */
	if(stret_kind == stret_memcpy){
		const struct vreg *stret_reg = &call_iregs[nints];
		nints++;

		if(cc1_fopt.verbose_asm){
			out_comment(octx, "stret spill space '%s' @ %s, %u bytes",
					type_to_str(stret_spill->t),
					out_val_str(stret_spill, 1),
					stret_stack);
		}

		out_val_retain(octx, stret_spill);
		v_freeup_reg(octx, stret_reg);
		out_flush_volatile(octx, v_to_reg_given(octx, stret_spill, stret_reg));
	}

	for(i = 0; i < nargs; i++){
		const out_val *const vp = local_args[i];
		/* we use float_arg[i] since vp->t may now be float *,
		 * if it's been spilt */
		const int is_float = float_arg[i];

		const struct vreg *rp = NULL;
		struct vreg r;

		if(is_float){
			assert(0 && "TODO: float merge");
			if(nfloats < N_CALL_REGS_F){
				assert(0 && "TODO");
#if 0
				/* NOTE: don't need to use call_regs_float,
				 * since it's xmm0 ... 7 */
				r.idx = nfloats;
				r.is_float = 1;

				rp = &r;
#else
				(void)r;
#endif
			}
			nfloats++;

		}else{
			/* integral */
			if(nints < n_call_iregs)
				rp = &call_iregs[nints];

			nints++;
		}

		if(rp){
			/* only bother if it's not already in the register */
			if(vp->type != V_REG || !vreg_eq(rp, &vp->bits.regoff.reg)){
				/* need to free it up, as v_to_reg_given doesn't clobber check */
				v_freeup_reg(octx, rp);

#if 0
				/* argument retainedness doesn't matter here -
				 * local arguments and autos are held onto, and
				 * inline functions hold onto their arguments one extra
				 */
				UCC_ASSERT(local_args[i]->retains == 1,
						"incorrectly retained arg %d: %d",
						i, local_args[i]->retains);
#endif


				local_args[i] = v_to_reg_given(octx, local_args[i], rp);
			}
			if(local_args[i]->type == V_REG && local_args[i]->bits.regoff.offset){
				/* need to ensure offsets are flushed */
				local_args[i] = v_reg_apply_offset(octx, local_args[i]);
			}
		}
		/* else already pushed */
	}

	impl_func_call(octx, fnty, nfloats, fn, args);

	if(arg_stack.bytesz){
		if(arg_stack.need_cleanup & CLEANUP_RESTORE_PUSHBACK){
			/* callee cleanup - the callee will have popped
			 * args from the stack, so we need a stack alloc
			 * to restore what we expect */

			if(arg_stack.need_cleanup & CLEANUP_POP){
				/* we wanted to pop anyway - callee did it for us */
			}else{
				v_stack_adj(octx, arg_stack.bytesz, /*sub:*/1);
			}

		}else if(arg_stack.need_cleanup & CLEANUP_POP){
			/* caller cleanup - we reuse the stack for other purposes */
				v_stack_adj(octx, arg_stack.bytesz, /*sub:*/0);
		}

		if(arg_stack.vptr)
			out_adealloc(octx, &arg_stack.vptr);
	}

	for(i = 0; i < nargs; i++)
		out_val_consume(octx, local_args[i]);
	dynarray_free(const out_val **, local_args, NULL);

	if(stret_kind != stret_scalar){
		if(stret_kind == stret_regs){
			/* we behave the same as stret_memcpy(),
			 * but we must spill the regs out */
			struct vreg regpair[2];
			int nregs;

			impl_func_overlay_regpair(regpair, &nregs, retty);

			retval_stret = out_val_retain(octx, stret_spill);
			out_val_retain(octx, retval_stret);

			/* spill from registers to the stack */
			impl_overlay_regs2mem(octx, stret_stack, nregs, regpair, retval_stret);
		}

		assert(stret_spill);
		out_val_release(octx, stret_spill);
	}

	/* return type */
	free(float_arg);

	if(stret_kind != stret_regs){
		/* rax / xmm0, otherwise the return has
		 * been set to a local stack address */
		const int fp = type_is_floating(retty);
		struct vreg rr = VREG_INIT(fp ? REG_RET_F_1 : REG_RET_I_1, fp);

		return v_new_reg(octx, fn, retty, &rr);
	}else{
		out_val_consume(octx, fn);
		return retval_stret;
	}
}

static ucc_wur const out_val *
impl_func_ret_memcpy(
		out_ctx *octx,
		struct vreg *ret_reg,
		type *called,
		const out_val *from)
{
	const out_val *stret_p;

	/* copy from *%rax to *%rdi (first argument) */
	out_comment(octx, "stret copy");

	stret_p = octx->current_stret;

	out_val_retain(octx, stret_p);
	stret_p = out_deref(octx, stret_p);

	out_flush_volatile(octx,
			out_memcpy(octx, stret_p, from, type_size(called, NULL)));

	/* return the stret pointer argument */
	ret_reg->is_float = 0;
	ret_reg->idx = REG_RET_I_1;

	return out_deref(octx, out_val_retain(octx, octx->current_stret));
}

static void impl_func_ret_regs(
		out_ctx *octx, type *called, const out_val *from)
{
	const unsigned sz = type_size(called, NULL);
	struct vreg regs[2];
	int nregs;

	impl_func_overlay_regpair(regs, &nregs, called);

	/* read from the stack to registers */
	impl_overlay_mem2regs(octx, sz, nregs, regs, from);
}

void impl_to_retreg(out_ctx *octx, const out_val *val, type *called)
{
	struct vreg r;

	switch(impl_func_stret(called)){
		case stret_memcpy:
			/* this function is responsible for memcpy()ing
			 * the struct back */
			val = impl_func_ret_memcpy(octx, &r, called, val);
			break;

		case stret_scalar:
			r.is_float = type_is_floating(called);
			r.idx = r.is_float ? REG_RET_F_1 : REG_RET_I_1;
			break;

		case stret_regs:
			/* this function returns in regs, the caller is
			 * responsible doing what it wants,
			 * i.e. memcpy()ing the regs into stack space */
			impl_func_ret_regs(octx, called, val);
			return;
	}

	/* not done for stret_regs: */

	/* v_to_reg since we don't handle lea/load ourselves */
	out_flush_volatile(octx, v_to_reg_given(octx, val, &r));
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

	stack_locn = out_aalloc(octx, stack_n, /*align*/voidpsz, voidp, NULL);

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

static const out_val *emit_stack_probe_call(out_ctx *octx, const char *probefn, v_stackt adj)
{
	type *intptr_ty = type_nav_btype(cc1_type_nav, type_intptr_t);
	funcargs *fargs = funcargs_new();
	type *fnty_noptr;
	type *fnty_ptr;
	char *mangled;
	const out_val *fn, *args[2], *newadj;

	fargs->args_void = 1;
	fnty_noptr = type_func_of(intptr_ty, fargs, NULL);
	fnty_ptr = type_ptr_to(fnty_noptr);
	mangled = func_mangle(probefn, fnty_noptr);

	fn = out_new_lbl(octx, fnty_ptr, mangled, /* another module */OUT_LBL_PIC);
	args[0] = out_new_l(octx, intptr_ty, adj);
	args[1] = NULL;

	newadj = out_call(octx, fn, args, fnty_ptr);

	if(mangled != probefn)
		free(mangled);

	return newadj;
}

static void allocate_stack(out_ctx *octx, v_stackt adj, type *fnty)
{
	impl_regs_reserve_args(octx, fnty);

	if(adj >= 4096){
		const enum sys sys = platform_sys();

		switch(sys){
			case SYS_linux:
			case SYS_freebsd:
			case SYS_darwin:
				break;
			case SYS_cygwin:
			{
				// Emit stack probes for windows/cygwin/mingw targets.
				// Windows => __chkstk
				// cygwin/mingw => __alloca
				const char *probefn;
				int still_need_stack_adj;
				const out_val *newadj;

				if(platform_32bit()){
					probefn = "_alloca"; /* cygwin and mingw */
					still_need_stack_adj = 0;

					/* probefn = "_chkstk"; // windows
					 * still_need_stack_adj = 0;
					 */
				}else{
					probefn = "__chkstk_ms"; /* cygwin and mingw */
					still_need_stack_adj = 1;

					/* probefn = "_chkstk"; // windows
					 * still_need_stack_adj = 1;
					 */
				}

				newadj = emit_stack_probe_call(octx, probefn, adj);

				if(still_need_stack_adj){
					v_stack_adj_val(octx, newadj, /*sub:*/1);
				}else{
					out_comment(octx, "stack adjusted by %s call", probefn);
					out_val_release(octx, newadj);
				}
				return;
			}
		}
	}

	v_stack_adj(octx, adj, /*sub:*/1);

	impl_regs_unreserve_args(octx, fnty);
}

void out_func_epilogue(
	out_ctx *octx,
	type *ty,
	const where *func_begin,
	char *end_dbg_lbl,
	const struct section *section)
{
	int clean_stack, redzone = 0;
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

	if(cc1_profileg && mopt_mode & MOPT_FENTRY){ /* stack frame required */
		clean_stack = 1;

	}else if(!octx->used_stack){ /* stack unused - try to omit frame pointer */
		clean_stack = !cc1_fopt.omit_frame_pointer;

	}else{ /* stack used - can we red-zone it? */
#ifdef REDZONE_BYTES
		redzone =
			(mopt_mode & MOPT_RED_ZONE) &&
			octx->max_stack_sz < REDZONE_BYTES &&
			!octx->had_call &&
			!octx->stack_ptr_manipulated;
#else
		redzone = 0;
#endif

		clean_stack = 1;
	}

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

		out_current_blk(octx, octx->stacksub_blk);
		{
			v_stackt stack_adj;

			/* must have more or equal stack to the alloc_n, because alloc_n will
			 * always add to {var,max}_stack_sz with possible padding,
			 * and that same value (minus padding) to stack_n_alloc */
			assert(octx->max_stack_sz >= octx->stack_n_alloc);

			if(cc1_fopt.verbose_asm){
				out_comment(octx,
						"stack_sz{cur=%lu,max=%lu} n_alloc=%lu call_spc=%lu calleesve=%lu max_align=%u",
						octx->cur_stack_sz,
						octx->max_stack_sz,
						octx->stack_n_alloc,
						octx->stack_callspace,
						octx->stack_calleesave_space,
						octx->max_align);

				out_comment(octx,
						"red-zone %s (stack @ %lu bytes, had_call: %d, stack_ptr_manipulated: %d)",
						redzone ? "active" : "inactive",
						octx->max_stack_sz,
						octx->had_call,
						octx->stack_ptr_manipulated);
			}

			if(octx->max_align){
				/* must align max_stack_sz,
				 * not the resultant after subtracting stack_n_alloc */
				octx->max_stack_sz = pack_to_align(octx->max_stack_sz, octx->max_align);
			}
			stack_adj = octx->max_stack_sz - octx->stack_n_alloc;

			if(!redzone)
				allocate_stack(octx, stack_adj, ty);
		}

		/* entry_blk -> stacksub_blk -> argspill_begin_blk -> [implicitly] argspill_done_blk */
		out_current_blk(octx, octx->entry_blk);
		out_ctrl_transfer_make_current(octx, octx->stacksub_blk);
		/* stacksub code written */
		out_ctrl_transfer(octx, octx->argspill_begin_blk, NULL, NULL, 0);
		/* argspill code written */
		out_current_blk(octx, octx->argspill_done_blk);

		if(call_save_spill_blk)
			out_ctrl_transfer_make_current(octx, call_save_spill_blk);

		out_ctrl_transfer(octx, octx->postprologue_blk, NULL, NULL, 0);
	}else{
		flush_root = octx->postprologue_blk;

		/* need to attach the label to prologue_postjoin_blk */
		blk_transfer(octx->entry_blk, flush_root);
	}
	octx->current_blk = NULL;

	blk_flushall(octx, flush_root, end_dbg_lbl, section);

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

void out_perfunc_init(out_ctx *octx, decl *fndecl, const char *sp)
{
	attribute *attr;
	type *const fnty = fndecl->ref;
	attribute *alignment;

	octx->current_fnty = fnty;

	assert(octx->cur_stack_sz == 0 && "non-empty stack for new func");
	assert(octx->alloca_count == 0 && "allocas left over?");
	octx->check_flags = 1;
	octx->had_call = 0;
	octx->stack_ptr_manipulated = 0;

	assert(!octx->current_blk);
	octx->entry_blk = out_blk_new_lbl(octx, sp);
	octx->stacksub_blk = out_blk_new(octx, "stacksub");
	octx->argspill_begin_blk = out_blk_new(octx, "argspill");
	octx->postprologue_blk = out_blk_new(octx, "post_prologue");
	octx->epilogue_blk = out_blk_new(octx, "epilogue");

	attr = attribute_present(fndecl, attr_no_sanitize);
	octx->no_sanitize_flags = attr ? attr->bits.no_sanitize : 0;

	if((alignment = attribute_present(fndecl, attr_aligned)))
		octx->entry_blk->align = const_fold_val_i(alignment->bits.align);
}

static void prologue_save_call_regs(out_ctx *octx, int nargs, const out_val *argvals[])
{
	type *fnty = octx->current_fnty;
	int is_stret = 0;
	const out_val *stret_ptr = NULL;

	/* save the stret hidden argument */
	switch(impl_func_stret(type_func_call(fnty, NULL))){
		case stret_regs:
		case stret_scalar:
			break;
		case stret_memcpy:
			nargs++;
			is_stret = 1;
	}

	impl_func_prologue_save_call_regs(
		octx,
		fnty,
		nargs,
		argvals,
		is_stret ? &stret_ptr : NULL);

	if(is_stret){
		octx->current_stret = stret_ptr;

		if(cc1_fopt.verbose_asm){
			const out_val *stret = octx->current_stret;

			out_comment(octx, "stret pointer '%s' @ %s",
					type_to_str(stret->t), out_val_str(stret, 1));
		}
	}
}

void out_func_prologue(
		out_ctx *octx,
		int nargs, int variadic, int stack_protector,
		const out_val *argvals[])
{
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
			prologue_save_call_regs(octx, nargs, argvals);

			if(variadic) /* save variadic call registers */
				impl_func_prologue_save_variadic(octx, octx->current_fnty);

			octx->argspill_done_blk = octx->current_blk;

			/* setup "pointers" to the right place in the stack */
			octx->stack_variadic_offset = octx->cur_stack_sz;
			octx->initial_stack_sz = octx->cur_stack_sz;

			if(stack_prot_slot){
				/* now all registers are safe, init stack protector */
				out_init_stack_canary(octx, stack_prot_slot);
			}
		}

		octx->used_stack = !!stack_prot_slot;
	}
	octx->in_prologue = 0;

	out_current_blk(octx, octx->postprologue_blk);
}

unsigned out_current_stack(out_ctx *octx)
{
	return octx->max_stack_sz;
}
