#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "../../util/util.h"
#include "../../util/alloc.h"
#include "../data_structs.h"

#include "asm.h"
#include "out.h"
#include "vstack.h"
#include "impl.h"
#include "basic_block.h"
#include "basic_block_int.h"

#include "../../util/platform.h"
#include "../cc1.h"
#include "../pack.h"
#include "../defs.h"
#include "../opt.h"
#include "../const.h"

#define v_check_type(t) if(!t) t = type_ref_cached_VOID_PTR()

static int calc_ptr_step(type_ref *t);

/*
 * This entire stack-output idea was inspired by tinycc, and improved somewhat
 */

#define N_VSTACK 1024
static struct vstack vstack[N_VSTACK];
struct vstack *vtop = NULL;

/*
 * stack layout is:
 *   2nd variadic-extra arg...
 *   1st variadic-extra arg,
 *   3rd arg...
 *   2nd arg
 *   1st arg
 *   --bp/ra--,
 *   call_regs[0],
 *   call_regs[1],
 *   call_regs[2],
 *   variadic_reg_args[2],
 *   variadic_reg_args[1],
 *   variadic_reg_args[0], <-- stack_variadic_offset
 *   local_var[0],         <-- stack_local_offset
 *   local_var[1],
 *   local_var[2],         <-- stack_sz (note local vars may be packed)
 *
 * all call/variadic regs must be at increasing memory addresses for
 * variadic logic is in impl_func_prologue_save_variadic
 */
static int stack_sz, stack_local_offset, stack_variadic_offset;

/* we won't reserve it more than 255 times */
static unsigned char reserved_regs[MAX(N_SCRATCH_REGS_I, N_SCRATCH_REGS_F)];

int out_vcount(basic_blk *bb)
{
	(void)bb;

	return vtop ? 1 + (int)(vtop - vstack) : 0;
}

int v_stack_sz(basic_blk *bb)
{
	(void)bb;
	return stack_sz;
}

void vpush(basic_blk *bb, type_ref *t)
{
	v_check_type(t);

	if(!vtop){
		vtop = vstack;
	}else{
		UCC_ASSERT(vtop < vstack + N_VSTACK - 1,
				"vstack overflow, vtop=%p, vstack=%p, diff %d",
				(void *)vtop, (void *)vstack, out_vcount(bb));

		if(vtop->type == V_FLAG)
			v_to_reg(bb, vtop);

		vtop++;
	}

	v_clear(vtop, t);
}

static void v_push_reg(basic_blk *bb, int r)
{
	struct vreg reg = VREG_INIT(r, 0);
	vpush(bb, NULL);
	v_set_reg(vtop, &reg);
}

static void v_push_sp(basic_blk *bb)
{
	v_push_reg(bb, REG_SP);
}

void v_clear(struct vstack *vp, type_ref *t)
{
	memset(vp, 0, sizeof *vp);
	vp->t = t;
}

void v_set_flag(
		struct vstack *vp,
		enum flag_cmp c, enum flag_mod mods)
{
	v_clear(vp, type_ref_cached_BOOL());

	vp->type = V_FLAG;
	vp->bits.flag.cmp = c;
	vp->bits.flag.mods = mods;
}

void vpop(basic_blk *bb)
{
	(void)bb;
	UCC_ASSERT(vtop, "NULL vtop for vpop");

	if(vtop == vstack){
		vtop = NULL;
	}else{
		UCC_ASSERT(vtop < vstack + N_VSTACK - 1, "vstack underflow");
		vtop--;
	}
}

static int v_reg_is_const(basic_blk *bb, struct vreg *r)
{
	(void)bb;
	return !r->is_float && r->idx == REG_BP;
}

static void v_flush_volatile_reg(basic_blk *bb, struct vstack *vp)
{
	if(vp->type == V_REG && vp->bits.regoff.offset){
		/* prevent sub-calls attempting to reflush this offset register */
		const long off = vp->bits.regoff.offset;
		vp->bits.regoff.offset = 0;

		UCC_ASSERT(!vp->bits.regoff.reg.is_float,
				"flush volatile float?");

		if(v_reg_is_const(bb, &vp->bits.regoff.reg)){
			/* move to an unused register */
			struct vreg r;
			v_unused_reg(bb, 1, vp->bits.regoff.reg.is_float, &r);
			impl_reg_cp(bb, vp, &r);
			v_set_reg(vp, &r);
			/* vstack updated, add to the register */
		}

		v_push_reg(bb, vp->bits.regoff.reg.idx);
		/* make it nice - abs() */
		out_push_l(bb, type_ref_cached_INTPTR_T(), abs(off));
		impl_op(bb, off > 0 ? op_plus : op_minus);
		vpop(bb);
	}
}

static void v_flush_volatile(basic_blk *bb, struct vstack *vp)
{
	if(!vp)
		return;

	v_to_reg(bb, vp);

	/* need to flush offset regs */
	v_flush_volatile_reg(bb, vp);
}

void out_flush_volatile(basic_blk *bb)
{
	v_flush_volatile(bb, vtop);
}

void out_assert_vtop_null(basic_blk *bb)
{
	UCC_ASSERT(!vtop, "vtop not null (%d entries)", out_vcount(bb));
}

void vswap(basic_blk *bb)
{
	struct vstack tmp;
	memcpy_safe(&tmp, vtop);
	memcpy_safe(vtop, &vtop[-1]);
	memcpy_safe(&vtop[-1], &tmp);

	(void)bb;
}

void out_dump(basic_blk *bb)
{
	unsigned i;

	for(i = 0; &vstack[i] <= vtop; i++)
		out_comment(bb, "vstack[%d] = { .type=%d, .t=%s, .reg.idx = %d }",
				i, vstack[i].type, type_ref_to_str(vstack[i].t),
				vstack[i].bits.regoff.reg.idx);
}

void out_swap(basic_blk *bb)
{
	vswap(bb);
}

int v_unused_reg(basic_blk *bb, int stack_as_backup, int fp, struct vreg *out)
{
	static unsigned char used[sizeof(reserved_regs)];
	struct vstack *it, *first;
	int i;

	memcpy(used, reserved_regs, sizeof used);
	first = NULL;

	for(it = vstack; it <= vtop; it++){
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
			return 0;
		}

	if(stack_as_backup){
		/* first->bits is clobbered by the v_freeup_regp(bb, ) call */
		const struct vreg freed = first->bits.regoff.reg;

		/* no free regs, move `first` to the stack and claim its reg */
		v_freeup_regp(bb, first);

		memcpy_safe(out, &freed);
		return 0;
	}
	return -1;
}

void v_set_reg(struct vstack *vp, const struct vreg *r)
{
	vp->type = V_REG;
	memcpy_safe(&vp->bits.regoff.reg, r);
	vp->bits.regoff.offset = 0;
}

void v_set_reg_i(struct vstack *vp, int i)
{
	const struct vreg r = VREG_INIT(i, 0);
	v_set_reg(vp, &r);
}

void v_to_reg_given(basic_blk *bb,
		struct vstack *from, const struct vreg *given)
{
	type_ref *const save = from->t;

	impl_load(bb, from, given);
	v_clear(from, save);
	v_set_reg(from, given);
}

void v_to_reg_out(basic_blk *bb, struct vstack *conv, struct vreg *out)
{
	if(conv->type != V_REG){
		struct vreg chosen;
		if(!out)
			out = &chosen;

		v_unused_reg(bb, 1, type_ref_is_floating(conv->t), out);
		v_to_reg_given(bb, conv, out);
	}else if(out){
		memcpy_safe(out, &conv->bits.regoff.reg);
	}
}

void v_to_reg(basic_blk *bb, struct vstack *conv)
{
	v_to_reg_out(bb, conv, NULL);
}

static void v_set_stack(struct vstack *vp, type_ref *ty, long off)
{
	v_set_reg_i(vp, REG_BP);
	if(ty)
		vp->t = ty;
	vp->bits.regoff.offset = off;
}

void v_to_mem_given(basic_blk *bb, struct vstack *vp, int stack_pos)
{
	struct vstack store = VSTACK_INIT(0);

	v_to(bb, vp, TO_CONST | TO_REG);

	v_set_stack(&store, vp->t, stack_pos);

	/* the following gen two instructions - subq and movq
	 * instead/TODO: impl_save_reg(vp) -> "pushq %%rax"
	 * -O1?
	 */
	v_store(bb, vp, &store);

	memcpy_safe(vp, &store);
}

void v_to_mem(basic_blk *bb, struct vstack *vp)
{
	switch(vp->type){
		case V_CONST_I:
		case V_CONST_F:
		case V_FLAG:
			v_to_reg(bb, vp);

		case V_REG:
			v_save_reg(bb, vp);

		case V_REG_SAVE:
		case V_LBL:
			break;
	}
}

static int v_in(basic_blk *bb, struct vstack *vp, enum vto to)
{
	(void)bb;
	switch(vp->type){
		case V_FLAG:
			break;

		case V_CONST_I:
		case V_CONST_F:
			return !!(to & TO_CONST);

		case V_REG:
		case V_REG_SAVE:
			return 0; /* needs further checks */

		case V_LBL:
			return !!(to & TO_MEM);
	}

	return 0;
}

void v_to(basic_blk *bb, struct vstack *vp, enum vto loc)
{
	if(v_in(bb, vp, loc))
		return;

	/* TO_CONST can't be done - it should already be const,
	 * or another option should be chosen */

	/* go for register first */
	if(loc & TO_REG){
		v_to_reg(bb, vp);
		/* need to flush any offsets */
		if(vp->bits.regoff.offset)
			v_flush_volatile(bb, vp);
		return;
	}

	if(loc & TO_MEM){
		v_to_mem(bb, vp);
		return;
	}

	ICE("can't satisfy v_to 0x%x", loc);
}

static struct vstack *v_find_reg(basic_blk *bb, const struct vreg *reg)
{
	struct vstack *vp;
	(void)bb;

	if(!vtop)
		return NULL;

	for(vp = vstack; vp <= vtop; vp++)
		if(vp->type == V_REG && vreg_eq(&vp->bits.regoff.reg, reg))
			return vp;

	return NULL;
}

void v_freeup_regp(basic_blk *bb, struct vstack *vp)
{
	/* freeup this reg */
	struct vreg r;
	int found_reg;

	UCC_ASSERT(vp->type == V_REG, "not reg");

	/* attempt to save to a register first */
	found_reg = (v_unused_reg(bb, 0, vp->bits.regoff.reg.is_float, &r) == 0);

	if(found_reg){
		impl_reg_cp(bb, vp, &r);

		v_clear(vp, vp->t); /* clear bitfield info, etc */
		v_set_reg(vp, &r);

	}else{
		v_save_reg(bb, vp);
	}
}

static void v_stack_adj(basic_blk *bb, unsigned amt, int sub)
{
	v_push_sp(bb);
	out_push_l(bb, type_ref_cached_INTPTR_T(), amt);
	out_op(bb, sub ? op_minus : op_plus);
	out_flush_volatile(bb);
	out_pop(bb);
}

static unsigned v_alloc_stack2(basic_blk *bb,
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
				out_comment(bb, "stack alignment for %s (%u -> %u)",
						desc, stack_sz, stack_sz + sz_rounded);
				out_comment(bb, "alloc_n by %u (-> %u), padding with %u",
						sz_initial, stack_sz + sz_initial,
						sz_rounded - sz_initial);
			}

			v_stack_adj(bb, to_alloc, 1);
		}

		stack_sz += sz_rounded;
	}

	return sz_rounded;
}

unsigned v_alloc_stack_n(basic_blk *bb, unsigned sz, const char *desc)
{
	return v_alloc_stack2(bb, sz, 1, desc);
}

unsigned v_alloc_stack(basic_blk *bb, unsigned sz, const char *desc)
{
	return v_alloc_stack2(bb, sz, 0, desc);
}

void v_stack_align(basic_blk *bb, unsigned const align, int mask)
{
	if(stack_sz & (align - 1)){
		type_ref *const ty = type_ref_cached_INTPTR_T();
		const int new_sz = pack_to_align(stack_sz, align);

		v_push_sp(bb);
		out_push_l(bb, ty, new_sz - stack_sz);
		out_op(bb, op_minus);
		stack_sz = new_sz;

		if(mask){
			out_push_l(bb, ty, align - 1);
			out_op(bb, op_and);
		}
		out_flush_volatile(bb);
		out_pop(bb);
		out_comment(bb, "stack aligned to %u bytes", align);
	}
}

void v_dealloc_stack(basic_blk *bb, unsigned sz)
{
	/* callers should've snapshotted the stack previously
	 * and be calling us with said snapshot value
	 */
	UCC_ASSERT((sz & (cc1_mstack_align - 1)) == 0,
			"can't dealloc by a non-stack-align amount");

	v_stack_adj(bb, sz, 0);

	stack_sz -= sz;
}

void v_save_reg(basic_blk *bb, struct vstack *vp)
{
	UCC_ASSERT(vp->type == V_REG, "not reg");

	out_comment(bb, "register spill:");

	v_alloc_stack(bb, type_ref_size(vp->t, NULL), "save reg");

	v_to_mem_given(
			bb,
			vp,
			-v_stack_sz(bb));

	vp->type = V_REG_SAVE;
}

void v_save_regs(basic_blk *bb, int n_ignore, type_ref *func_ty)
{
	struct vstack *p;
	int n;

	if(!vtop)
		return;

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
			if(v_reg_is_const(bb, &p->bits.regoff.reg))
			{
				/* don't save stack references */
				if(fopt_mode & FOPT_VERBOSE_ASM)
					out_comment(bb, "not saving const-reg %d", p->bits.regoff.reg.idx);
				save = 0;

			}else if(func_ty
			&& impl_reg_is_callee_save(&p->bits.regoff.reg, func_ty))
			{
				/* only comment for non-const regs */
				out_comment(bb, "not saving reg %d - callee save",
						p->bits.regoff.reg.idx);

				save = 0;
			}else{
				out_comment(bb, "saving register %d", p->bits.regoff.reg.idx);
			}
		}
		if(save)
			v_to_mem(bb, p);
	}
}

void v_freeup_reg(basic_blk *bb, const struct vreg *r, int allowable_stack)
{
	struct vstack *vp = v_find_reg(bb, r);

	if(vp && vp < &vtop[-allowable_stack + 1])
		v_freeup_regp(bb, vp);
}

static void v_alter_reservation(basic_blk *bb, const struct vreg *r, int n)
{
	int i = impl_reg_to_scratch(r);
	(void)bb;
	if(0 <= i && i < (int)sizeof reserved_regs)
		reserved_regs[i] += n;
}

void v_reserve_reg(basic_blk *bb, const struct vreg *r)
{
	v_alter_reservation(bb, r, +1);
}

void v_unreserve_reg(basic_blk *bb, const struct vreg *r)
{
	v_alter_reservation(bb, r, -1);
}

void v_freeup_regs(basic_blk *bb, const struct vreg *a, const struct vreg *b)
{
	v_reserve_reg(bb, a);
	v_reserve_reg(bb, b);

	v_freeup_reg(bb, a, 2);
	v_freeup_reg(bb, b, 2);

	v_unreserve_reg(bb, a);
	v_unreserve_reg(bb, b);
}

void v_inv_cmp(struct flag_opts *flag)
{
	switch(flag->cmp){
#define OPPOSITE2(from, to)    \
		case flag_ ## from:        \
			flag->cmp = flag_ ## to; \
			return

#define OPPOSITE(from, to) \
		OPPOSITE2(from, to);   \
		OPPOSITE2(to, from)

		OPPOSITE(eq, ne);

		OPPOSITE(le, gt);

		OPPOSITE(lt, ge);

		OPPOSITE(overflow, no_overflow);

		/*OPPOSITE(z, nz);
		OPPOSITE(nz, z);*/
#undef OPPOSITE
#undef OPPOSITE2
	}
	ICE("invalid op");
}

void out_pop(basic_blk *bb)
{
	vpop(bb);
}

void out_push_num(basic_blk *bb, type_ref *t, const numeric *n)
{
	const int ty_fp = type_ref_is_floating(t),
				n_fp = K_FLOATING(*n);

	vpush(bb, t);

	if(ty_fp != n_fp)
		ICW(
			"float type mismatch (n=%c, ty=%c)",
			"ny"[n_fp],
			"ny"[ty_fp]);

	if(ty_fp){
		vtop->type = V_CONST_F;
		vtop->bits.val_f = n->val.f;
		impl_load_fp(bb, vtop);
	}else{
		vtop->type = V_CONST_I;
		vtop->bits.val_i = n->val.i;
		impl_load_iv(bb, vtop);
	}
}

void out_push_l(basic_blk *bb, type_ref *t, long l)
{
	numeric iv;
	memset(&iv, 0, sizeof iv);
	iv.val.i = l;

	out_push_num(bb, t, &iv);
}

void out_push_zero(basic_blk *bb, type_ref *t)
{
	numeric n;
	memset(&n, 0, sizeof n);

	if(type_ref_is_floating(t))
		n.suffix = VAL_FLOAT;

	out_push_num(bb, t, &n);
}

static void out_set_lbl(basic_blk *bb, const char *s, int pic)
{
	(void)bb;
	vtop->type = V_LBL;

	vtop->bits.lbl.str = s;
	vtop->bits.lbl.pic = pic;
	vtop->bits.lbl.offset = 0;
}

void out_push_lbl(basic_blk *bb, const char *s, int pic)
{
	vpush(bb, NULL);

	out_set_lbl(bb, s, pic);
}

void out_push_noop(basic_blk *bb)
{
	out_push_zero(bb, type_ref_cached_INTPTR_T());
}

void out_dup(basic_blk *bb)
{
	/* TODO: mark reg as duped, but COW */
	vpush(bb, NULL);
	switch(vtop[-1].type){
		case V_FLAG:
			v_to_reg(bb, &vtop[-1]);
			/* fall */
		case V_REG:
		case V_REG_SAVE:
			if(v_reg_is_const(bb, &vtop[-1].bits.regoff.reg)){
				/* fall to memcpy */
			}else{
				/* need a new reg */
				struct vreg r;

				const long off = vtop[-1].bits.regoff.offset;
				/* if it's offset, save the offset,
				 * copy the reg and restore the offset
				 */
				vtop[-1].bits.regoff.offset = 0;

				v_unused_reg(bb, 1, vtop[-1].bits.regoff.reg.is_float, &r);

				out_comment(bb, "dup");
				impl_reg_cp(bb, &vtop[-1], &r);

				v_set_reg(vtop, &r);
				vtop->t = vtop[-1].t;

				/* restore offset */
				vtop[-1].bits.regoff.offset = vtop->bits.regoff.offset = off;
				break;
			}
		case V_CONST_I:
		case V_CONST_F:
		case V_LBL:
			/* fine */
			memcpy_safe(&vtop[0], &vtop[-1]);
			break;
	}
}

void out_pulltop(basic_blk *bb, int i)
{
	struct vstack tmp;
	int j;

	(void)bb;

	memcpy_safe(&tmp, &vtop[-i]);

	for(j = -i; j < 0; j++)
		vtop[j] = vtop[j + 1];

	memcpy_safe(vtop, &tmp);
}

/* expects vtop to be the value to merge with,
 * and &vtop[-1] to be a pointer to the word of memory */
static void bitfield_scalar_merge(basic_blk *bb, const struct vbitfield *const bf)
{
	/* load in,
	 * &-out where we want our value (to 0),
	 * &-out our value (so we don't affect other bits),
	 * |-in our new value,
	 * store
	 */

	type_ref *const ty = type_ref_ptr_depth_dec(vtop[-1].t, NULL);
	unsigned long mask_leading_1s, mask_back_0s, mask_rm;

	/* coerce vtop to a vtop[-1] type */
	out_cast(bb, ty);

	/* load the pointer to the store, forgetting the bitfield */
	/* stack: store, val */
	out_swap(bb);
	out_dup(bb);
	vtop->bitfield.nbits = 0;
	/* stack: val, store, store-less-bitfield */

	/* load the bitfield without using bitfield semantics */
	out_deref(bb);
	/* stack: val, store, orig-val */
	out_pulltop(bb, 2);
	/* stack: store, orig-val, val */
	out_swap(bb);
	/* stack: store, val, orig-val */

	/* e.g. width of 3, offset of 2:
	 * 111100000
	 *     ^~~^~
	 *     |  |- offset
	 *     +- width
	 */

	mask_leading_1s = -1UL << (bf->off + bf->nbits);

	/* e.g. again:
	 *
	 * -1 = 11111111
	 * << = 11111100
	 * ~  = 00000011
	 */
	mask_back_0s = ~(-1UL << bf->off);

	/* | = 111100011 */
	mask_rm = mask_leading_1s | mask_back_0s;

	/* &-out our value */
	out_push_l(bb, ty, mask_rm);
	out_comment(bb, "bitmask/rm = %#lx", mask_rm);
	out_op(bb, op_and);

	/* stack: store, val, orig-val-masked */
	/* bring the value to the top */
	out_swap(bb);

	/* mask and shift our value up */
	out_push_l(bb, ty, ~(-1UL << bf->nbits));
	out_op(bb, op_and);

	out_push_l(bb, ty, bf->off);
	out_op(bb, op_shiftl);

	/* | our value in with the dereferenced store value */
	out_op(bb, op_or);
}

void out_store(basic_blk *bb)
{
	struct vstack *store, *val;

	val   = &vtop[0];
	store = &vtop[-1];

	store->t = type_ref_ptr_depth_dec(store->t, NULL);

	v_store(bb, val, store);

	/* swap, popping the store, but not the value */
	vswap(bb);
	vpop(bb);
}

void v_store(basic_blk *bb, struct vstack *val, struct vstack *store)
{
	if(store->bitfield.nbits){
		UCC_ASSERT(val == vtop && store == vtop-1,
				"TODO: bitfield merging for non-vtops");

		bitfield_scalar_merge(bb, &store->bitfield);
	}

	v_flush_volatile_reg(bb, val); /* flush offset on any registers
	                            * only for the value we're assigning */
	impl_store(bb, val, store);
}

void out_normalise(basic_blk *bb)
{
	switch(vtop->type){
		case V_FLAG:
			/* already normalised */
			break;

		case V_CONST_I:
			vtop->bits.val_i = !!vtop->bits.val_i;
			break;

		case V_CONST_F:
			vtop->bits.val_i = !!vtop->bits.val_f;
			vtop->type = V_CONST_I;
			break;

		default:
			v_to_reg(bb, vtop);

		case V_REG:
			out_comment(bb, "normalise");

			out_push_zero(bb, vtop->t);
			out_op(bb, op_ne);
			/* 0 -> `0 != 0` = 0
			 * 1 -> `1 != 0` = 1
			 * 5 -> `5 != 0` = 1
			 */
	}
}

void out_push_sym(basic_blk *bb, sym *s)
{
	decl *const d = s->decl;

	vpush(bb, type_ref_ptr_depth_inc(d->ref));

	switch(s->type){
		case sym_local:
			if(DECL_IS_FUNC(d))
				goto label;

			if((d->store & STORE_MASK_STORE) == store_register && d->spel_asm)
				ICW("TODO: %s asm(\"%s\")", decl_to_str(d), d->spel_asm);

			/* sym offsetting takes into account the stack growth direction */
			v_set_stack(vtop, NULL, -(long)(s->loc.stack_pos + stack_local_offset));
			break;

		case sym_arg:
			v_set_stack(vtop, NULL, s->loc.arg_offset);
			break;

		case sym_global:
label:
			out_set_lbl(bb, decl_asm_spel(d), /*pic:*/1);
			break;
	}
}

void out_push_sym_val(basic_blk *bb, sym *s)
{
	out_push_sym(bb, s);
	out_deref(bb);
}

static int calc_ptr_step(type_ref *t)
{
	/* we are calculating the sizeof *t */
	int mul;

	if(type_ref_is_type(type_ref_is_ptr(t), type_void))
		return type_primitive_size(type_void);

	if(type_ref_is_type(t, type_unknown))
		mul = 1;
	else
		mul = type_ref_size(type_ref_next(t), NULL);

	return mul;
}

void out_op(basic_blk *bb, enum op_type op)
{
	/*
	 * the implementation does a vpop(bb) and
	 * sets vtop sufficiently to describe how
	 * the result is returned
	 */

	struct vstack *t_const = NULL, *t_mem_reg = NULL;

	/* check for adding or subtracting to stack */
#define POPULATE_TYPE(vp)   \
	switch(vp.type){          \
		case V_CONST_I:         \
			t_const = &vp;        \
			break;                \
		case V_REG:             \
		case V_LBL:             \
			t_mem_reg = &vp;      \
		default:                \
			break;                \
	}

	POPULATE_TYPE(vtop[0]);
	POPULATE_TYPE(vtop[-1]);

	if(t_const && t_mem_reg
	/* if it's a minus, we enforce an order */
	&& (op == op_plus || (op == op_minus && t_const == vtop))
	&& (t_mem_reg->type != V_LBL || (fopt_mode & FOPT_SYMBOL_ARITH)))
	{
		/* t_const == vtop... should be */
		long *p;
		switch(t_mem_reg->type){
			case V_LBL: p = &t_mem_reg->bits.lbl.offset; break;
			case V_REG: p = &t_mem_reg->bits.regoff.offset; break;
			default: ucc_unreach();
		}

		*p += (op == op_minus ? -1 : 1) *
			t_const->bits.val_i *
			calc_ptr_step(t_mem_reg->t);

		goto pop_const;

	}else if(t_const){
		/* lvalues implicitly disallowed */
		switch(op){
			case op_plus:
			case op_minus:
			case op_or:
			case op_xor:
			case op_shiftl: /* if we're shifting 0, or shifting _by_ zero, noop */
			case op_shiftr:
				if(t_const->bits.val_i == 0)
					goto pop_const;
			default:
				break;

			case op_multiply:
			case op_divide:
				if(t_const->bits.val_i == 1)
					goto pop_const;
				break;

			case op_and:
				if((sintegral_t)t_const->bits.val_i == -1)
					goto pop_const;
				break;

pop_const:
				/* we can only do this for non-commutative ops
				 * if t_const is the top/rhs */
				if(!op_is_commutative(op) && t_const != vtop)
					break;

				if(t_const != vtop)
					vswap(bb); /* need t_const on top for discarding */
				vpop(bb);
				goto out;
		}

		/* constant folding */
		if(((t_const == vtop ? &vtop[-1] : vtop)->type) == V_CONST_I){
			const char *err = NULL;
			const integral_t eval = const_op_exec(
					vtop[-1].bits.val_i, &vtop->bits.val_i,
					op, type_ref_is_signed(vtop->t),
					&err);

			UCC_ASSERT(!err, "const op err %s", err);

			vpop(bb);
			vtop->type = V_CONST_I;
			vtop->bits.val_i = eval;
			goto out;
		}
	}

	{
		int div = 0;

		switch(op){
			case op_plus:
			case op_minus:
			{
				int l_ptr = !!type_ref_is(vtop[-1].t, type_ref_ptr),
				    r_ptr = !!type_ref_is(vtop->t   , type_ref_ptr);

				if(l_ptr || r_ptr){
					const int ptr_step = calc_ptr_step(
							l_ptr ? vtop[-1].t : vtop->t);

					if(l_ptr ^ r_ptr){
						/* ptr +/- int, adjust the non-ptr by sizeof *ptr */
						struct vstack *val = &vtop[l_ptr ? 0 : -1];

						switch(val->type){
							case V_CONST_I:
								val->bits.val_i *= ptr_step;
								break;

							case V_CONST_F:
								ICE("float pointer?");

							case V_LBL:
							case V_FLAG:
							case V_REG_SAVE:
								v_to_reg(bb, val);

							case V_REG:
							{
								int swap;
								if((swap = (val != vtop)))
									vswap(bb);

								out_push_l(bb, type_ref_cached_INTPTR_T(), ptr_step);
								out_op(bb, op_multiply);

								if(swap)
									vswap(bb);
							}
						}

					}else if(l_ptr && r_ptr){
						/* difference - divide afterwards */
						div = ptr_step;
					}
				}
				break;
			}

			default:
				break;
		}

		impl_op(bb, op);

		if(div){
			out_push_l(bb, type_ref_cached_VOID_PTR(), div);
			out_op(bb, op_divide);
		}
	}
out:;
}

void out_set_bitfield(basic_blk *bb, unsigned off, unsigned nbits)
{
	(void)bb;
	vtop->bitfield.off   = off;
	vtop->bitfield.nbits = nbits;
}

static void bitfield_to_scalar(basic_blk *bb, const struct vbitfield *bf)
{
	type_ref *const ty = vtop->t;

	/* shift right, then mask */
	out_push_l(bb, ty, bf->off);
	out_op(bb, op_shiftr);

	out_push_l(bb, ty, ~(-1UL << bf->nbits));
	out_op(bb, op_and);

	/* if it's signed we need to sign extend
	 * using a signed right shift to copy its MSB
	 */
	if(type_ref_is_signed(ty)){
		const unsigned ty_sz = type_ref_size(ty, NULL);
		const unsigned nshift = CHAR_BIT * ty_sz - bf->nbits;

		out_push_l(bb, ty, nshift);
		out_op(bb, op_shiftl);
		out_push_l(bb, ty, nshift);
		out_op(bb, op_shiftr);
	}
}

void v_deref_decl(struct vstack *vp)
{
	/* XXX: memleak */
	vp->t = type_ref_ptr_depth_dec(vp->t, NULL);
}

void out_deref(basic_blk *bb)
{
	const struct vbitfield bf = vtop->bitfield;
#define DEREF_CHECK
#ifdef DEREF_CHECK
	type_ref *const vtop_t = vtop->t;
#endif
	type_ref *indir;
	int fp;

	indir = type_ref_ptr_depth_dec(vtop->t, NULL);

	/* if the pointed-to object is not an lvalue, don't deref */
	if(type_ref_is(indir, type_ref_array)
	|| type_ref_is(type_ref_is_ptr(vtop->t), type_ref_func)){
		out_change_type(bb, indir);
		return; /* noop */
	}

	v_deref_decl(vtop);
	fp = type_ref_is_floating(vtop->t);

	switch(vtop->type){
		case V_FLAG:
			ICE("deref of flag");
		case V_CONST_F:
			ICE("deref of float");

		case V_REG:
			/* attempt to convert to v_reg_save if possible */
			if(v_reg_is_const(bb, &vtop->bits.regoff.reg)){
				vtop->type = V_REG_SAVE;
				break;
			}
		case V_REG_SAVE:
		case V_LBL:
		case V_CONST_I:
		{
			struct vreg r;

			v_unused_reg(bb, 1, fp, &r);

			impl_deref(bb, vtop, &r, indir);

			v_set_reg(vtop, &r);
			break;
		}
	}

#ifdef DEREF_CHECK
	UCC_ASSERT(vtop_t != vtop->t, "no depth change");
#endif

	if(bf.nbits)
		bitfield_to_scalar(bb, &bf);
}


void out_op_unary(basic_blk *bb, enum op_type op)
{
	(void)bb;

	switch(op){
		case op_plus:
			return;

		default:
			/* special case - reverse the flag if possible */
			switch(vtop->type){
				case V_FLAG:
					if(op == op_not){
						v_inv_cmp(&vtop->bits.flag);
						return;
					}
					break;

				case V_CONST_I:
					switch(op){
#define OP(t, o) case op_ ## t: vtop->bits.val_i = o vtop->bits.val_i; return
						OP(not, !);
						OP(minus, -);
						OP(bnot, ~);
#undef OP

						default:
							ICE("invalid unary op");
					}
					break;

				case V_REG_SAVE:
				case V_REG:
				case V_LBL:
				case V_CONST_F:
					break;
			}
	}

	impl_op_unary(bb, op);
}

void out_cast(basic_blk *bb, type_ref *to)
{
	v_cast(bb, vtop, to);
}

void v_cast(basic_blk *bb, struct vstack *vp, type_ref *to)
{
	type_ref *const from = vp->t;
	char fp[2];
	fp[0] = type_ref_is_floating(from);
	fp[1] = type_ref_is_floating(to);

	if(fp[0] || fp[1]){
		if(fp[0] && fp[1]){

			if(type_ref_primitive(from) != type_ref_primitive(to))
				impl_f2f(bb, vp, from, to);
			/* else no-op cast */

		}else if(fp[0]){
			/* float -> int */
			UCC_ASSERT(type_ref_is_integral(to),
					"fp to %s?", type_ref_to_str(to));

			impl_f2i(bb, vp, from, to);

		}else{
			/* int -> float */
			UCC_ASSERT(type_ref_is_integral(from),
					"%s to fp?", type_ref_to_str(from));

			impl_i2f(bb, vp, from, to);
		}

	}else{
		/* casting integral vtop
		 * don't bother if it's a constant,
		 * just change the size */
		if(vp->type != V_CONST_I){
			int szfrom = asm_type_size(from),
					szto   = asm_type_size(to);

			if(szfrom != szto){
				if(szto > szfrom){
					impl_cast_load(bb, vp, from, to,
							type_ref_is_signed(from));
				}else{
					char buf[TYPE_REF_STATIC_BUFSIZ];

					out_comment(bb, "truncate cast from %s to %s, size %d -> %d",
							from ? type_ref_to_str_r(buf, from) : "",
							to   ? type_ref_to_str(to) : "",
							szfrom, szto);
				}
			}
		}
	}

	vp->t = to;
}

void out_change_type(basic_blk *bb, type_ref *t)
{
	(void)bb;

	v_check_type(t);
	/* XXX: memleak */
	vtop->t = t;

	/* we can't change type for large integer values,
	 * they need truncating
	 */
	UCC_ASSERT(
			vtop->type != V_CONST_I
			|| !integral_is_64_bit(vtop->bits.val_i, vtop->t),
			"can't %s for large constant %" NUMERIC_FMT_D, __func__,
			vtop->bits.val_i);
}

void out_call(basic_blk *bb, int nargs, type_ref *r_ret, type_ref *r_func)
{
	const int fp = type_ref_is_floating(r_ret);
	struct vreg vr = VREG_INIT(fp ? REG_RET_F : REG_RET_I, fp);

	impl_call(bb, nargs, r_ret, r_func);

	/* return type */
	v_clear(vtop, r_ret);

	v_set_reg(vtop, &vr);
}

void out_comment_sec(enum section_type sec, const char *fmt, ...)
{
	va_list l;

	va_start(l, fmt);
	impl_comment_sec(sec, fmt, l);
	va_end(l);
}

void out_comment(basic_blk *bb, const char *fmt, ...)
{
	va_list l;

	va_start(l, fmt);
	bb_commentv(bb, fmt, l);
	va_end(l);
}

basic_blk *out_func_prologue(
		const char *spel,
		type_ref *rf,
		int stack_res, int nargs, int variadic,
		int arg_offsets[])
{
	basic_blk *bb = bb_new();

	bb_label(bb, spel);

	UCC_ASSERT(stack_sz == 0, "non-empty stack for new func");

	impl_func_prologue_save_fp(bb);

	if(mopt_mode & MOPT_STACK_REALIGN)
		v_stack_align(bb, cc1_mstack_align, 1);

	impl_func_prologue_save_call_regs(bb, rf, nargs, arg_offsets);

	if(variadic) /* save variadic call registers */
		bb = impl_func_prologue_save_variadic(bb, rf);

	/* setup "pointers" to the right place in the stack */
	stack_variadic_offset = stack_sz - platform_word_size();
	stack_local_offset = stack_sz;

	if(stack_res)
		v_alloc_stack(bb, stack_res, "local variables");

	return bb;
}

void out_func_epilogue(basic_blk *bb, type_ref *rf)
{
	if(fopt_mode & FOPT_VERBOSE_ASM)
		out_comment(bb, "epilogue, stack_sz = %u", stack_sz);

	impl_func_epilogue(bb, rf);
	stack_local_offset = stack_sz = 0;
}

void out_pop_func_ret(basic_blk *bb, type_ref *t)
{
	impl_pop_func_ret(bb, t);
}

void out_undefined(basic_blk *bb)
{
	out_flush_volatile(bb);
	impl_undefined(bb);
}

void out_push_overflow(basic_blk *bb)
{
	vpush(bb, type_ref_cached_BOOL());
	impl_set_overflow(bb);
}

void out_push_frame_ptr(basic_blk *bb, int nframes)
{
	struct vreg vr = VREG_INIT(
		impl_frame_ptr_to_reg(bb, nframes),
		0
	);

	out_flush_volatile(bb);

	vpush(bb, NULL);
	v_set_reg(vtop, &vr);
}

void out_push_reg_save_ptr(basic_blk *bb)
{
	out_flush_volatile(bb);

	vpush(bb, NULL);
	v_set_stack(vtop, NULL, -stack_variadic_offset);
}

void out_push_nan(basic_blk *bb, type_ref *ty)
{
	UCC_ASSERT(type_ref_is_floating(ty),
			"non-float nan?");
	vpush(bb, NULL);
	impl_set_nan(bb, ty);
}
