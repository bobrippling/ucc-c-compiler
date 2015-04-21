#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include "../../util/dynarray.h"

#include "forwards.h"
#include "blk.h"

#include "out.h" /* this file defs */
#include "asm.h"
#include "val.h"
#include "impl.h"
#include "ctx.h"
#include "virt.h"

static void v_transfer_spill(out_ctx *octx, const out_val *skip)
{
	/* all active regs must be spilt here since we don't know if they'll be spilt
	 * (or moved to callee-save) in one of the branches we're about to jump into,
	 * and if they are spilt, they might not be spilt in the other branch,
	 * leaving the other branch's code path undefined
	 */

	v_save_regs(octx, NULL, NULL, skip);
}

void out_ctrl_branch(
		out_ctx *octx,
		const out_val *cond,
		out_blk *if_true, out_blk *if_false)
{
	v_decay_flags_except1(octx, cond);
	v_transfer_spill(octx, cond);

	impl_branch(octx,
			cond, if_true, if_false,
			!!(cond->flags & VAL_FLAG_LIKELY));

	out_current_blk(octx, if_true);
}

void out_ctrl_end_ret(out_ctx *octx, const out_val *ret, type *ty)
{
	if(ret)
		impl_to_retreg(octx, ret, ty);
	out_ctrl_transfer(octx, octx->epilogue_blk, NULL, NULL);
	octx->current_blk = NULL;
}

void out_ctrl_end_undefined(out_ctx *octx)
{
	impl_undefined(octx);
	octx->current_blk = NULL;
}

static out_val *unphi(out_val *phival)
{
	phival->flags &= ~VAL_IS_PHI;
	return phival;
}

const out_val *out_ctrl_merge_n(out_ctx *octx, out_blk **rets)
{
	out_blk *const saved_current_blk = octx->current_blk;
	struct vreg merge_reg;
	int merge_reg_set = 0;
	out_blk **blk_iter;
	struct
	{
		type *ty;
		unsigned sz;
	} max = { NULL, 0 };

	/* optimisation: if we only have one (e.g. single inline ret),
	 * then we can just returns its phi */
	if(!rets[1]){
		/* we only need to use unphi() here (as opposed to for unphi'ing all the
		 * phi vals), because this is the only escapable phi val */
		return unphi(rets[0]->phi_val);
	}

	/* get the largest size */
	for(blk_iter = rets; *blk_iter; blk_iter++){
		const out_val *phi = (*blk_iter)->phi_val;
		unsigned this_sz;

		assert(phi);

		this_sz = type_size(phi->t, NULL);
		if(this_sz > max.sz){
			max.sz = this_sz;
			max.ty = phi->t;
		}
	}

	/* need them all in registers */
	for(blk_iter = rets; *blk_iter; blk_iter++){
		out_blk *const blk = *blk_iter;
		const out_val *regged;

		out_current_blk(octx, blk);

		if(!merge_reg_set){
			/* apply_offset - prevent merge_reg being a base pointer */
			regged =
				v_reg_apply_offset(octx,
						v_to_reg_out(
							octx,
							blk->phi_val,
							&merge_reg));

			/* apply_offset may move to another reg */
			memcpy_safe(&merge_reg, &regged->bits.regoff.reg);
			merge_reg_set = 1;
		}else{
			regged = v_to_reg_given(octx, blk->phi_val, &merge_reg);
		}

		/* if we have mismatching sizes, we need to cast one side up,
		 * so that we fill out all parts of the smaller size'd register,
		 */
		if(type_size(regged->t, NULL) < max.sz)
			regged = out_cast(octx, regged, max.ty, 0);


		out_flush_volatile(octx, regged);
	}

	out_current_blk(octx, saved_current_blk);

	return v_new_reg(octx, NULL, max.ty, &merge_reg);
}

const out_val *out_ctrl_merge(out_ctx *octx, out_blk *from_a, out_blk *from_b)
{
	out_blk *rets[] = { from_a, from_b, NULL };
	return out_ctrl_merge_n(octx, rets);
}

void out_current_blk(out_ctx *octx, out_blk *new_blk)
{
	assert(new_blk);
	v_decay_flags(octx);

	octx->last_used_blk = new_blk;

	octx->current_blk = new_blk;
}

void out_ctrl_transfer(out_ctx *octx, out_blk *to,
		const out_val *phi /* optional */, out_blk **mergee)
{
	out_blk *from = octx->current_blk;

	v_decay_flags_except1(octx, phi);

	v_transfer_spill(octx, phi);

	assert(!!phi == !!mergee);

	if(mergee)
		*mergee = from;

	/* always add a merge_pred - this means we can generate
	 * terminating branches before their merge block, even though
	 * they don't merge to it (instead of generating them after ret)
	 */
	assert(octx->last_used_blk);
	dynarray_add(&to->merge_preds, from ? from : octx->last_used_blk);

	if(!from){
		/* we're transferring from unreachable code, ignore */
		if(phi)
			out_val_consume(octx, phi);
		return;
	}

	assert(!from->phi_val);
	if(phi){
		/* we're keeping a hold of phi, and setting VAL_IS_PHI on it,
		 * so we need our own modifiable copy */
		out_val *phi_mut = v_dup_or_reuse(octx, phi, phi->t);
		phi_mut->flags |= VAL_IS_PHI;

		from->phi_val = phi_mut;
	}else{
		from->phi_val = NULL;
	}

	assert(from->type == BLK_UNINIT);
	from->type = BLK_NEXT_BLOCK;
	from->bits.next = to;

	octx->current_blk = NULL;
}

void out_ctrl_transfer_make_current(out_ctx *octx, out_blk *to)
{
	out_ctrl_transfer(octx, to, NULL, NULL);
	out_current_blk(octx, to);
}

void out_ctrl_transfer_exp(out_ctx *octx, const out_val *addr)
{
	assert(addr->retains == 1); /* don't want this changing under us */

	v_decay_flags_except1(octx, addr);

	v_transfer_spill(octx, addr);

	impl_jmp_expr(octx, addr); /* must jump now, while we have octx */

	if(octx->current_blk){
		octx->current_blk->type = BLK_NEXT_EXPR;
		octx->current_blk->bits.exp = addr;
	}

	octx->current_blk = NULL;
}
