#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../../util/alloc.h"

#include "../type.h"
#include "../type_nav.h"
#include "../type_is.h"
#include "../pack.h"

#include "../fopt.h"
#include "../cc1.h" /* cc1_mstack_align, fopt */

#include "val.h"
#include "out.h"
#include "virt.h"
#include "ctx.h"
#include "asm.h"
#include "impl.h"
#include "ctrl.h"

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
	/* if it's in a register already, ensure we keep the result in that register
	 * (e.g. stack pointer, argument register, return register, machine regs (idiv), etc */
	struct vreg reg;
	int got_reg = 0;
	const out_val *result;

	switch(val->type){
		case V_SPILT:
		case V_FLAG:
		case V_CONST_I:
		case V_CONST_F:
		case V_LBL:
			break;

		case V_REG:
		case V_REGOFF:
			/* make sure we get this before we start any v_to_reg() shenanigans */
			memcpy_safe(&reg, &val->bits.regoff.reg);
			got_reg = 1;
			break;
	}

	result = v_reg_apply_offset(octx, v_to_reg(octx, val));

	if(got_reg){
		assert(result->type == V_REG);
		v_reg_cp_no_off(octx, result, &reg);
	}

	out_val_consume(octx, result);
}

void v_reg_cp_no_off(
		out_ctx *octx, const out_val *from, const struct vreg *to_reg)
{
	assert(from->type == V_REG && "reg_cp on non register type");

	if(!from->bits.regoff.offset && vreg_eq(&from->bits.regoff.reg, to_reg))
		return;

	impl_reg_cp_no_off(octx, from, to_reg);
}

int v_is_const_reg(const out_val *v)
{
	return v->type == V_REG
		&& impl_reg_frame_const(&v->bits.regoff.reg, 0);
}

type *v_get_type(const out_val *v)
{
	if(v->type == V_SPILT)
		return type_is_ptr(v->t);

	return v->t;
}

int v_needs_GOT(const out_val *v)
{
	return v->type == V_LBL
		&& v->bits.lbl.pic_type & OUT_LBL_PIC
		&& !(v->bits.lbl.pic_type & OUT_LBL_PICLOCAL);
}

const out_val *v_to_stack_mem(
		out_ctx *octx, const out_val *val, const out_val *stk,
		enum out_val_store type)
{
	out_val *spilt = v_dup_or_reuse(octx, stk, stk->t);

	val = v_to(octx, val, TO_CONST | TO_REG);

	out_val_retain(octx, spilt);
	out_store(octx, spilt, val);

	switch(type){
		case V_SPILT:
		case V_REGOFF:
			spilt->type = type;
			break;
		default:
			assert(0 && "can only store to stack mem for spill or regoff");
	}

	return spilt;
}

const out_val *v_reg_to_stack_mem(
		out_ctx *octx, struct vreg const *vr, const out_val *stk)
{
	const out_val *reg = v_new_reg(octx, NULL, stk->t, vr);
	return v_to_stack_mem(octx, reg, stk, V_REGOFF);
}

static int v_in(const out_val *vp, enum vto to)
{
	switch(vp->type){
		case V_SPILT:
		case V_FLAG:
			break;

		case V_CONST_I:
		case V_CONST_F:
			return !!(to & TO_CONST);

		case V_REG:
			return (to & TO_REG) && vp->bits.regoff.offset == 0;

		case V_REGOFF:
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
				octx, v_reg, stack_pos, V_SPILT);

		out_val_overwrite((out_val *)v_reg, spilt);

		out_val_release(octx, spilt);
	}

	return v_reg;
}

static void mark_callee_save_reg_as_used(out_ctx *octx, const struct vreg *reg)
{
	enum { END_SENTINEL = 2 };
	struct vreg *p;
	size_t current;

	for(p = octx->used_callee_saved; p && p->is_float != END_SENTINEL; p++)
		if(vreg_eq(p, reg))
			return;

	current = p ? p - octx->used_callee_saved : 0;

	octx->used_callee_saved = urealloc1(
			octx->used_callee_saved,
			(current + 2) * sizeof *octx->used_callee_saved);

	memcpy_safe(&octx->used_callee_saved[current], reg);
	octx->used_callee_saved[current + 1].is_float = END_SENTINEL;
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
			v_reg_cp_no_off(octx, vp, &cs_reg);
			memcpy_safe(&((out_val *)vp)->bits.regoff.reg, &cs_reg);

			mark_callee_save_reg_as_used(octx, &cs_reg);

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
		v_reg_cp_no_off(octx, vp, &r);

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

	OCTX_ITER_VALS(octx, i){
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
	&& !out_val_is_blockphi(to_replace, NULL)
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

	OCTX_ITER_VALS(octx, it){
		const out_val *this = &it->val;
		if(this->retains
		/*&& !out_val_is_blockphi(this, octx->current_blk)
		 * we don't want to overwrite phiblock values (so check them in this loop),
		 * even if we ignore them in other parts of the register liveness code */
		&& this->type == V_REG
		&& this->bits.regoff.reg.is_float == fp
		&& regtest(octx->current_fnty, &this->bits.regoff.reg))
		{
			if(!first)
				first = this;
			used[impl_reg_to_scratch(&this->bits.regoff.reg)] = 1;
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

const out_val *v_to_reg_given_freeup_no_off(
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
		int is_float;

		if(!out)
			out = &chosen;

		is_float = type_is_floating(v_get_type(conv));

		/* get a register */
		v_unused_reg(octx, 1, is_float, out, NULL);

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
		case V_REGOFF:
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
	return impl_op(octx, off > 0 ? op_plus : op_minus,
			vreg,
			out_new_l(octx, vreg->t, labs(off)));
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
			case V_SPILT:
				/* this is analogous to the V_REGOFF and V_REG cases */
				assert(!impl_reg_savable(&v->bits.regoff.reg));
				save = 0;
				break;

			case V_REGOFF:
			case V_REG:
				if(!impl_reg_savable(&v->bits.regoff.reg)){
					/* don't save stack references */

				}else if(func_ty
				&& impl_reg_is_callee_save(octx->current_fnty, &v->bits.regoff.reg))
				{
					/* only comment for non-const regs */
					if(cc1_fopt.verbose_asm){
						out_comment(octx, "not saving reg %d - callee save",
								v->bits.regoff.reg.idx);
					}

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
	octx->reserved_regs[impl_reg_to_scratch(r)]++;
}

void v_unreserve_reg(out_ctx *octx, const struct vreg *r)
{
	octx->reserved_regs[impl_reg_to_scratch(r)]--;
}

#define CASE_SWAP(from, to) \
		case flag_ ## from: return flag_ ## to; \
		case flag_ ## to: return flag_ ## from

enum flag_cmp v_not_cmp(enum flag_cmp cmp)
{
	switch(cmp){
		CASE_SWAP(le, gt);
		CASE_SWAP(lt, ge);
		CASE_SWAP(eq, ne);
		CASE_SWAP(overflow, no_overflow);
		CASE_SWAP(signbit, no_signbit);
	}
	assert(0 && "invalid op");
}

enum flag_cmp v_commute_cmp(enum flag_cmp cmp)
{
	switch(cmp){
		CASE_SWAP(le, ge);
		CASE_SWAP(lt, gt);

		case flag_eq: return flag_eq;
		case flag_ne: return flag_ne;

		case flag_overflow:
		case flag_no_overflow:
		case flag_signbit:
		case flag_no_signbit:
			assert(0 && "shouldn't be called for (no_)[overflow|signbit]");
	}
	assert(0 && "invalid op");
}
#undef CASE_SWAP
