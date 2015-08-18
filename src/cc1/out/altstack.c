#include <stddef.h>
#include <assert.h>

#include "../type.h"

#include "val.h"
#include "blk.h"
#include "ctx.h"
#include "backend.h"
#include "out.h"
#include "virt.h"

#include "altstack.h"

struct reserve_ctx
{
	out_ctx *octx;
	int success;
};

static void iter_altstack(out_ctx *octx, int fn(out_val *, void *), void *ctx)
{
	struct out_val_list *vi;

	for(vi = octx->val_head; vi; vi = vi->next){
		out_val *v = &vi->val;
		if(v->retains == 0)
			continue;
		if(v->type != V_ALTSTACK)
			continue;

		if(fn(v, ctx))
			break;
	}
}

static int inc_dec(out_val *val, void *vctx)
{
	int *pdiff = vctx;
	val->bits.altstack.pos += *pdiff;

	return 0;
}

static void v_altstack_alter_all(out_ctx *octx, int diff)
{
	iter_altstack(octx, inc_dec, &diff);

	octx->altstackcount += diff;
}

void v_altstack_pop_all(out_ctx *octx)
{
	v_altstack_alter_all(octx, -1);
}

void v_altstack_push_all(out_ctx *octx)
{
	v_altstack_alter_all(octx, +1);
}

#include <stdio.h>
static int reserve(out_val *val, void *vctx)
{
	struct reserve_ctx *ctx = vctx;
	const out_val *saved;
	out_ctx *octx = ctx->octx;

	assert(val->type == V_ALTSTACK);

	if(val->bits.altstack.locked)
		return 0; /* keep going */

	/* found one - evict it */
	out_comment(octx, "spilling altstack");

	fprintf(stderr, "spilling altstack, val retains %d\n", val->retains);

	saved = v_save_reg(octx, val, NULL);

	fprintf(stderr, "          spilt:   val retains %d, saved %d\n",
			val->retains, saved->retains);

	out_val_overwrite(octx, val, saved);
	out_val_consume(octx, saved);

	fprintf(stderr, "       overwritten: val retains %d, saved %d\n",
			val->retains, saved->retains);

	ctx->success = 1;

	return 1;
}

void v_altstack_reserve(out_ctx *octx)
{
	struct reserve_ctx ctx = { 0 };

	if(octx->altstackcount + 1 < N_ALTSTACK)
		return;

	ctx.octx = octx;

	iter_altstack(octx, reserve, &ctx);

	if(!ctx.success){
		assert(0 && "couldn't spill altstack for reserve");
	}
}

static void v_altstack_lock_unlock(const out_val *v, int lock)
{
	assert(v->type == V_ALTSTACK);
	REMOVE_CONST(out_val *, v)->bits.altstack.locked = lock;
}

void v_altstack_lock(const out_val *v)
{
	v_altstack_lock_unlock(v, 1);
}

void v_altstack_unlock(const out_val *v)
{
	v_altstack_lock_unlock(v, 0);
}
