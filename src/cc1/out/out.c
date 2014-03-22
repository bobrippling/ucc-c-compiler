#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "../../util/util.h"
#include "../../util/alloc.h"

#include "../../util/platform.h"
#include "../cc1.h"
#include "../pack.h"
#include "../defs.h"
#include "../opt.h"
#include "../const.h"
#include "../type_nav.h"
#include "../type_is.h"

#include "asm.h"
#include "out.h"
#include "val.h"
#include "impl.h"
#include "lbl.h"

struct out_ctx
{
	out_blk *current_blk;

	int stack_sz, stack_local_offset, stack_variadic_offset;

	/* we won't reserve it more than 255 times */
	unsigned char reserved_regs[MAX(N_SCRATCH_REGS_I, N_SCRATCH_REGS_F)];
};

struct out_blk
{
	out_ctx *octx;
	const char *desc;
	out_val *phi_val;

	struct
	{
		enum { BLK_NEXT_BLOCK, BLK_NEXT_EXPR } type;
		union
		{
			out_blk *blk;
			out_val *exp;
		} bits;
	} next;
};

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

#if 0
int v_unused_reg(out_ctx *octx, int stack_as_backup, int fp, struct vreg *out)
{
	unsigned char used[sizeof(octx->reserved_regs)];
	struct vstack *it, *first;
	int i;

	memcpy(used, octx->reserved_regs, sizeof used);
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
		/* first->bits is clobbered by the v_freeup_regp() call */
		const struct vreg freed = first->bits.regoff.reg;

		/* no free regs, move `first` to the stack and claim its reg */
		v_freeup_regp(first);

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

void v_to_reg_given(struct vstack *from, const struct vreg *given)
{
	type *const save = from->t;

	impl_load(from, given);
	v_clear(from, save);
	v_set_reg(from, given);
}

void v_to_reg_out(struct vstack *conv, struct vreg *out)
{
	if(conv->type != V_REG){
		struct vreg chosen;
		if(!out)
			out = &chosen;

		v_unused_reg(1, type_is_floating(conv->t), out);
		v_to_reg_given(conv, out);
	}else if(out){
		memcpy_safe(out, &conv->bits.regoff.reg);
	}
}

void v_to_reg(struct vstack *conv)
{
	v_to_reg_out(conv, NULL);
}

static void v_set_stack(
		struct vstack *vp, type *ty,
		long off, int lval)
{
	v_set_reg_i(vp, REG_BP);
	if(lval)
		vp->type = V_REG_SAVE;
	if(ty)
		vp->t = ty;
	vp->bits.regoff.offset = off;
}

void v_to_mem_given(struct vstack *vp, int stack_pos)
{
	struct vstack store = VSTACK_INIT(0);

	v_to(vp, TO_CONST | TO_REG);

	v_set_stack(&store, vp->t, stack_pos, /*lval:*/0);

	/* the following gen two instructions - subq and movq
	 * instead/TODO: impl_save_reg(vp) -> "pushq %%rax"
	 * -O1?
	 */
	v_store(vp, &store);

	memcpy_safe(vp, &store);
}

void v_to_mem(struct vstack *vp)
{
	switch(vp->type){
		case V_CONST_I:
		case V_CONST_F:
		case V_FLAG:
			v_to_reg(vp);

		case V_REG:
			v_save_reg(vp);

		case V_REG_SAVE:
		case V_LBL:
			break;
	}
}

static int v_in(struct vstack *vp, enum vto to)
{
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
			if(!vp->is_lval)
				return !!(to & TO_MEM);
	}

	return 0;
}

void v_to(struct vstack *vp, enum vto loc)
{
	if(v_in(vp, loc))
		return;

	/* TO_CONST can't be done - it should already be const,
	 * or another option should be chosen */

	/* go for register first */
	if(loc & TO_REG){
		v_to_reg(vp);
		/* need to flush any offsets */
		if(vp->bits.regoff.offset)
			v_flush_volatile(vp);
		return;
	}

	if(loc & TO_MEM){
		v_to_mem(vp);
		return;
	}

	ICE("can't satisfy v_to 0x%x", loc);
}

static struct vstack *v_find_reg(const struct vreg *reg)
{
	struct vstack *vp;
	if(!vtop)
		return NULL;

	for(vp = vstack; vp <= vtop; vp++)
		if(vp->type == V_REG && vreg_eq(&vp->bits.regoff.reg, reg))
			return vp;

	return NULL;
}

void v_freeup_regp(struct vstack *vp)
{
	/* freeup this reg */
	struct vreg r;
	int found_reg;

	UCC_ASSERT(vp->type == V_REG, "not reg");

	/* attempt to save to a register first */
	found_reg = (v_unused_reg(0, vp->bits.regoff.reg.is_float, &r) == 0);

	if(found_reg){
		impl_reg_cp(vp, &r);

		v_clear(vp, vp->t);
		v_set_reg(vp, &r);

	}else{
		v_save_reg(vp);
	}
}

void v_save_reg(struct vstack *vp)
{
	UCC_ASSERT(vp->type == V_REG, "not reg");

	out_comment("register spill:");

	v_alloc_stack(type_size(vp->t, NULL), "save reg");

	v_to_mem_given(
			vp,
			-v_stack_sz());

	vp->type = V_REG_SAVE;
}

void v_save_regs(int n_ignore, type *func_ty)
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
}

void v_freeup_reg(const struct vreg *r, int allowable_stack)
{
	struct vstack *vp = v_find_reg(r);

	if(vp && vp < &vtop[-allowable_stack + 1])
		v_freeup_regp(vp);
}

static void v_alter_reservation(const struct vreg *r, int n)
{
	int i = impl_reg_to_scratch(r);
	if(0 <= i && i < (int)sizeof reserved_regs)
		reserved_regs[i] += n;
}

void v_reserve_reg(const struct vreg *r)
{
	v_alter_reservation(r, +1);
}

void v_unreserve_reg(const struct vreg *r)
{
	v_alter_reservation(r, -1);
}

void v_freeup_regs(const struct vreg *a, const struct vreg *b)
{
	v_reserve_reg(a);
	v_reserve_reg(b);

	v_freeup_reg(a, 2);
	v_freeup_reg(b, 2);

	v_unreserve_reg(a);
	v_unreserve_reg(b);
}

void v_inv_cmp(struct flag_opts *flag, int invert_eq)
{
	switch(flag->cmp){
#define OPPOSITE2(from, to)    \
		case flag_ ## from:        \
			flag->cmp = flag_ ## to; \
			return

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
				flag->cmp = (flag->cmp == flag_eq ? flag_ne : flag_eq);
			return;
	}
	ICE("invalid op");
}

void out_pop(void)
{
	vpop();
}

void out_set_lvalue()
{
	vtop->is_lval = 1;
}

void out_push_num(type *t, const numeric *n)
{
	const int ty_fp = type_is_floating(t),
				n_fp = K_FLOATING(*n);

	vpush(t);

	if(ty_fp != n_fp)
		ICW(
			"float type mismatch (n=%c, ty=%c)",
			"ny"[n_fp],
			"ny"[ty_fp]);

	if(ty_fp){
		vtop->type = V_CONST_F;
		vtop->bits.val_f = n->val.f;
		impl_load_fp(vtop);
	}else{
		vtop->type = V_CONST_I;
		vtop->bits.val_i = n->val.i;
		impl_load_iv(vtop);
	}
}

void out_push_l(type *t, long l)
{
	numeric iv;
	memset(&iv, 0, sizeof iv);
	iv.val.i = l;

	out_push_num(t, &iv);
}

void out_push_zero(type *t)
{
	numeric n;
	memset(&n, 0, sizeof n);

	if(type_is_floating(t))
		n.suffix = VAL_FLOAT;

	out_push_num(t, &n);
}

static void out_set_lbl(const char *s, int pic)
{
	vtop->type = V_LBL;

	vtop->bits.lbl.str = s;
	vtop->bits.lbl.pic = pic;
	vtop->bits.lbl.offset = 0;
}

void out_push_lbl(const char *s, int pic)
{
	UCC_ASSERT(s, "null label");

	vpush(type_ptr_to(type_nav_btype(cc1_type_nav, type_void)));

	out_set_lbl(s, pic);
}

void out_push_noop()
{
	out_push_zero(type_nav_btype(cc1_type_nav, type_intptr_t));
}

void out_dup(void)
{
	/* TODO: mark reg as duped, but COW */
	out_comment("dup");
	vpush(vtop->t);
	switch(vtop[-1].type){
		case V_FLAG:
			v_to_reg(&vtop[-1]);
			/* fall */
		case V_REG:
		case V_REG_SAVE:
			if(v_reg_is_const(&vtop[-1].bits.regoff.reg)){
				/* fall to memcpy */
			}else{
				/* need a new reg */
				struct vreg r;

				const long off = vtop[-1].bits.regoff.offset;
				/* if it's offset, save the offset,
				 * copy the reg and restore the offset
				 */
				vtop[-1].bits.regoff.offset = 0;

				v_unused_reg(1, vtop[-1].bits.regoff.reg.is_float, &r);

				out_comment("dup");
				impl_reg_cp(&vtop[-1], &r);

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

void out_pulltop(int i)
{
	struct vstack tmp;
	int j;

	memcpy_safe(&tmp, &vtop[-i]);

	for(j = -i; j < 0; j++)
		vtop[j] = vtop[j + 1];

	memcpy_safe(vtop, &tmp);
}

/* expects vtop to be the value to merge with,
 * and &vtop[-1] to be a pointer to the word of memory */
static void bitfield_scalar_merge(const struct vbitfield *const bf)
{
	/* load in,
	 * &-out where we want our value (to 0),
	 * &-out our value (so we don't affect other bits),
	 * |-in our new value,
	 * store
	 */

	/* we get the lvalue type - change to pointer */
	type *const ty = vtop[-1].t, *ty_ptr = type_ptr_to(ty);
	unsigned long mask_leading_1s, mask_back_0s, mask_rm;

	/* coerce vtop to a vtop[-1] type */
	out_cast(vtop[-1].t, 0);

	/* load the pointer to the store, forgetting the bitfield */
	/* stack: store, val */
	out_swap();
	out_change_type(ty_ptr); /* XXX: memleak */
	out_dup();
	vtop->bitfield.nbits = 0;
	/* stack: val, store, store-less-bitfield */

	/* load the bitfield without using bitfield semantics */
	out_deref();
	/* stack: val, store, orig-val */
	out_pulltop(2);
	/* stack: store, orig-val, val */
	out_swap();
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
	out_push_l(ty, mask_rm);
	out_comment("bitmask/rm = %#lx", mask_rm);
	out_op(op_and);

	/* stack: store, val, orig-val-masked */
	/* bring the value to the top */
	out_swap();

	/* mask and shift our value up */
	out_push_l(ty, ~(-1UL << bf->nbits));
	out_op(op_and);

	out_push_l(ty, bf->off);
	out_op(op_shiftl);

	/* | our value in with the dereferenced store value */
	out_op(op_or);
}

void out_store()
{
	struct vstack *store, *val;

	val   = &vtop[0];
	store = &vtop[-1];

	store->t = type_pointed_to(store->t);

	v_store(val, store);

	/* swap, popping the store, but not the value */
	vswap();
	vpop();
}

void v_store(struct vstack *val, struct vstack *store)
{
	if(store->bitfield.nbits){
		UCC_ASSERT(val == vtop && store == vtop-1,
				"TODO: bitfield merging for non-vtops");

		bitfield_scalar_merge(&store->bitfield);
	}

	v_flush_volatile_reg(val); /* flush offset on any registers
	                            * only for the value we're assigning */
	impl_store(val, store);
}

void out_push_sym(sym *s)
{
	decl *const d = s->decl;

	vpush(type_ptr_to(d->ref));

	switch(s->type){
		case sym_local:
			if(type_is(d->ref, type_func))
				goto label;

			if((d->store & STORE_MASK_STORE) == store_register && d->spel_asm)
				ICW("TODO: %s asm(\"%s\")", decl_to_str(d), d->spel_asm);

			/* sym offsetting takes into account the stack growth direction */
			v_set_stack(
					vtop, NULL,
					-(long)(s->loc.stack_pos + stack_local_offset),
					/*lval:*/1);
			break;

		case sym_arg:
			v_set_stack(
					vtop, NULL, s->loc.arg_offset, /*lval:*/1);
			break;

		case sym_global:
label:
			out_set_lbl(decl_asm_spel(d), /*pic:*/1);
			break;
	}

	out_set_lvalue();
}

void out_push_sym_val(sym *s)
{
	out_push_sym(s);
	out_deref();
}

static int calc_ptr_step(type *t)
{
	/* we are calculating the sizeof *t */
	int mul;

	if(type_is_primitive(type_is_ptr(t), type_void))
		return type_primitive_size(type_void);

	if(type_is_primitive(t, type_unknown))
		mul = 1;
	else
		mul = type_size(type_next(t), NULL);

	return mul;
}

void out_op(enum op_type op)
{
	/*
	 * the implementation does a vpop() and
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
		case V_REG_SAVE:        \
			if(!vp.is_lval)       \
				break;              \
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
			case V_LBL:
				p = &t_mem_reg->bits.lbl.offset;
				break;

			case V_REG_SAVE:
			case V_REG:
				p = &t_mem_reg->bits.regoff.offset;
				break;

			default:
				ucc_unreach();
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
					vswap(); /* need t_const on top for discarding */
				vpop();
				goto out;
		}

		/* constant folding */
		if(((t_const == vtop ? &vtop[-1] : vtop)->type) == V_CONST_I){
			const char *err = NULL;
			const integral_t eval = const_op_exec(
					vtop[-1].bits.val_i, &vtop->bits.val_i,
					op, type_is_signed(vtop->t),
					&err);

			if(!err){
				vpop();
				vtop->type = V_CONST_I;
				vtop->bits.val_i = eval;
				goto out;
			}
		}
	}

	{
		int div = 0;

		switch(op){
			case op_plus:
			case op_minus:
			{
				int l_ptr = !!type_is(vtop[-1].t, type_ptr),
				    r_ptr = !!type_is(vtop->t   , type_ptr);

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
								v_to_reg(val);

							case V_REG:
							{
								int swap;
								if((swap = (val != vtop)))
									vswap();

								out_push_l(type_nav_btype(cc1_type_nav, type_intptr_t), ptr_step);
								out_op(op_multiply);

								if(swap)
									vswap();
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

		impl_op(op);

		if(div){
			out_push_l(type_ptr_to(type_nav_btype(cc1_type_nav, type_void)), div);
			out_op(op_divide);
		}
	}
out:;
}

static void bitfield_to_scalar(const struct vbitfield *bf)
{
	type *const ty = vtop->t;

	/* shift right, then mask */
	out_push_l(ty, bf->off);
	out_op(op_shiftr);

	out_push_l(ty, ~(-1UL << bf->nbits));
	out_op(op_and);

	/* if it's signed we need to sign extend
	 * using a signed right shift to copy its MSB
	 */
	if(type_is_signed(ty)){
		const unsigned ty_sz = type_size(ty, NULL);
		const unsigned nshift = CHAR_BIT * ty_sz - bf->nbits;

		out_push_l(ty, nshift);
		out_op(op_shiftl);
		out_push_l(ty, nshift);
		out_op(op_shiftr);
	}
}

void v_deref_decl(struct vstack *vp)
{
	/* XXX: memleak */
	vp->t = type_pointed_to(vp->t);
}

void out_deref()
{
	const struct vbitfield bf = vtop->bitfield;
#define DEREF_CHECK
#ifdef DEREF_CHECK
	type *const vtop_t = vtop->t;
#endif
	type *indir;
	int fp;

	indir = type_pointed_to(vtop->t);

	/* if the pointed-to object is not an lvalue, don't deref */
	if(type_is(indir, type_array)
	|| type_is(type_is_ptr(vtop->t), type_func)){
		out_change_type(indir);
		return; /* noop */
	}

	v_deref_decl(vtop);
	fp = type_is_floating(vtop->t);

	switch(vtop->type){
		case V_FLAG:
			ICE("deref of flag");
		case V_CONST_F:
			ICE("deref of float");

		case V_REG:
			/* attempt to convert to v_reg_save if possible */
			if(vtop->is_lval && v_reg_is_const(&vtop->bits.regoff.reg)){
				vtop->type = V_REG_SAVE;
				break;
			}
		case V_REG_SAVE:
		case V_LBL:
		case V_CONST_I:
		{
			struct vreg r;

			v_unused_reg(1, fp, &r);

			impl_deref(vtop, &r, indir);

			v_set_reg(vtop, &r);
			break;
		}
	}

#ifdef DEREF_CHECK
	UCC_ASSERT(vtop_t != vtop->t, "no depth change");
#endif

	if(bf.nbits)
		bitfield_to_scalar(&bf);
}


void out_op_unary(enum op_type op)
{
	switch(op){
		case op_plus:
			return;

		default:
			/* special case - reverse the flag if possible */
			switch(vtop->type){
				case V_FLAG:
					if(op == op_not){
						v_inv_cmp(&vtop->bits.flag, 1);
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

	impl_op_unary(op);
}

void out_change_type(type *t)
{
	impl_change_type(t);
}

void out_jmp(void)
{
	if(vtop > vstack){
		/* flush the stack-val we need to generate before the jump */
		v_flush_volatile(vtop - 1);
	}

	impl_jmp();
	vpop();
}

void out_jtrue(const char *lbl)
{
	impl_jcond(1, lbl);

	vpop();
}

void out_jfalse(const char *lbl)
{
	int cond = 0;

	if(vtop->type == V_FLAG){
		v_inv_cmp(&vtop->bits.flag, 1);
		cond = 1;
	}

	impl_jcond(cond, lbl);

	vpop();
}

static void out_label2(const char *lbl, int affect_code)
{
	UCC_ASSERT(lbl, "no label");

	if(affect_code){
		/* if we have volatile data, ensure it's in a register */
		out_flush_volatile();
	}

	impl_lbl(lbl);
}

void out_label(const char *lbl)
{
	out_label2(lbl, 1);
}

void out_label_noop(const char *lbl)
{
	out_label2(lbl, 0);
}

static void out_comment_vsec(
		enum section_type sec, const char *fmt, va_list l)
{
	impl_comment(sec, fmt, l);
}

void out_comment_sec(enum section_type sec, const char *fmt, ...)
{
	va_list l;

	va_start(l, fmt);
	out_comment_vsec(sec, fmt, l);
	va_end(l);
}

void out_comment(const char *fmt, ...)
{
	va_list l;

	va_start(l, fmt);
	out_comment_vsec(SECTION_TEXT, fmt, l);
	va_end(l);
}

void out_pop_func_ret(type *t)
{
	impl_pop_func_ret(t);
}

void out_undefined(void)
{
	out_flush_volatile();
	impl_undefined();
}

void out_push_overflow(void)
{
	vpush(type_nav_btype(cc1_type_nav, type__Bool));
	impl_set_overflow();
}

void out_push_frame_ptr(int nframes)
{
	/* XXX: memleak */
	struct vreg r;
	type *const void_pp = type_ptr_to(
			type_ptr_to(type_nav_btype(cc1_type_nav, type_void)));

	vpush(void_pp);
	v_set_reg_i(vtop, REG_BP);

	if(nframes > 1){
		v_unused_reg(1, 0, &r);
		impl_reg_cp(vtop, &r);
		v_set_reg(vtop, &r);
	}

	while(--nframes > 0)
		impl_deref(vtop, &r, void_pp); /* movq (<reg>), <reg> */

	/* force to void * */
	out_change_type(type_pointed_to(void_pp));
}

void out_push_reg_save_ptr(void)
{
	out_flush_volatile();

	vpush(type_ptr_to(type_nav_btype(cc1_type_nav, type_void)));
	v_set_stack(vtop, NULL, -stack_variadic_offset, /*lval:*/0);
}

void out_push_nan(type *ty)
{
	UCC_ASSERT(type_is_floating(ty),
			"non-float nan?");
	vpush(ty);
	impl_set_nan(ty);
}
#endif

#include "out_ctrl.c"
#include "out_new.c"
#include "out_func.c"

out_ctx *out_ctx_new(void)
{
	out_ctx *ctx = umalloc(sizeof *ctx);
	return ctx;
}

void out_ctx_end(out_ctx *octx)
{
	/* TODO */
	(void)octx;
}

out_blk *out_blk_new(const char *desc)
{
	out_blk *blk = umalloc(sizeof *blk);
	blk->desc = desc;
	return blk;
}

void out_comment(const char *fmt, ...)
{
	/* TODO */
	(void)fmt;
}

out_val *out_cast(out_ctx *octx, out_val *val, type *to, int normalise_bool)
{
	type *const from = val->t;
	char fp[2];

	/* normalise before the cast, otherwise we do things like
	 * 5.3 -> 5, then normalise 5, instead of 5.3 != 0.0
	 */
	if(normalise_bool && type_is_primitive(to, type__Bool))
		val = out_normalise(octx, val);

	fp[0] = type_is_floating(from);
	fp[1] = type_is_floating(to);

	if(fp[0] || fp[1]){
		if(fp[0] && fp[1]){

			if(type_primitive(from) != type_primitive(to))
				val = impl_f2f(octx, val, from, to);
			/* else no-op cast */

		}else if(fp[0]){
			/* float -> int */
			UCC_ASSERT(type_is_integral(to),
					"fp to %s?", type_to_str(to));

			val = impl_f2i(octx, val, from, to);

		}else{
			/* int -> float */
			UCC_ASSERT(type_is_integral(from),
					"%s to fp?", type_to_str(from));

			val = impl_i2f(octx, val, from, to);
		}

	}else{
		/* casting integral vtop
		 * don't bother if it's a constant,
		 * just change the size */
		if(val->type != V_CONST_I){
			int szfrom = asm_type_size(from),
					szto   = asm_type_size(to);

			if(szfrom != szto){
				if(szto > szfrom){
					/* we take from's signedness for our sign-extension,
					 * e.g. uint64_t x = (int)0x8000_0000;
					 * sign extends the int to an int64_t, then changes
					 * the type
					 */
					val = impl_cast_load(octx, val, from, to,
							type_is_signed(from));
				}else{
					char buf[TYPE_STATIC_BUFSIZ];

					out_comment("truncate cast from %s to %s, size %d -> %d",
							from ? type_to_str_r(buf, from) : "",
							to   ? type_to_str(to) : "",
							szfrom, szto);
				}
			}
		}
	}

	return val;
}

out_val *out_change_type(out_ctx *octx, out_val *val, type *ty)
{
	out_val *new = outval_new(octx, val);
	new->t = ty;
	return new;
}

out_val *out_deref(out_ctx *octx, out_val *target)
{
	return impl_deref(octx, target);
}

out_val *out_normalise(out_ctx *octx, out_val *unnormal)
{
	out_val *normalised = outval_new(octx, unnormal);

	switch(unnormal->type){
		case V_FLAG:
			/* already normalised */
			break;

		case V_CONST_I:
			normalised->bits.val_i = !!normalised->bits.val_i;
			break;

		case V_CONST_F:
			normalised->bits.val_i = !!normalised->bits.val_f;
			normalised->type = V_CONST_I;
			break;

		default:
			v_to_reg(normalised);
			/* fall */

		case V_REG:
		{
			out_val *z = out_new_zero(octx, normalised->t);

			normalised = out_op(octx, op_ne, z, unnormal);
			/* 0 -> `0 != 0` = 0
			 * 1 -> `1 != 0` = 1
			 * 5 -> `5 != 0` = 1
			 */
		}
	}

	return normalised;
}

out_val *out_op(out_ctx *octx, enum op_type binop, out_val *lhs, out_val *rhs)
{
	return impl_op(octx, binop, lhs, rhs);
}

out_val *out_op_unary(out_ctx *octx, enum op_type uop, out_val *exp)
{
	return impl_op_unary(octx, uop, exp);
}

void out_set_bitfield(
		out_ctx *octx, out_val *val,
		unsigned off, unsigned nbits)
{
	(void)octx;
	val->bitfield.off = off;
	val->bitfield.nbits = nbits;
}

void out_store(out_ctx *octx, out_val *dest, out_val *val)
{
	impl_store(octx, dest, val);
}
