#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include "forwards.h"
#include "blk.h"

#include "out.h" /* this file defs */
#include "asm.h"
#include "val.h"
#include "impl.h"
#include "ctx.h"
#include "virt.h"

static void register_block(out_ctx *octx, out_blk *blk)
{
	blk->octx = octx;
}

void out_ctrl_branch(
		out_ctx *octx,
		out_val *cond,
		out_blk *if_true, out_blk *if_false)
{
	register_block(octx, if_true);
	register_block(octx, if_false);

	impl_branch(octx, cond, if_true, if_false);
	out_val_consume(octx, cond);

	out_current_blk(octx, if_true);
}

void out_ctrl_end_ret(out_ctx *octx, out_val *ret, type *ty)
{
	if(ret)
		impl_to_retreg(octx, ret, ty);
	out_ctrl_transfer(octx, octx->epilogue_blk, NULL);
	octx->current_blk = NULL;
}

void out_ctrl_end_undefined(out_ctx *octx)
{
	impl_undefined(octx);
	octx->current_blk = NULL;
}

out_val *out_ctrl_merge(out_ctx *octx, out_blk *from_a, out_blk *from_b)
{
	/* here we set merge_preds */
	out_blk *const saved_current_blk = octx->current_blk;
	type *ty;
	struct vreg merge_reg;

	assert(from_a->phi_val && from_b->phi_val);

	ty = from_a->phi_val->t;

	/* need them both in a register */
	out_current_blk(octx, from_a);
	{
		out_val *regged = v_to_reg_out(octx, from_a->phi_val, &merge_reg);
		out_flush_volatile(octx, regged);
		out_comment(octx, "FLUSH TO REG %d", merge_reg.idx);
	}
	out_current_blk(octx, from_b);
	{
		out_val *regged = v_to_reg_given(octx, from_b->phi_val, &merge_reg);
		out_flush_volatile(octx, regged);
	}

	out_current_blk(octx, saved_current_blk);

	return v_new_reg(octx, NULL, ty, &merge_reg);
}

void out_current_blk(out_ctx *octx, out_blk *new_blk)
{
	out_blk *cur = octx->current_blk;

	octx->last_used_blk = new_blk;

	if(new_blk == cur)
		return;

	register_block(octx, new_blk);

	octx->current_blk = new_blk;
}

void out_ctrl_transfer(out_ctx *octx, out_blk *to,
		out_val *phi /* optional */)
{
	out_blk *const from = octx->current_blk;

	if(!from){
		/* we're transferring from unreachable code, ignore */
		if(phi)
			out_val_consume(octx, phi);
		return;
	}

	from->phi_val = phi;

	to->pred_count++;

	assert(from->type == BLK_UNINIT);
	from->type = BLK_NEXT_BLOCK;
	from->bits.next = to;

	octx->current_blk = NULL;
}

void out_ctrl_transfer_exp(out_ctx *octx, out_val *addr)
{
	out_blk *cur = octx->current_blk;
	octx->current_blk = NULL;

	cur->type = BLK_NEXT_EXPR;
	cur->bits.exp = addr;
}
