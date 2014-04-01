#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../../util/alloc.h"

#include "../type.h"
#include "../type_nav.h"
#include "../type_is.h"
#include "../pack.h"

#include "../cc1.h" /* cc1_mstack_align */

#include "val.h"
#include "out.h"
#include "virt.h"
#include "ctx.h"
#include "asm.h"
#include "impl.h"

#define TODO() \
	fprintf(stderr, "%s:%d: TODO: %s", __FILE__, __LINE__, __func__)

out_val *v_to_stack_mem(out_ctx *octx, out_val *vp, long stack_pos)
{
	out_val *store = v_new_sp3(octx, vp, vp->t, stack_pos);

	out_val_retain(octx, store);

	vp = v_to(octx, vp, TO_CONST | TO_REG);

	/* the following gen two instructions - subq and movq
	 * instead/TODO: impl_save_reg(vp) -> "pushq %%rax"
	 * -O1?
	 */
	out_store(octx, store, vp);

	out_val_release(octx, store);

	return store;
}

static int v_in(out_val *vp, enum vto to)
{
	switch(vp->type){
		case V_FLAG:
			break;

		case V_CONST_I:
		case V_CONST_F:
			return !!(to & TO_CONST);

		case V_REG:
			return 0; /* needs further checks */

		case V_REG_SAVE:
		case V_LBL:
			return !!(to & TO_MEM);
	}

	return 0;
}

static out_val *v_save_reg(out_ctx *octx, out_val *vp)
{
	assert(vp->type == V_REG && "not reg");

	out_comment("register spill:");

	v_alloc_stack(octx, type_size(vp->t, NULL), "save reg");

	return v_to_stack_mem(
			octx,
			vp,
			-octx->stack_sz);
}

out_val *v_to(out_ctx *octx, out_val *vp, enum vto loc)
{
	if(v_in(vp, loc))
		return vp;

	/* TO_CONST can't be done - it should already be const,
	 * or another option should be chosen */

	/* go for register first */
	if(loc & TO_REG){
		return v_to_reg(octx, vp);
	}

	if(loc & TO_MEM){
		return v_save_reg(octx, v_to_reg(octx, vp));
	}

	assert(0 && "can't satisfy v_to");
}

static out_val *v_freeup_regp(out_ctx *octx, out_val *vp)
{
	/* freeup this reg */
	struct vreg r;
	out_val *free_reg;

	assert(vp->type == V_REG && "not reg");

	/* attempt to save to a register first */
	free_reg = v_unused_reg(octx, 0, vp->bits.regoff.reg.is_float, &r);

	if(free_reg){
		return free_reg;
	}else{
		/* no free registers, save this one
		 * and return its stack repr */
		return v_save_reg(octx, vp);
	}
}

void v_freeup_reg(const struct vreg *r)
{
	(void)r;
	TODO();
#if 0
	out_val *val = v_find_reg(r);

	if(vp && vp < &vtop[-allowable_stack + 1])
		v_freeup_regp(vp);
#endif
}

out_val *v_unused_reg(
		out_ctx *octx,
		int stack_as_backup, int fp,
		struct vreg *out)
{
	unsigned char used[sizeof(octx->reserved_regs)];
	out_val *it, *first;
	int i;

	memcpy(used, octx->reserved_regs, sizeof used);
	first = NULL;

	for(it = octx->val_head; it; it = it->next){
		if(it->type == V_REG && it->bits.regoff.reg.is_float == fp){
			if(!first)
				first = it;
			used[impl_reg_to_scratch(&it->bits.regoff.reg)] = 1;
		}
	}

	for(i = 0; i < (fp ? N_SCRATCH_REGS_F : N_SCRATCH_REGS_I); i++)
		if(!used[i]){
			impl_scratch_to_reg(i, out);
			out->is_float = fp;
			return v_new_reg(octx, NULL, out);
		}

	if(stack_as_backup){
		/* first->bits is clobbered by the v_freeup_regp() call */
		const struct vreg freed = first->bits.regoff.reg;

		/* no free regs, move `first` to the stack and claim its reg */
		v_freeup_regp(octx, first);

		return v_new_reg(octx, NULL, &freed);
	}
	return NULL;
}

out_val *v_to_reg_given(out_val *from, const struct vreg *given)
{
	return impl_load(from, given);
}

out_val *v_to_reg_out(out_ctx *octx, out_val *conv, struct vreg *out)
{
	if(conv->type != V_REG){
		struct vreg chosen;
		if(!out)
			out = &chosen;

		/* get a register */
		v_unused_reg(octx, 1, type_is_floating(conv->t), out);

		/* load into register */
		return v_to_reg_given(conv, out);

	}else{
		if(out)
			memcpy_safe(out, &conv->bits.regoff.reg);

		return conv;
	}
}

out_val *v_to_reg(out_ctx *octx, out_val *conv)
{
	return v_to_reg_out(octx, conv, NULL);
}

void v_save_regs(int n_ignore, type *func_ty)
{
	(void)n_ignore;
	(void)func_ty;
#if 0
	struct vstack *p;
	int n;

	/* see if we have fewer vstack entries than n_ignore */
	n = 1 + (vtop - vstack);

	if(n_ignore >= n)
		return;

	/* save all registers,
	 * except callee save regs unless we need to
	 */
	for(p = vstack; p < vtop - n_ignore; p++){
		int save = 1;
		if(p->type == V_REG){
			if(v_reg_is_const(&p->bits.regoff.reg))
			{
				/* don't save stack references */
				if(fopt_mode & FOPT_VERBOSE_ASM)
					out_comment("not saving const-reg %d", p->bits.regoff.reg.idx);
				save = 0;

			}else if(func_ty
			&& impl_reg_is_callee_save(&p->bits.regoff.reg, func_ty))
			{
				/* only comment for non-const regs */
				out_comment("not saving reg %d - callee save",
						p->bits.regoff.reg.idx);

				save = 0;
			}else{
				out_comment("saving register %d", p->bits.regoff.reg.idx);
			}
		}
		if(save)
			v_to_mem(p);
	}
#endif
	TODO();
}

void v_stack_adj(out_ctx *octx, unsigned amt, int sub)
{
	out_op(
			octx, sub ? op_minus : op_plus,
			out_new_l(
				octx,
				type_nav_btype(cc1_type_nav, type_intptr_t),
				amt),
			v_new_sp(octx, NULL));
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
			unsigned to_alloc;

			if(!noop){
				to_alloc = sz_rounded; /* the whole hog */
			}else{
				/* the extra we need to align by */
				to_alloc = sz_rounded - sz_initial;
			}

			if(fopt_mode & FOPT_VERBOSE_ASM){
				out_comment("stack alignment for %s (%u -> %u)",
						desc, octx->stack_sz, octx->stack_sz + sz_rounded);
				out_comment("alloc_n by %u (-> %u), padding with %u",
						sz_initial, octx->stack_sz + sz_initial,
						sz_rounded - sz_initial);
			}

			v_stack_adj(octx, to_alloc, 1);
		}

		octx->stack_sz += sz_rounded;
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
	if(force_mask || (octx->stack_sz & (align - 1))){
		type *const ty = type_nav_btype(cc1_type_nav, type_intptr_t);
		const unsigned new_sz = pack_to_align(octx->stack_sz, align);
		const unsigned added = new_sz - octx->stack_sz;
		out_val *sp = v_new_sp(octx, NULL);

		sp = out_op(
				octx, op_minus,
				sp,
				out_new_l(octx, ty, added));

		octx->stack_sz = new_sz;

		if(force_mask){
			sp = out_op(octx, op_and, sp, out_new_l(octx, ty, align - 1));
		}
		out_comment("stack aligned to %u bytes", align);
		return added;
	}
	return 0;
}

void v_dealloc_stack(out_ctx *octx, unsigned sz)
{
	/* callers should've snapshotted the stack previously
	 * and be calling us with said snapshot value
	 */
	assert((sz & (cc1_mstack_align - 1)) == 0
			&& "can't dealloc by a non-stack-align amount");

	v_stack_adj(octx, sz, 0);

	octx->stack_sz -= sz;
}
