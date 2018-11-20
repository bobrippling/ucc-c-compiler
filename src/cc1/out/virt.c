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
#include "ctrl.h"

#define REMOVE_CONST(t, exp) ((t)(exp))

static int v_unused_reg2(
		out_ctx *octx,
		int stack_as_backup, const int fp,
		struct vreg *out,
		out_val const *to_replace,
		int regtest(type *, const struct vreg *));

unsigned char *v_alloc_reg_reserve(out_ctx *octx, int *p)
{
	int n = N_SCRATCH_REGS_I + N_SCRATCH_REGS_F;
	if(p)
		*p = n;
	return umalloc(n * sizeof *octx->reserved_regs);
}

void out_flush_volatile(out_ctx *octx, const out_val *val)
{
	out_val_consume(octx, v_reg_apply_offset(octx, v_to_reg(octx, val)));
}

int v_is_const_reg(const out_val *v)
{
	return v->type == V_REG
		&& impl_reg_frame_const(&v->bits.regoff.reg, 0);
}

const out_val *v_to_stack_mem(
		out_ctx *octx, const out_val *val, const out_val *stk)
{
	out_val *spilt = v_dup_or_reuse(octx, stk, stk->t);

	val = v_to(octx, val, TO_CONST | TO_REG);

	out_val_retain(octx, spilt);
	out_store(octx, spilt, val);

	spilt->type = V_REG_SPILT;

	return spilt;
}

const out_val *v_reg_to_stack_mem(
		out_ctx *octx, struct vreg const *vr, const out_val *stk)
{
	const out_val *reg = v_new_reg(octx, NULL, stk->t, vr);
	return v_to_stack_mem(octx, reg, stk);
}

static int v_in(const out_val *vp, enum vto to)
{
	switch(vp->type){
		case V_FLAG:
			break;

		case V_CONST_I:
		case V_CONST_F:
			return !!(to & TO_CONST);

		case V_REG:
			return (to & TO_REG) && vp->bits.regoff.offset == 0;

		case V_REG_SPILT:
		case V_LBL:
			return !!(to & TO_MEM);
	}

	return 0;
}

static ucc_wur const out_val *v_spill_reg(
		out_ctx *octx, const out_val *v_reg)
{
	const out_val *stack_pos = out_aalloct(octx, v_reg->t);

	out_val_retain(octx, v_reg);

	{
		const out_val *spilt = v_to_stack_mem(
				octx, v_reg, stack_pos);

		out_val_overwrite((out_val *)v_reg, spilt);

		out_val_release(octx, spilt);
	}

	return v_reg;
}

static ucc_wur const out_val *v_save_reg(
		out_ctx *octx, const out_val *vp, type *fnty)
{
	assert(vp->type == V_REG && "not reg");

	if(fnty){
		/* try callee save */
		struct vreg cs_reg;
		int got_reg;

		got_reg = v_unused_reg2(
				octx, /*stack*/0, type_is_floating(vp->t),
				&cs_reg, NULL, impl_reg_is_callee_save);

		if(got_reg){
			struct vreg *p;
			int already_used = 0;

			impl_reg_cp_no_off(octx, vp, &cs_reg);
			memcpy_safe(&((out_val *)vp)->bits.regoff.reg, &cs_reg);

			for(p = octx->used_callee_saved; p && p->is_float != 2; p++){
				if(vreg_eq(p, &cs_reg)){
					already_used = 1;
					break;
				}
			}

			if(!already_used){
				size_t current = p ? p - octx->used_callee_saved : 0;

				octx->used_callee_saved = urealloc1(
						octx->used_callee_saved,
						(current + 2) * sizeof *octx->used_callee_saved);

				memcpy_safe(&octx->used_callee_saved[current], &cs_reg);
				octx->used_callee_saved[current + 1].is_float = 2;
			}

			return vp;
		}
	}

	return v_spill_reg(octx, vp);
}

const out_val *v_to(out_ctx *octx, const out_val *vp, enum vto loc)
{
	if(v_in(vp, loc))
		return REMOVE_CONST(out_val *, vp);

	/* TO_CONST can't be done - it should already be const,
	 * or another option should be chosen */

	/* go for register first */
	if(loc & TO_REG){
		return v_reg_apply_offset(octx, v_to_reg(octx, vp));
	}

	if(loc & TO_MEM){
		vp = v_to_reg(octx, vp);
		return v_save_reg(octx, vp, NULL);
	}

	assert(0 && "can't satisfy v_to");
}

static ucc_wur const out_val *v_freeup_regp(out_ctx *octx, const out_val *vp)
{
	struct vreg r;
	int got_reg;

	assert(vp->type == V_REG && "not reg");

	v_reserve_reg(octx, &vp->bits.regoff.reg);

	/* attempt to save to a register first */
	got_reg = v_unused_reg(octx, 0, vp->bits.regoff.reg.is_float, &r, NULL);

	v_unreserve_reg(octx, &vp->bits.regoff.reg);

	if(got_reg){
		/* move 'vp' into the fresh reg */
		impl_reg_cp_no_off(octx, vp, &r);

		/* change vp's register to 'r', so that vp's original register is free */
		REMOVE_CONST(out_val *, vp)->bits.regoff.reg = r;

		return out_val_retain(octx, vp);

	}else{
		/* no free registers, save this one to the stack and mutate vp */
		return out_val_retain(octx, v_save_reg(octx, vp, octx->current_fnty));
		/* vp is now a stack value */
	}
}

static const out_val *v_find_reg(out_ctx *octx, const struct vreg *reg)
{
	out_val_list *i;

	for(i = octx->val_head; i; i = i->next){
		const out_val *v = &i->val;

		if(!v->retains)
			continue;
		if(out_val_is_blockphi(v, octx->current_blk)){
			/* if it's the same block, we've found the reg, else ignore and continue */
			continue;
		}

		if(v->type == V_REG && vreg_eq(&v->bits.regoff.reg, reg))
			return v;
	}

	return NULL;
}

void v_freeup_reg(out_ctx *octx, const struct vreg *r)
{
	const out_val *v = v_find_reg(octx, r);

	if(v)
		out_val_consume(octx, v_freeup_regp(octx, v));
}

static int v_unused_reg2(
		out_ctx *octx,
		int stack_as_backup, const int fp,
		struct vreg *out,
		out_val const *to_replace,
		int regtest(type *, const struct vreg *))
{
	unsigned char *used;
	int nused;
	out_val_list *it;
	const out_val *first;
	int i, begin, end;

	/* if the value is in a register, we only need a new register
	 * if we have other references to it */
	if(to_replace
	&& to_replace->retains == 1
	&& to_replace->type == V_REG
	&& to_replace->bits.regoff.reg.is_float == fp
	&& !impl_reg_frame_const(&to_replace->bits.regoff.reg, /*sp*/1))
	{
		memcpy_safe(out, &to_replace->bits.regoff.reg);
		return 1;
	}

	used = v_alloc_reg_reserve(octx, &nused);
	memcpy(used, octx->reserved_regs, nused * sizeof *used);

	begin = fp ? N_SCRATCH_REGS_I : 0;
	end = fp ? nused : N_SCRATCH_REGS_I;
	{
		int obegin = fp ? 0 : N_SCRATCH_REGS_I;
		int oend = fp ? N_SCRATCH_REGS_I : nused;
		for(i = obegin; i < oend; i++)
			used[i] = 1;
	}

	first = NULL;

	for(it = octx->val_head; it; it = it->next){
		const out_val *this = &it->val;
		if(this->retains
		&& this->type == V_REG
		&& this->bits.regoff.reg.is_float == fp
		&& regtest(octx->current_fnty, &this->bits.regoff.reg))
		{
			if(!first)
				first = this;
			used[impl_reg_to_idx(&this->bits.regoff.reg)] = 1;
		}
	}

	for(i = begin; i < end; i++){
		if(!used[i]){
			/* `i' should be in the `fp' range, since we're going
			 * from `begin' to `end' */
			out->is_float = fp;
			impl_scratch_to_reg(i, out);

			if(regtest(octx->current_fnty, out)){
				free(used);
				return 1;
			}
		}
	}

	free(used), used = NULL;

	if(stack_as_backup){
		const out_val *freed;

		/* no free regs, move `first` to the stack and claim its reg */
		*out = first->bits.regoff.reg;

		freed = v_freeup_regp(octx, first);

		out_val_consume(octx, freed);

		return 1;
	}
	return 0;
}

int v_unused_reg(
		out_ctx *octx,
		int stack_as_backup, const int fp,
		struct vreg *out,
		out_val const *to_replace)
{
	return v_unused_reg2(
			octx, stack_as_backup, fp, out, to_replace,
			impl_reg_is_scratch);
}

const out_val *v_to_reg_given(
		out_ctx *octx, const out_val *from,
		const struct vreg *given)
{
	return impl_load(octx, from, given);
}

const out_val *v_to_reg_given_freeup(
		out_ctx *octx, const out_val *from,
		const struct vreg *given)
{
	if(from->type == V_REG
	&& vreg_eq(&from->bits.regoff.reg, given))
	{
		return from;
	}

	v_freeup_reg(octx, given);
	return v_to_reg_given(octx, from, given);
}

const out_val *v_to_reg_out(out_ctx *octx, const out_val *conv, struct vreg *out)
{
	if(conv->type != V_REG){
		struct vreg chosen;
		if(!out)
			out = &chosen;

		/* get a register */
		v_unused_reg(octx, 1, type_is_floating(conv->t), out, NULL);

		/* load into register */
		return v_to_reg_given(octx, conv, out);

	}else{
		if(out)
			memcpy_safe(out, &conv->bits.regoff.reg);

		return conv;
	}
}

const out_val *v_to_reg(out_ctx *octx, const out_val *conv)
{
	return v_to_reg_out(octx, conv, NULL);
}

const out_val *v_reg_apply_offset(out_ctx *octx, const out_val *const orig)
{
	const out_val *vreg;
	long off;

	switch(orig->type){
		case V_REG:
		case V_REG_SPILT:
			break;
		default:
			assert(0 && "not a reg");
	}

	/* offset normalise */
	off = orig->bits.regoff.offset;
	if(!off)
		return orig;

	REMOVE_CONST(out_val *, orig)->bits.regoff.offset = 0;
	{
		vreg = v_dup_or_reuse(octx, orig, orig->t);
	}
	REMOVE_CONST(out_val *, orig)->bits.regoff.offset = off;
	REMOVE_CONST(out_val *, vreg)->bits.regoff.offset = 0;

	/* use impl_op as it doesn't do reg offsetting */
	return impl_op(octx, op_plus,
			vreg,
			out_new_l(octx, vreg->t, off));
}

static int val_present(const out_val *v, const out_val **ignores)
{
	const out_val **i;
	for(i = ignores; i && *i; i++)
		if(v == *i)
			return 1;
	return 0;
}

void v_save_regs(
		out_ctx *octx, type *func_ty,
		const out_val *ignores[], const out_val *fnval)
{
	/* save all registers except callee save,
	 * and the call-expr reg, if present */
	out_val_list *l;

	/* go backwards in case the list is added to */
	for(l = octx->val_tail; l; l = l->prev){
		out_val *v = &l->val;
		int save = 0;

		if(v->retains == 0)
			continue;
		if(v->phiblock)
			continue; /* phi values are special and don't need to be spilt across jumps */

		if(v == fnval || val_present(v, ignores)){
			/* don't save */
			continue;
		}

		switch(v->type){
			case V_REG_SPILT:
			case V_REG:
				if(!impl_reg_savable(&v->bits.regoff.reg)){
					/* don't save stack references */

				}else if(func_ty
				&& impl_reg_is_callee_save(octx->current_fnty, &v->bits.regoff.reg))
				{
					/* only comment for non-const regs */
					out_comment(octx, "not saving reg %d - callee save",
							v->bits.regoff.reg.idx);

				}else{
					out_comment(octx, "saving register %d", v->bits.regoff.reg.idx);
					save = 1;
				}
				break;

			case V_CONST_I:
			case V_CONST_F:
			case V_LBL:
				save = 0;
				break;

			case V_FLAG:
				assert(v->retains == 1 && "v_save_regs(): retained v");
				v = REMOVE_CONST(out_val *, v_to_reg(octx, v));
				save = 1;
		}

		if(save){
			const out_val *new;

			/* other out_val:s can be as heavily retained as they want,
			 * we still change the underlying val without any effect */

			new = v_save_reg(octx, v, func_ty);

			if(new != v){
				out_val_overwrite(v, new);
				/* transfer retain-ness to 'v' from 'new' */
				out_val_release(octx, new);
				out_val_retain(octx, v);
			}else{
				/* moved to callee save reg */
			}
		}
	}
}

void v_reserve_reg(out_ctx *octx, const struct vreg *r)
{
	octx->reserved_regs[impl_reg_to_idx(r)]++;
}

void v_unreserve_reg(out_ctx *octx, const struct vreg *r)
{
	octx->reserved_regs[impl_reg_to_idx(r)]--;
}

enum flag_cmp v_inv_cmp(enum flag_cmp cmp, int invert_eq)
{
	switch(cmp){
#define OPPOSITE2(from, to)    \
		case flag_ ## from:        \
			return flag_ ## to; \

#define OPPOSITE(from, to) \
		OPPOSITE2(from, to);   \
		OPPOSITE2(to, from)

		OPPOSITE(le, gt);
		OPPOSITE(lt, ge);
		OPPOSITE(overflow, no_overflow);

		/*OPPOSITE(z, nz);
		OPPOSITE(nz, z);*/
#undef OPPOSITE
#undef OPPOSITE2

		case flag_eq:
		case flag_ne:
			if(invert_eq)
				return (cmp == flag_eq ? flag_ne : flag_eq);
			return cmp;
	}
	assert(0 && "invalid op");
}
