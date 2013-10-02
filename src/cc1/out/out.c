#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "../../util/util.h"
#include "../../util/alloc.h"
#include "../data_structs.h"
#include "out.h"
#include "vstack.h"
#include "impl.h"
#include "../../util/platform.h"
#include "../cc1.h"
#include "asm.h"
#include "../pack.h"
#include "../defs.h"
#include "../const.h"

#define v_check_type(t) if(!t) t = type_ref_cached_VOID_PTR()

typedef char chk[OUT_VPHI_SZ == sizeof(struct vstack) ? 1 : -1];

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
static unsigned char reserved_regs[N_SCRATCH_REGS];

int out_vcount(void)
{
	return vtop ? 1 + (int)(vtop - vstack) : 0;
}

static void vpush(type_ref *t)
{
	v_check_type(t);

	if(!vtop){
		vtop = vstack;
	}else{
		UCC_ASSERT(vtop < vstack + N_VSTACK - 1,
				"vstack overflow, vtop=%p, vstack=%p, diff %d",
				(void *)vtop, (void *)vstack, out_vcount());

		if(vtop->type == FLAG)
			v_to_reg(vtop);

		vtop++;
	}

	v_clear(vtop, t);
}

void v_clear(struct vstack *vp, type_ref *t)
{
	v_check_type(t);

	memset(vp, 0, sizeof *vp);
	vp->t = t;
}

void vpop(void)
{
	UCC_ASSERT(vtop, "NULL vtop for vpop");

	if(vtop == vstack){
		vtop = NULL;
	}else{
		UCC_ASSERT(vtop < vstack + N_VSTACK - 1, "vstack underflow");
		vtop--;
	}
}

void out_phi_pop_to(void *vvphi)
{
	struct vstack *const vphi = vvphi;

	/* put the current value into the phi-save area */
	memcpy_safe(vphi, vtop);

	if(vphi->type == REG)
		v_reserve_reg(vphi->bits.reg); /* XXX: watch me */

	out_pop();
}

void out_phi_join(void *vvphi)
{
	struct vstack *const vphi = vvphi;

	if(vphi->type == REG)
		v_unreserve_reg(vphi->bits.reg); /* XXX: voila */

	/* join vtop and the current phi-save area */
	v_to_reg(vtop);
	v_to_reg(vphi);

	if(vtop->bits.reg != vphi->bits.reg){
		/* _must_ match vphi, since it's already been generated */
		impl_reg_cp(vtop, vphi->bits.reg);
		memcpy_safe(vtop, vphi);
	}
}

void out_flush_volatile(void)
{
	if(vtop)
		v_to_reg(vtop);
}

void out_assert_vtop_null(void)
{
	UCC_ASSERT(!vtop, "vtop not null (%d entries)", out_vcount());
}

void out_dump(void)
{
	int i;

	for(i = 0; &vstack[i] <= vtop; i++)
		fprintf(stderr,
				"vstack[%d] = { .type = %d, .reg = %d, "
				".bitfield = { .nbits = %u, .off = %u } }\n",
				i, vstack[i].type, vstack[i].bits.reg,
				vstack[i].bitfield.nbits, vstack[i].bitfield.off);
}

void vswap(void)
{
	struct vstack tmp;
	memcpy_safe(&tmp, vtop);
	memcpy_safe(vtop, &vtop[-1]);
	memcpy_safe(&vtop[-1], &tmp);
}

void out_swap(void)
{
	vswap();
}

void v_to_reg_const(struct vstack *vp)
{
	switch(vp->type){
		case STACK:
		case LBL:
			/* need to pull the values from the stack */

		case STACK_SAVE:
			/* ditto, impl handles pulling from stack */

		case FLAG:
			/* obviously can't have a flag in cmp/mov code */
			v_to_reg(vp);

		case REG:
		case CONST:
			break;
	}
}

int v_unused_reg(int stack_as_backup)
{
	static unsigned char used[N_SCRATCH_REGS];
	struct vstack *it, *first;
	int i;

	memcpy(used, reserved_regs, sizeof used);
	first = NULL;

	for(it = vstack; it <= vtop; it++){
		if(it->type == REG){
			if(!first)
				first = it;
			used[impl_reg_to_scratch(it->bits.reg)] = 1;
		}
	}

	for(i = 0; i < N_SCRATCH_REGS; i++)
		if(!used[i])
			return impl_scratch_to_reg(i);

	if(stack_as_backup){
		/* no free regs, move `first` to the stack and claim its reg */
		int reg = first->bits.reg;

		v_freeup_regp(first);

		return reg;
	}
	return -1;
}

void v_to_reg2(struct vstack *from, int reg)
{
	type_ref *const save = from->t;
	int lea = 0;

	switch(from->type){
		case STACK:
		case LBL:
			lea = 1;
		case FLAG:
		case STACK_SAVE: /* voila */
		case CONST:
		case REG:
			break;
	}

	(lea ? impl_lea : impl_load)(from, reg);

	v_clear(from, save);
	from->type = REG;
	from->bits.reg = reg;
}

int v_to_reg(struct vstack *conv)
{
	if(conv->type != REG)
		v_to_reg2(conv, v_unused_reg(1));

	return conv->bits.reg;
}

static struct vstack *v_find_reg(int reg)
{
	struct vstack *vp;
	if(!vtop)
		return NULL;

	for(vp = vstack; vp <= vtop; vp++)
		if(vp->type == REG && vp->bits.reg == reg)
			return vp;

	return NULL;
}

void v_freeup_regp(struct vstack *vp)
{
	/* freeup this reg */
	int r;

	UCC_ASSERT(vp->type == REG, "not reg");

	/* attempt to save to a register first */
	r = v_unused_reg(0);

	if(r >= 0){
		impl_reg_cp(vp, r);

		v_clear(vp, vp->t);
		vp->type = REG;
		vp->bits.reg = r;

	}else{
		v_save_reg(vp);
	}
}

static int v_alloc_stack(int sz)
{
	static int word_size;
	/* sz must be a multiple of word_size */

	if(!word_size)
		word_size = platform_word_size();

	if(sz){
		/* sz must be a multiple of mstack_align */
		sz = pack_to_align(sz, cc1_mstack_align);

		vpush(NULL);
		vtop->type = REG;
		vtop->bits.reg = REG_SP;

		out_push_i(type_ref_cached_INTPTR_T(), sz);
		out_op(op_minus);
		out_pop();
	}

	return stack_sz += sz;
}

void v_save_reg(struct vstack *vp)
{
	struct vstack store;

	UCC_ASSERT(vp->type == REG, "not reg");

	memset(&store, 0, sizeof store);

	store.type = STACK;
	store.t = type_ref_ptr_depth_inc(vp->t);

	/* the following gen two instructions - subq and movq
	 * instead/TODO: impl_save_reg(vp) -> "pushq %%rax"
	 * -O1?
	 */
	store.bits.off_from_bp = -v_alloc_stack(type_ref_size(store.t, NULL));
#ifdef DEBUG_REG_SAVE
	out_comment("save register %d", vp->bits.reg);
#endif
	impl_store(vp, &store);

	store.type = STACK_SAVE;

	memcpy_safe(vp, &store);

	vp->t = type_ref_ptr_depth_dec(vp->t, NULL);
}

void v_freeup_reg(int r, int allowable_stack)
{
	struct vstack *vp = v_find_reg(r);

	if(vp && vp < &vtop[-allowable_stack + 1])
		v_freeup_regp(vp);
}

static void v_alter_reservation(int r, int n)
{
	r = impl_reg_to_scratch(r);
	if(0 <= r && r < N_SCRATCH_REGS)
		reserved_regs[r] += n;
}

void v_reserve_reg(int r)
{
	v_alter_reservation(r, +1);
}

void v_unreserve_reg(int r)
{
	v_alter_reservation(r, -1);
}

void v_freeup_regs(const int a, const int b)
{
	v_reserve_reg(a);
	v_reserve_reg(b);

	v_freeup_reg(a, 2);
	v_freeup_reg(b, 2);

	v_unreserve_reg(a);
	v_unreserve_reg(b);
}

void v_inv_cmp(struct vstack *vp)
{
	switch(vp->bits.flag.cmp){
#define OPPOSITE2(from, to) \
		case flag_ ## from: vp->bits.flag.cmp = flag_ ## to; return

#define OPPOSITE(from, to) OPPOSITE2(from, to); OPPOSITE2(to, from)

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

void out_pop(void)
{
	vpop();
}

void out_push_iv(type_ref *t, intval *iv)
{
	vpush(t);

	vtop->type = CONST;
	vtop->bits.val = iv->val;
	impl_load_iv(vtop);
}

void out_push_i(type_ref *t, int i)
{
	intval iv = {
		.val = i,
		.suffix = 0
	};

	out_push_iv(t, &iv);
}

static void out_set_lbl(const char *s, int pic)
{
	vtop->type = LBL;

	vtop->bits.lbl.str = s;
	vtop->bits.lbl.pic = pic;
	vtop->bits.lbl.offset = 0;
}

void out_push_lbl(const char *s, int pic)
{
	vpush(NULL);

	out_set_lbl(s, pic);
}

void out_push_noop()
{
	out_push_i(type_ref_cached_INTPTR_T(), 0);
}

void out_dup(void)
{
	/* TODO: mark reg as duped, but COW */
	out_comment("dup");
	vpush(NULL);
	switch(vtop[-1].type){
		case CONST:
		case STACK:
		case STACK_SAVE:
		case LBL:
			/* fine */
			memcpy_safe(&vtop[0], &vtop[-1]);
			break;
		case FLAG:
			v_to_reg(&vtop[-1]);
			/* fall */
		case REG:
		{
			/* need a new reg */
			int r = v_unused_reg(1);
			impl_reg_cp(&vtop[-1], r);

			vtop->type = REG;
			vtop->bits.reg = r;
			vtop->t = vtop[-1].t;

			break;
		}
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

	type_ref *const ty = type_ref_ptr_depth_dec(vtop[-1].t, NULL);
	unsigned long mask_leading_1s, mask_back_0s, mask_rm;

	/* coerce vtop to a vtop[-1] type */
	out_cast(ty);

	/* load the pointer to the store, forgetting the bitfield */
	/* stack: store, val */
	out_swap();
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
	out_push_i(ty, mask_rm);
	out_comment("bitmask/rm = %#lx", mask_rm);
	out_op(op_and);

	/* stack: store, val, orig-val-masked */
	/* bring the value to the top */
	out_swap();

	/* mask and shift our value up */
	out_push_i(ty, ~(-1UL << bf->nbits));
	out_op(op_and);

	out_push_i(ty, bf->off);
	out_op(op_shiftl);

	/* | our value in with the dereferenced store value */
	out_op(op_or);
}

void out_store()
{
	struct vstack *store, *val;

	val   = &vtop[0];
	store = &vtop[-1];

	if(store->bitfield.nbits)
		bitfield_scalar_merge(&store->bitfield);

	impl_store(val, store);

	/* swap, popping the store, but not the value */
	vswap();
	vpop();
}

void out_normalise(void)
{
	switch(vtop->type){
		case CONST:
			vtop->bits.val = !!vtop->bits.val;
			break;

		default:
			v_to_reg(vtop);

		case REG:
			out_comment("normalise");

			out_push_i(vtop->t, 0);
			out_op(op_ne);
			/* 0 -> `0 != 0` = 0
			 * 1 -> `1 != 0` = 1
			 * 5 -> `5 != 0` = 1
			 */
	}
}

void out_push_sym(sym *s)
{
	decl *const d = s->decl;

	vpush(type_ref_ptr_depth_inc(d->ref));

	switch(s->type){
		case sym_local:
			if(DECL_IS_FUNC(d))
				goto label;

			if((d->store & STORE_MASK_STORE) == store_register && d->spel_asm)
				ICW("TODO: %s asm(\"%s\")", decl_to_str(d), d->spel_asm);

			vtop->type = STACK;
			/* sym offsetting takes into account the stack growth direction */
			vtop->bits.off_from_bp = -(s->offset + stack_local_offset);
			break;

		case sym_arg:
			vtop->type = STACK;
			{
				int off;

				if(s->offset < N_CALL_REGS){
					/* in saved_regs area */
					off = -(s->offset + 1);
				}else{
					/* in extra area */
					off = s->offset - N_CALL_REGS + 2;
				}

				vtop->bits.off_from_bp = platform_word_size() * off;
				break;
			}

		case sym_global:
label:
			out_set_lbl(decl_asm_spel(d), 1);
			break;
	}
}

void out_push_sym_val(sym *s)
{
	out_push_sym(s);
	out_deref();
}

static int calc_ptr_step(type_ref *t)
{
	/* we are calculating the sizeof *t */
	int sz;

	if(type_ref_is_type(type_ref_is_ptr(t), type_void))
		return type_primitive_size(type_void);

	sz = type_ref_size(type_ref_next(t), &t->where);

	return sz;
}

void out_op(enum op_type op)
{
	/*
	 * the implementation does a vpop() and
	 * sets vtop sufficiently to describe how
	 * the result is returned
	 */

	struct vstack *t_const = NULL, *t_mem = NULL;

	/* check for adding or subtracting to stack */
#define POPULATE_TYPE(vp) \
	switch(vp.type){        \
		case CONST:           \
			t_const = &vp;      \
			break;              \
		case STACK:           \
		case LBL:             \
			t_mem = &vp;        \
		default:              \
			break;              \
	}

	POPULATE_TYPE(vtop[0]);
	POPULATE_TYPE(vtop[-1]);

	if((op == op_plus || op == op_minus) && t_const && t_mem){
		/* t_const == vtop... should be */
		long *p = t_mem->type == STACK
			? &t_mem->bits.off_from_bp
			: &t_mem->bits.lbl.offset;

		*p += (op == op_minus ? -1 : 1) *
			t_const->bits.val *
			calc_ptr_step(t_mem->t);

		goto pop_const;

	}else if(t_const){
		switch(op){
			case op_plus:
			case op_minus:
			case op_or:
			case op_xor:
			case op_shiftl: /* if we're shifting 0, or shifting _by_ zero, noop */
			case op_shiftr:
				if(t_const->bits.val == 0)
					goto pop_const;
			default:
				break;

			case op_multiply:
			case op_divide:
				if(t_const->bits.val == 1)
					goto pop_const;
				break;

			case op_and:
				if((sintval_t)t_const->bits.val == -1)
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
				return;
		}

		/* constant folding */
		if((t_const == vtop ? &vtop[-1] : vtop)->type == CONST){
			const char *err = NULL;
			const intval_t eval = const_op_exec(
					vtop[-1].bits.val, &vtop->bits.val,
					op, type_ref_is_signed(vtop->t),
					&err);

			UCC_ASSERT(!err, "const op err %s", err);

			vpop();
			vtop->type = CONST;
			vtop->bits.val = eval;
			return;
		}
	}

	{
		int div = 0;

		switch(op){
			case op_plus:
			case op_minus:
			{
				int l_ptr, r_ptr;

				l_ptr = !!type_ref_is(vtop->t   , type_ref_ptr);
				r_ptr = !!type_ref_is(vtop[-1].t, type_ref_ptr);

				if(l_ptr || r_ptr){
					const int ptr_step = calc_ptr_step(l_ptr ? vtop->t : vtop[-1].t);

					if(l_ptr ^ r_ptr){
						/* ptr +/- int, adjust the non-ptr by sizeof *ptr */
						struct vstack *val = &vtop[l_ptr ? -1 : 0];

						switch(val->type){
							case CONST:
								val->bits.val *= ptr_step;
								break;

							default:
								v_to_reg(val);

							case REG:
							{
								int swap;
								if((swap = (val != vtop)))
									vswap();

								out_push_i(type_ref_cached_VOID_PTR(), ptr_step);
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
			out_push_i(type_ref_cached_VOID_PTR(), div);
			out_op(op_divide);
		}
	}
}

void out_set_bitfield(unsigned off, unsigned nbits)
{
	vtop->bitfield.off   = off;
	vtop->bitfield.nbits = nbits;
}

static void bitfield_to_scalar(const struct vbitfield *bf)
{
	type_ref *const ty = vtop->t;

	/* shift right, then mask */
	out_push_i(ty, bf->off);
	out_op(op_shiftr);

	out_push_i(ty, ~(-1UL << bf->nbits));
	out_op(op_and);

	/* if it's signed we need to sign extend
	 * using a signed right shift to copy its MSB
	 */
	if(type_ref_is_signed(ty)){
		const unsigned ty_sz = type_ref_size(ty, NULL);
		const unsigned nshift = CHAR_BIT * ty_sz - bf->nbits;

		out_push_i(ty, nshift);
		out_op(op_shiftl);
		out_push_i(ty, nshift);
		out_op(op_shiftr);
	}
}

void v_deref_decl(struct vstack *vp)
{
	/* XXX: memleak */
	vp->t = type_ref_ptr_depth_dec(vp->t, NULL);
}

void out_deref()
{
	const struct vbitfield bf = vtop->bitfield;
#define DEREF_CHECK
#ifdef DEREF_CHECK
	type_ref *const vtop_t = vtop->t;
#endif
	type_ref *indir;
	/* if the pointed-to object is not an lvalue, don't deref */

	indir = type_ref_ptr_depth_dec(vtop->t, NULL);

	if(type_ref_is(indir, type_ref_array)
	|| type_ref_is(type_ref_is_ptr(vtop->t), type_ref_func)){
		out_change_type(indir);
		return; /* noop */
	}

	/* optimisation: if we're dereffing a pointer to stack/lbl, just do a mov */
	switch(vtop->type){
		case FLAG:
			ICE("deref of flag");

		default:
			v_to_reg(vtop);
		case REG:
			impl_deref_reg();
			break;

		case LBL:
		case STACK:
		case CONST:
		{
			int r = v_unused_reg(1);

			v_deref_decl(vtop);

			/* impl_load, since we don't want a lea switch */
			impl_load(vtop, r);

			vtop->type = REG;
			vtop->bits.reg = r;
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
				case FLAG:
					if(op == op_not){
						v_inv_cmp(vtop);
						return;
					}
					break;

				case CONST:
					switch(op){
#define OP(t, o) case op_ ## t: vtop->bits.val = o vtop->bits.val; return
						OP(not, !);
						OP(minus, -);
						OP(bnot, ~);
#undef OP

						default:
							ICE("invalid unary op");
					}
					break;

				case REG:
				case STACK:
				case STACK_SAVE:
				case LBL:
					break;
			}
	}

	impl_op_unary(op);
}

void out_cast(type_ref *to)
{
	v_cast(vtop, to);
}

void v_cast(struct vstack *vp, type_ref *to)
{
	/* casting vtop - don't bother if it's a constant, just change the size */
	if(vp->type != CONST){
		type_ref *from = vp->t;

		int szfrom = asm_type_size(from),
				szto   = asm_type_size(to);

		if(szfrom != szto){
			if(szto > szfrom){
				impl_cast_load(vp, from, to,
						type_ref_is_signed(from));
			}else{
				char buf[TYPE_REF_STATIC_BUFSIZ];

				out_comment("truncate cast from %s to %s, size %d -> %d",
						from ? type_ref_to_str_r(buf, from) : "",
						to   ? type_ref_to_str(to) : "",
						szfrom, szto);
			}
		}
	}

	vp->t = to;
}

void out_change_type(type_ref *t)
{
	v_check_type(t);
	/* XXX: memleak */
	vtop->t = t;

	/* we can't change type for large integer values,
	 * they need truncating
	 */
	UCC_ASSERT(
			vtop->type != CONST
			|| !intval_is_64_bit(vtop->bits.val, vtop->t),
			"can't %s for large constant %" INTVAL_FMT_X, __func__, vtop->bits.val);
}

void v_save_regs()
{
	int i = 0;

	/* save all registers */
	for(; &vstack[i] < vtop; i++){
		if(i == 0)
			out_comment("pre-call reg-save");

		/* TODO: v_to_mem (__asm__ branch) */
		if(vstack[i].type == REG || vstack[i].type == FLAG)
			v_save_reg(&vstack[i]);
	}
}

void out_call(int nargs, type_ref *r_ret, type_ref *r_func)
{
	impl_call(nargs, r_ret, r_func);

	/* return type */
	v_clear(vtop, r_ret);
	vtop->type = REG;
	vtop->bits.reg = REG_RET;
}

void out_jmp(void)
{
	if(vtop > vstack){
		/* flush the stack-val we need to generate before the jump */
		vtop--;
		out_flush_volatile();
		vtop++;
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

	if(vtop->type == FLAG){
		v_inv_cmp(vtop);
		cond = 1;
	}

	impl_jcond(cond, lbl);

	vpop();
}

void out_label(const char *lbl)
{
	/* if we have volatile data, ensure it's in a register */
	out_flush_volatile();

	impl_lbl(lbl);
}

void out_comment(const char *fmt, ...)
{
	va_list l;

	va_start(l, fmt);
	impl_comment(fmt, l);
	va_end(l);
}

void out_func_prologue(int stack_res, int nargs, int variadic)
{
	UCC_ASSERT(stack_sz == 0, "non-empty stack for new func");

	stack_sz = MIN(nargs, N_CALL_REGS) * platform_word_size();

	impl_func_prologue_save_fp();
	impl_func_prologue_save_call_regs(nargs);

	if(variadic) /* save variadic call registers */
		stack_sz += impl_func_prologue_save_variadic(nargs);

	/* setup "pointers" to the right place in the stack */
	stack_variadic_offset = stack_sz;
	stack_local_offset    = stack_sz;

	if(stack_res)
		v_alloc_stack(stack_res);
}

void out_func_epilogue()
{
	impl_func_epilogue();
	stack_local_offset = stack_sz = 0;
}

void out_pop_func_ret(type_ref *t)
{
	impl_pop_func_ret(t);
}

int out_n_call_regs(void)
{
	return N_CALL_REGS;
}

void out_undefined(void)
{
	out_flush_volatile();
	impl_undefined();
}

void out_push_overflow(void)
{
	vpush(type_ref_cached_BOOL());
	impl_set_overflow();
}

void out_push_frame_ptr(int nframes)
{
	out_flush_volatile();

	vpush(NULL);
	vtop->type = REG;
	vtop->bits.reg = impl_frame_ptr_to_reg(nframes);
}

void out_push_reg_save_ptr(void)
{
	out_flush_volatile();

	vpush(NULL);
	vtop->type = STACK;
	vtop->bits.off_from_bp = -stack_variadic_offset;
}
