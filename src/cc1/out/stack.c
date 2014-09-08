#include <stddef.h>

#include "../type.h"
#include "../type_nav.h"
#include "../pack.h"

#include "macros.h"

#include "forwards.h"

#include "val.h"
#include "ctx.h"
#include "blk.h"

#include "out.h"

#include "stack.h"

void v_stack_adj(out_ctx *octx, v_stackt amt, int sub)
{
	out_flush_volatile(
			octx,
			out_op(
				octx, sub ? op_minus : op_plus,
				v_new_sp(octx, NULL),
				out_new_l(
					octx,
					type_nav_btype(cc1_type_nav, type_intptr_t),
					amt)));
}

#if 0
static void octx_set_stack_sz(out_ctx *octx, unsigned new)
{
	octx->cur_stack_sz = new;

	if(octx->cur_stack_sz > octx->max_stack_sz)
		octx->max_stack_sz = octx->var_stack_sz;
}

unsigned v_alloc_stack2(
		out_ctx *octx,
		const unsigned sz_initial, int noop, const char *desc)
{
	unsigned sz_rounded = sz_initial;

	if(sz_initial){
		/* must be a multiple of mstack_align.
		 * assume stack_sz is aligned, and just
		 * align what we add to it
		 */
		sz_rounded = pack_to_align(sz_initial, cc1_mstack_align);

		/* if it changed, we need to realign the stack */
		if(!noop || sz_rounded != sz_initial){
			if(fopt_mode & FOPT_VERBOSE_ASM){
				out_comment(octx, "stack alignment for %s (%u -> %u)",
						desc, octx->var_stack_sz, octx->var_stack_sz + sz_rounded);
				out_comment(octx, "alloc_n by %u (-> %u), padding with %u",
						sz_initial, octx->var_stack_sz + sz_initial,
						sz_rounded - sz_initial);
			}

			/* no actual stack adjustments here - done purely in prologue */
		}

		octx_set_stack_sz(octx, octx->var_stack_sz + sz_rounded);

		if(noop)
			octx->stack_n_alloc += sz_initial;
	}

	return sz_rounded;
}

unsigned v_alloc_stack_n(out_ctx *octx, unsigned sz, const char *desc)
{
	return v_alloc_stack2(octx, sz, 1, desc);
}

unsigned v_alloc_stack(out_ctx *octx, unsigned sz, const char *desc)
{
	return v_alloc_stack2(octx, sz, 0, desc);
}

unsigned v_stack_align(out_ctx *octx, unsigned const align, int force_mask)
{
	if(force_mask || (octx->var_stack_sz & (align - 1))){
		type *const ty = type_nav_btype(cc1_type_nav, type_intptr_t);
		const unsigned new_sz = pack_to_align(octx->var_stack_sz, align);
		unsigned added = new_sz - octx->var_stack_sz;
		const out_val *sp = v_new_sp(octx, NULL);

		assert(sp->retains == 1);

		if(force_mask && added == 0)
			added = align;

		sp = out_op(
				octx, op_minus,
				sp,
				out_new_l(octx, ty, added));

		octx_set_stack_sz(octx, new_sz);

		if(force_mask){
			sp = out_op(octx, op_and, sp, out_new_l(octx, ty, align - 1));
		}
		out_val_release(octx, sp);
		assert(sp->retains == 0);
		out_comment(octx, "stack aligned to %u bytes", align);
		return added;
	}
	return 0;
}

void v_need_stackalign(out_ctx *octx, unsigned align)
{
	/* aligning the stack isn't sufficient here - if the stack is adjusted after,
	 * it might not be at a 16-byte alignment */
	octx->max_align = MAX(octx->max_align, align);
}
#endif

const out_val *out_aalloc(out_ctx *octx, unsigned sz, unsigned align)
{
	/* align greater than size - we increase
	 * size so it can be aligned to 'align' */
	if(align > sz)
		sz = pack_to_align(sz, align);

	/* packing takes care of everything */
	pack_next(&octx->cur_stack_sz, NULL, sz, align);
}
