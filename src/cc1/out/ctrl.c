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

	out_current_blk(octx, if_true);
}

void out_ctrl_end_ret(out_ctx *octx, out_val *ret, type *ty)
{
	impl_return(octx, ret, ty);
	octx->current_blk = NULL;
}

void out_ctrl_end_undefined(out_ctx *octx)
{
	impl_undefined(octx);
	octx->current_blk = NULL;
}

out_val *out_ctrl_merge(out_ctx *octx, out_blk *from_a, out_blk *from_b)
{
	/* maybe ret null */
	(void)octx;
	(void)from_a;
	(void)from_b;
	return NULL;
}

void out_current_blk(out_ctx *octx, out_blk *new_blk)
{
	out_blk *cur = octx->current_blk;

	if(new_blk == cur)
		return;

	register_block(octx, new_blk);

	if(cur && cur->next.type == BLK_NEXT_NONE){
		/* implicit continue to next block */
		assert(!cur->next.bits.blk);

		cur->next.type = BLK_NEXT_BLOCK;
		cur->next.bits.blk = new_blk;
	}

	octx->current_blk = new_blk;
}

void out_ctrl_transfer(out_ctx *octx, out_blk *to,
		out_val *phi_arg /* optional */)
{
	out_blk *const from = octx->current_blk;

	from->phi_val = phi_arg;

	out_current_blk(octx, to);

	if(!phi_arg && from->next.type == BLK_NEXT_BLOCK){
		/* it's a scope return, e.g.
		 * if(cond){
		 *     <BB1>;
		 * }else{
		 *     <BB2>;
		 * }
		 * <BB3>; // ctrl transfer to here
		 */
		from->next.type = BLK_NEXT_BLOCK_UPSCOPE;
	}
}

void out_ctrl_transfer_exp(out_ctx *octx, out_val *addr)
{
	out_blk *cur = octx->current_blk;
	octx->current_blk = NULL;

	cur->next.type = BLK_NEXT_EXPR;
	cur->next.bits.exp = addr;
}
