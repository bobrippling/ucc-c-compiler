#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "../../util/util.h"
#include "../../util/alloc.h"
#include "../../util/dynarray.h"
#include "../../util/platform.h"

#include "../op.h"
#include "../decl.h"
#include "../type.h"
#include "../type_is.h"
#include "../type_nav.h"
#include "../funcargs.h"
#include "../defs.h"

#include "../cc1.h"

#include "val.h"
#include "asm.h"
#include "impl.h"
#include "impl_jmp.h"
#include "common.h"
#include "out.h"
#include "lbl.h"
#include "write.h"
#include "virt.h"

#include "ctx.h"
#include "blk.h"

/* Darwin's `as' can only create movq:s with
 * immediate operands whose highest bit is bit
 * 31 or lower (i.e. up to UINT_MAX)
 */
#define AS_MAX_MOV_BIT 31

#define integral_high_bit_ABS(v, t) integral_high_bit(llabs(v), t)

#define NUM_FMT "%d"
/* format for movl $5, -0x6(%rbp) asm output
                        ^~~                    */

#define REG_STR_SZ 8

#define VSTACK_STR_SZ 128

const struct asm_type_table asm_type_table[ASM_TABLE_LEN] = {
	{ 1, "byte" },
	{ 2, "word" },
	{ 4, "long" },
	{ 8, "quad" },
};

/* TODO: each register has a class, smarter than this */
static const struct calling_conv_desc
{
	int caller_cleanup;

	unsigned n_call_regs;
	struct vreg call_regs[6 + 8];

	unsigned n_callee_save_regs;
	int callee_save_regs[6];
} calling_convs[] = {
	[conv_x64_sysv] = {
		1,
		6,
		{
			{ X86_64_REG_RDI, 0 },
			{ X86_64_REG_RSI, 0 },
			{ X86_64_REG_RDX, 0 },
			{ X86_64_REG_RCX, 0 },
			{ X86_64_REG_R8,  0 },
			{ X86_64_REG_R9,  0 },

			{ X86_64_REG_XMM0, 1 },
			{ X86_64_REG_XMM1, 1 },
			{ X86_64_REG_XMM2, 1 },
			{ X86_64_REG_XMM3, 1 },
			{ X86_64_REG_XMM4, 1 },
			{ X86_64_REG_XMM5, 1 },
			{ X86_64_REG_XMM6, 1 },
			{ X86_64_REG_XMM7, 1 },
		},
		6,
		{
			X86_64_REG_RBX,
			X86_64_REG_RBP,

			X86_64_REG_R12,
			X86_64_REG_R13,
			X86_64_REG_R14,
			X86_64_REG_R15
		}
	},

	[conv_x64_ms]   = {
		1,
		4,
		{
			{ X86_64_REG_RCX, 0 },
			{ X86_64_REG_RDX, 0 },
			{ X86_64_REG_R8,  0 },
			{ X86_64_REG_R9,  0 },
		}
	},

	[conv_cdecl] = {
		1,
		0,
		{
			{ 0 }
		},
		3,
		{
			X86_64_REG_RBX,
			X86_64_REG_RDI,
			X86_64_REG_RSI
		}
	},

	[conv_stdcall]  = { 0, 0 },

	[conv_fastcall] = {
		0,
		2,
		{
			{ X86_64_REG_RCX, 0 },
			{ X86_64_REG_RDX, 0 }
		}
	}
};

static const char *x86_intreg_str(unsigned reg, type *r)
{
	static const char *const rnames[][4] = {
#define REG(x) {  #x "l",  #x "x", "e"#x"x", "r"#x"x" }
		REG(a), REG(b), REG(c), REG(d),
#undef REG

		{ "dil",  "di", "edi", "rdi" },
		{ "sil",  "si", "esi", "rsi" },

		/* r[8 - 15] -> r8b, r8w, r8d,  r8 */
#define REG(x) {  "r" #x "b",  "r" #x "w", "r" #x "d", "r" #x  }
		REG(8),  REG(9),  REG(10), REG(11),
		REG(12), REG(13), REG(14), REG(15),
#undef REG

		{  "bpl", "bp", "ebp", "rbp" },
		{  "spl", "sp", "esp", "rsp" },
	};
#define N_REGS (sizeof rnames / sizeof *rnames)

	UCC_ASSERT(reg < N_REGS, "invalid x86 int reg %d", reg);

	return rnames[reg][asm_table_lookup(r)];
}

static const char *x86_fpreg_str(unsigned i)
{
	static const char *nams[] = {
		"xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"
	};

	UCC_ASSERT(i < sizeof nams/sizeof(*nams),
			"bad fp reg index %d", i);

	return nams[i];
}

static const char *x86_suffix(type *ty)
{
	if(type_is_floating(ty)){
		switch(type_primitive(ty)){
			case type_float:
				return "ss";
			case type_double:
				return "sd";
			case type_ldouble:
				ICE("TODO: ldouble");
			default:
				ICE("bad float");
		}
	}

	switch(ty ? type_size(ty, NULL) : 8){
		case 1: return "b";
		case 2: return "w";
		case 4: return "l";
		case 8: return "q";
	}
	ICE("no suffix for %s", type_to_str(ty));
}

static const char *x86_reg_str(const struct vreg *reg, type *r)
{
	/* must be sync'd with header */
	if(reg->is_float){
		return x86_fpreg_str(reg->idx);
	}else{
		return x86_intreg_str(reg->idx, r);
	}
}

static const char *vstack_str_r(
		char buf[VSTACK_STR_SZ], out_val *vs, const int deref)
{
	switch(vs->type){
		case V_CONST_I:
		{
			char *p = buf;
			/* we should never get a 64-bit value here
			 * since movabsq should load those in
			 */
			UCC_ASSERT(integral_high_bit_ABS(vs->bits.val_i, vs->t) < AS_MAX_MOV_BIT,
					"can't load 64-bit constants here (0x%llx)", vs->bits.val_i);

			if(deref == 0)
				*p++ = '$';

			integral_str(p, VSTACK_STR_SZ - (deref == 0 ? 1 : 0),
					vs->bits.val_i, vs->t);
			break;
		}

		case V_CONST_F:
			ICE("can't stringify float here");

		case V_FLAG:
			ICE("%s shouldn't be called with cmp-flag data", __func__);

		case V_LBL:
		{
			const int pic = fopt_mode & FOPT_PIC && vs->bits.lbl.pic;

			if(vs->bits.lbl.offset){
				SNPRINTF(buf, VSTACK_STR_SZ, "%s+%ld%s",
						vs->bits.lbl.str,
						vs->bits.lbl.offset,
						pic ? "(%rip)" : "");
			}else{
				SNPRINTF(buf, VSTACK_STR_SZ, "%s%s",
						vs->bits.lbl.str,
						pic ? "(%rip)" : "");
			}
			break;
		}

		case V_REG:
		case V_REG_SPILT:
		{
			long off = vs->bits.regoff.offset;
			const char *rstr = x86_reg_str(
					&vs->bits.regoff.reg, deref ? NULL : vs->t);

			if(off){
				UCC_ASSERT(deref,
						"can't add to a register in %s",
						__func__);

				SNPRINTF(buf, VSTACK_STR_SZ,
						"%s" NUM_FMT "(%%%s)",
						off < 0 ? "-" : "",
						abs(off),
						rstr);
			}else{
				SNPRINTF(buf, VSTACK_STR_SZ,
						"%s%%%s%s",
						deref ? "(" : "",
						rstr,
						deref ? ")" : "");
			}

			break;
		}
	}

	return buf;
}

static const char *vstack_str(out_val *vs, int deref)
{
	static char buf[VSTACK_STR_SZ];
	return vstack_str_r(buf, vs, deref);
}

int impl_reg_to_scratch(const struct vreg *r)
{
	return r->idx;
}

void impl_scratch_to_reg(int scratch, struct vreg *r)
{
	r->idx = scratch;
}

static const struct calling_conv_desc *x86_conv_lookup(type *fr)
{
	funcargs *fa = type_funcargs(fr);

	return &calling_convs[fa->conv];
}

static int x86_caller_cleanup(type *fr)
{
	const int cr_clean = x86_conv_lookup(fr)->caller_cleanup;

	if(!cr_clean && type_is_variadic_func(fr))
		die_at(type_loc(fr), "variadic functions can't be callee cleanup");

	return cr_clean;
}

static void x86_call_regs(
		type *fr, unsigned *pn,
		const struct vreg **par)
{
	const struct calling_conv_desc *ent = x86_conv_lookup(fr);
	*pn = ent->n_call_regs;
	if(par)
		*par = ent->call_regs;
}

static int x86_func_nargs(type *rf)
{
	return dynarray_count(type_funcargs(rf)->arglist);
}

int impl_reg_is_callee_save(const struct vreg *r, type *fr)
{
	const struct calling_conv_desc *ent;
	unsigned i;

	if(r->is_float)
		return 0;

	ent = x86_conv_lookup(fr);
	for(i = 0; i < ent->n_callee_save_regs; i++)
		if(ent->callee_save_regs[i] == r->idx){
			ICW("TODO: callee-save register saving");
			break;
			return 1;
		}

	return 0;
}

int impl_reg_frame_const(const struct vreg *r)
{
	return !r->is_float && r->idx == X86_64_REG_RBP;
}

int impl_reg_savable(const struct vreg *r)
{
	if(r->is_float)
		return 1;
	switch(r->idx){
		case X86_64_REG_RBP:
		case X86_64_REG_RSP:
			return 0;
	}
	return 1;
}

void impl_func_prologue_save_fp(out_ctx *octx)
{
	out_asm(octx, "pushq %%rbp");
	out_asm(octx, "movq %%rsp, %%rbp");
	/* a v_alloc_stack_n() is done later to align,
	 * but not interfere with argument locations */
}

void impl_func_prologue_save_call_regs(
		out_ctx *octx,
		type *rf, unsigned nargs,
		int arg_offsets[/*nargs*/])
{
	if(nargs){
		const unsigned ws = platform_word_size();

		funcargs *const fa = type_funcargs(rf);

		unsigned n_call_i, n_call_f;
		const struct vreg *call_regs;

		n_call_f = N_CALL_REGS_F;
		x86_call_regs(rf, &n_call_i, &call_regs);

		{
			/* trim by the number of args */
			unsigned fp_cnt, int_cnt;

			funcargs_ty_calc(fa, &int_cnt, &fp_cnt);

			n_call_i = MIN(n_call_i, int_cnt);
			n_call_f = MIN(n_call_f, fp_cnt);
		}

		/* two cases
		 * - for all integral arguments, we can just push them
		 * - if we have floats, we must mov them to the stack
		 * each argument takes a full word for now - subject to change
		 * (e.g. long double, struct/union args, etc)
		 */
		if(n_call_f){
			unsigned i_arg, i_stk, i_arg_stk, i_i, i_f;

			v_alloc_stack(octx,
					(n_call_f + n_call_i) * platform_word_size(),
					"save call regs float+integral");

			for(i_arg = i_i = i_f = i_stk = i_arg_stk = 0;
					i_arg < nargs;
					i_arg++)
			{
				type *const ty = fa->arglist[i_arg]->ref;
				const struct vreg *rp;
				struct vreg vr;

				if(type_is_floating(ty)){
					if(i_f >= n_call_f)
						goto pass_via_stack;

					rp = &vr;
					vr.is_float = 1;
					vr.idx = i_f++;
				}else{
					if(i_i >= n_call_i)
						goto pass_via_stack;

					rp = &call_regs[i_i++];
				}

				{
					int const off = ++i_stk * ws;

					v_reg_to_stack(octx, rp, ty, off);

					arg_offsets[i_arg] = -off;
				}

				continue;
pass_via_stack:
				arg_offsets[i_arg] = (i_arg_stk++ + 2) * ws;
			}
		}else{
			unsigned i;
			for(i = 0; i < nargs; i++){
				if(i < n_call_i){
					out_asm(octx, "push%s %%%s",
							x86_suffix(NULL),
							x86_reg_str(&call_regs[i], NULL));

					/* +1 to step over saved rbp */
					arg_offsets[i] = -(i + 1) * ws;
				}else{
					/* +2 to step back over saved rbp and saved rip */
					arg_offsets[i] = (i - n_call_i + 2) * ws;
				}
			}

			/* this aligns the stack too */
			v_alloc_stack_n(octx,
					n_call_i * platform_word_size(),
					"save call regs push-version");
		}
	}
}

void impl_func_prologue_save_variadic(out_ctx *octx, type *rf)
{
	const unsigned pws = platform_word_size();

	unsigned n_call_regs;
	const struct vreg *call_regs;

	type *const ty_dbl = type_nav_btype(cc1_type_nav, type_double);
	type *const ty_integral = type_nav_btype(cc1_type_nav, type_intptr_t);

	unsigned n_int_args, n_fp_args;

	unsigned stk_top;
	unsigned i;

	x86_call_regs(rf, &n_call_regs, &call_regs);

	funcargs_ty_calc(type_funcargs(rf), &n_int_args, &n_fp_args);

	/* space for all call regs */
	v_alloc_stack(octx,
			(N_CALL_REGS_I + N_CALL_REGS_F * 2) * platform_word_size(),
			"stack call arguments");

	stk_top = octx->stack_sz;

	/* go backwards, as we want registers pushed in reverse
	 * so we can iterate positively.
	 *
	 * note: we don't push call regs, just variadic ones after,
	 * hence >= n_args
	 */
	for(i = n_int_args; i < n_call_regs; i++){
		/* intergral call regs go _below_ floating */
		struct vreg vr;

		vr.is_float = 0;
		vr.idx = call_regs[i].idx;

		/* integral args are at the lowest address */
		v_reg_to_stack(octx, &vr, ty_integral,
				stk_top - i * pws);
	}

	{
#ifdef VA_SHORTCIRCUIT
		char *vfin = out_label_code("va_skip_float");
		type *const ty_ch = type_nav_btype(cc1_type_nav, type_nchar);

		/* testb %al, %al ; jz vfin */
		vpush(ty_ch);
		v_set_reg_i(vtop, X86_64_REG_RAX);
		out_push_zero(ty_ch);
		out_op(op_eq);
		out_jtrue(vfin);
#endif

		for(i = 0; i < N_CALL_REGS_F; i++){
			struct vreg vr;

			vr.is_float = 1;
			vr.idx = i;

			/* we go above the integral regs */
			v_reg_to_stack(octx, &vr, ty_dbl,
					stk_top - (i * 2 + n_call_regs) * pws);
		}

#ifdef VA_SHORTCIRCUIT
		out_label(vfin);
		free(vfin);
#endif
	}
}

void impl_func_epilogue(out_ctx *octx, type *rf)
{
	out_asm(octx, "leaveq");

	if(fopt_mode & FOPT_VERBOSE_ASM)
		out_comment("stack at %u bytes", octx->stack_sz);

	/* callee cleanup */
	if(!x86_caller_cleanup(rf)){
		const int nargs = x86_func_nargs(rf);

		out_asm(octx, "retq $%d", nargs * platform_word_size());
	}else{
		out_asm(octx, "retq");
	}
}

void impl_to_retreg(out_ctx *octx, out_val *val, type *retty)
{
	struct vreg r;

	r.idx =
		(r.is_float = type_is_floating(retty))
		? REG_RET_F
		: REG_RET_I;

	r.is_float = 0;

	/* v_to_reg since we don't handle lea/load ourselves */
	out_flush_volatile(octx, v_to_reg_given(octx, val, &r));
}

static const char *x86_cmp(struct flag_opts *flag)
{
	switch(flag->cmp){
#define OP(e, s, u)  \
		case flag_ ## e: \
		return flag->mods & flag_mod_signed ? s : u

		OP(eq, "e" , "e");
		OP(ne, "ne", "ne");
		OP(le, "le", "be");
		OP(lt, "l",  "b");
		OP(ge, "ge", "ae");
		OP(gt, "g",  "a");
#undef OP

		case flag_overflow: return "o";
		case flag_no_overflow: return "no";

		/*case flag_z:  return "z";
		case flag_nz: return "nz";*/
	}
	return NULL;
}

#if 0
void impl_load_iv(out_val *vp)
{
	const int high_bit = integral_high_bit_ABS(vp->bits.val_i, vp->t);

	if(high_bit >= AS_MAX_MOV_BIT){
		char buf[INTEGRAL_BUF_SIZ];
		struct vreg r;
		v_unused_reg(1, 0, &r);

		/* TODO: 64-bit registers in general on 32-bit */
		UCC_ASSERT(!IS_32_BIT(), "TODO: 32-bit 64-literal loads");

		if(high_bit > 31 /* not necessarily AS_MAX_MOV_BIT */){
			/* must be loading a long */
			if(type_size(vp->t, NULL) != 8){
				/* FIXME: enums don't auto-size currently */
				ICW("loading 64-bit literal (%lld) for non-8-byte type? (%s)",
						vp->bits.val_i, type_to_str(vp->t));
			}
		}

		integral_str(buf, sizeof buf, vp->bits.val_i, NULL);

		out_asm("movabsq $%s, %%%s", buf, x86_reg_str(&r, NULL));

		v_set_reg(vp, &r);
	}
}
#endif

#if 0
void impl_load_fp(out_val *from)
{
	/* if it's an int-const, we can load without a label */
	switch(from->type){
		case V_CONST_I:
			/* CONST_I shouldn't be entered,
			 * type prop. should cast */
			ICE("load int into float?");

		case V_CONST_F:
			if(from->bits.val_f == (integral_t)from->bits.val_f
			&& fopt_mode & FOPT_INTEGRAL_FLOAT_LOAD)
			{
				type *const ty_fp = from->t;

				from->type = V_CONST_I;
				from->bits.val_i = from->bits.val_f;
				/* TODO: use just an int if we can get away with it */
				from->t = type_nav_btype(cc1_type_nav, type_llong);

				out_cast(ty_fp, /*normalise_bool:*/1);
				break;
			}
			/* fall */

		default:
		{
			/* save to a label */
			char *lbl = out_label_data_store(STORE_FLOAT);
			struct vreg r;

			asm_nam_begin3(SECTION_DATA, lbl, type_align(from->t, NULL));
			asm_out_fp(SECTION_DATA, from->t, from->bits.val_f);

			v_clear(from, from->t);
			from->type = V_LBL;
			from->bits.lbl.str = lbl;
			from->bits.lbl.pic = 1;

			/* impl_load since we don't want a lea */
			v_unused_reg(1, 1, &r);
			impl_load(from, &r);

			v_set_reg(from, &r);

			free(lbl);
			break;
		}
	}
}
#endif

static int x86_need_fp_parity_p(
		struct flag_opts const *fopt, int *par_default)
{
	if(!(fopt->mods & flag_mod_float))
		return 0;

	/*
	 * for x86, we check the parity flag, if set we have a nan.
	 * ucomi* with a nan sets CF, PF and ZF
	 *
	 * we try to avoid checking PF by using seta instead of setb.
	 * 'seta' and 'setb' test the carry flag, which is 1 if we have a nan.
	 * setb => return CF
	 * seta => return !CF
	 *
	 * so 'seta' doesn't need a parity check, as it'll return false if we
	 * have a nan. 'setb' does.
	 */
	switch(fopt->cmp){
		case flag_ge:
		case flag_gt:
			/* can skip parity checks */
			return 0;

		case flag_ne:
			*par_default = 1; /* a != a is true if a == nan */
			/* fall */
		default:
			return 1;
	}
}

out_val *impl_load(out_ctx *octx, out_val *from, const struct vreg *reg)
{
	type *new_ty = NULL;

	if(from->type == V_REG
	&& vreg_eq(reg, &from->bits.regoff.reg))
	{
		return from;
	}

	switch(from->type){
		case V_FLAG:
		{
			out_val *vtmp_val;
			char *parity = NULL;
			int parity_default = 0;

			/* check float/orderedness */
			if(x86_need_fp_parity_p(&from->bits.flag, &parity_default))
				parity = out_label_code("parity");

			vtmp_val = out_new_l(octx, from->t, parity_default);

			impl_load(octx, vtmp_val, reg);

			if(parity)
				out_asm(octx, "jp %s", parity);

			/* force set%s to set the low byte */
			from->t = type_nav_btype(cc1_type_nav, type_nchar);
			new_ty = from->t;

			/* actual cmp */
			out_asm(octx, "set%s %%%s",
					x86_cmp(&from->bits.flag),
					x86_reg_str(reg, from->t));

			if(parity){
				/* don't use out_label - this does a vstack flush */
				impl_lbl(octx, parity);
				free(parity);
			}
			break;
		}

		case V_REG_SPILT:
			/* actually a pointer to T */
			return impl_deref(octx, from, reg);

		case V_REG:
			if(from->bits.regoff.offset)
				goto lea;
			/* fall */

		case V_CONST_I:
			out_asm(octx, "mov%s %s, %%%s",
					x86_suffix(from->t),
					vstack_str(from, 0),
					x86_reg_str(reg, from->t));

			new_ty = from->t;
			break;

lea:
		case V_LBL:
		{
			const int fp = type_is_floating(from->t);
			/* leab doesn't work as an instruction */
			type *suff_ty = fp ? NULL : from->t;
			type *chosen_ty = from->t;

			/* just go with leaq for small sizes */
			if(suff_ty && type_size(suff_ty, NULL) < 4)
				suff_ty = chosen_ty = NULL;

			out_asm(octx, "%s%s %s, %%%s",
					fp ? "mov" : "lea",
					x86_suffix(suff_ty),
					vstack_str(from, 1),
					x86_reg_str(reg, chosen_ty));

			if(from->type == V_LBL)
				new_ty = type_pointed_to(from->t);
			break;
		}

		case V_CONST_F:
			ICE("trying to load fp constant - should've been labelled");
	}

	return v_new_reg(octx, from, new_ty, reg);
}

void impl_store(out_ctx *octx, out_val *to, out_val *from)
{
	char vbuf[VSTACK_STR_SZ];

	/* from must be either a reg, value or flag */
	if(from->type == V_FLAG
	&& to->type == V_REG)
	{
		/* setting a register from a flag - easy */
		impl_load(octx, from, &to->bits.regoff.reg);
		out_val_consume(octx, to);
		return;
	}

	from = v_to(octx, from, TO_REG | TO_CONST);

	switch(to->type){
		case V_FLAG:
		case V_CONST_F:
			ICE("invalid store lvalue 0x%x", to->type);

		case V_REG_SPILT:
			/* need to load the store value from memory
			 * aka. double indir */
			to = v_to_reg(octx, to);
			break;

		case V_REG:
		case V_LBL:
		case V_CONST_I:
			break;
	}

	out_asm(octx, "mov%s %s, %s",
			x86_suffix(from->t),
			vstack_str_r(vbuf, from, 0),
			vstack_str(to, 1));

	out_val_consume(octx, from);
	out_val_consume(octx, to);
}

void impl_reg_cp(out_ctx *octx, out_val *from, const struct vreg *to_reg)
{
	char buf_v[VSTACK_STR_SZ];
	const char *regstr;

	UCC_ASSERT(from->type == V_REG,
			"reg_cp on non register type 0x%x", from->type);

	if(!from->bits.regoff.offset && vreg_eq(&from->bits.regoff.reg, to_reg))
		return;

	/* force offset normalisation */
	from = v_to(octx, from, TO_REG);

	regstr = x86_reg_str(to_reg, from->t);

	out_asm(octx, "mov%s %s, %%%s",
			x86_suffix(from->t),
			vstack_str_r(buf_v, from, 0),
			regstr);
}

out_val *impl_op(out_ctx *octx, enum op_type op, out_val *l, out_val *r)
{
	const char *opc;
	out_val *min_retained = l->retains < r->retains ? l : r;

	if(type_is_floating(l->t)){
		if(op_is_comparison(op)){
			/* ucomi%s reg_or_mem, reg */
			char b1[VSTACK_STR_SZ], b2[VSTACK_STR_SZ];

			l = v_to(octx, l, TO_REG | TO_MEM);
			r = v_to_reg(octx, r);

			out_asm(octx, "ucomi%s %s, %s",
					x86_suffix(l->t),
					vstack_str_r(b1, r, 0),
					vstack_str_r(b2, l, 0));

			if(l != min_retained) out_val_consume(octx, l);
			if(r != min_retained) out_val_consume(octx, r);

			/* not flag_mod_signed - we want seta, not setgt */
			return v_new_flag(
					octx, min_retained,
					op_to_flag(op), flag_mod_float);
		}

		switch(op){
#define OP(e, s) case op_ ## e: opc = s; break
			OP(multiply, "mul");
			OP(divide,   "div");
			OP(plus,     "add");
			OP(minus,    "sub");

			case op_not:
			case op_orsc:
			case op_andsc:
				ICE("unary/sc float op");

			default:
				ICE("bad fp op %s", op_to_str(op));
		}

		/* attempt to not do anything in the following v_to()
		 * by swapping operands, similarly to the integral case.
		 *
		 * [should merge at some point - generic instructions etc]
		 */

		if(l->type != V_REG && op_is_commutative(op)){
			out_val *tmp = l;
			l = r, r = tmp;
		}

		/* memory or register */
		l = v_to(octx, l, TO_REG);
		r = v_to(octx, r, TO_REG | TO_MEM);

		{
			char b1[VSTACK_STR_SZ], b2[VSTACK_STR_SZ];

			out_asm(octx, "%s%s %s, %s",
					opc, x86_suffix(l->t),
					vstack_str_r(b1, r, 0),
					vstack_str_r(b2, l, 0));

			out_val_consume(octx, l);
			return v_dup_or_reuse(octx, r, l->t);
		}
	}

	switch(op){
		OP(multiply, "imul");
		OP(plus,     "add");
		OP(minus,    "sub");
		OP(xor,      "xor");
		OP(or,       "or");
		OP(and,      "and");
#undef OP

		case op_bnot:
		case op_not:
			ICE("unary op in binary");

		case op_shiftl:
		case op_shiftr:
		{
			ICE("TODO: shift");
#if 0
			char bufv[VSTACK_STR_SZ], bufs[VSTACK_STR_SZ];
			struct vreg rtmp;

			/* value to shift must be a register */
			v_to_reg(&vtop[-1]);

			rtmp.is_float = 0, rtmp.idx = X86_64_REG_RCX;
			v_freeup_reg(&rtmp, 2); /* shift by rcx... x86 sigh */

			switch(vtop->type){
				default:
					v_to_reg(vtop); /* TODO: v_to_reg_preferred(vtop, X86_64_REG_RCX) */

				case V_REG:
					vtop->t = type_nav_btype(cc1_type_nav, type_nchar);

					rtmp.is_float = 0, rtmp.idx = X86_64_REG_RCX;
					if(!vreg_eq(&vtop->bits.regoff.reg, &rtmp)){
						impl_reg_cp(vtop, &rtmp);
						memcpy_safe(&vtop->bits.regoff.reg, &rtmp);
					}
					break;

				case V_CONST_F:
					ICE("float shift");
				case V_CONST_I:
					break;
			}

			vstack_str_r(bufs, vtop, 0);
			vstack_str_r(bufv, &vtop[-1], 0);

			out_asm(octx, "%s%s %s, %s",
					op == op_shiftl      ? "shl" :
					type_is_signed(vtop[-1].t) ? "sar" : "shr",
					x86_suffix(vtop[-1].t),
					bufs, bufv);

			vpop();
			return;
#endif
		}

		case op_modulus:
		case op_divide:
		{
			ICE("TODO: div/mod");
#if 0
			/*
			 * divides the 64 bit integer EDX:EAX
			 * by the operand
			 * quotient  -> eax
			 * remainder -> edx
			 */
			struct vreg rtmp[2], rdiv;

			/*
			 * if we are using reg_[ad] elsewhere
			 * and they aren't queued for this idiv
			 * then save them, so we can use them
			 * for idiv
			 */

			/*
			 * Must freeup the lower
			 */
			memset(rtmp, 0, sizeof rtmp);
			rtmp[0].idx = X86_64_REG_RAX;
			rtmp[1].idx = X86_64_REG_RDX;
			v_freeup_regs(&rtmp[0], &rtmp[1]);

			v_reserve_reg(&rtmp[1]); /* prevent rdx being used in the division */

			v_to_reg_out(&vtop[-1], &rdiv); /* TODO: similar to above - v_to_reg_preferred */

			if(rdiv.idx != X86_64_REG_RAX){
				/* we already have rax in use by vtop, swap the values */
				if(vtop->type == V_REG
				&& vtop->bits.regoff.reg.idx == X86_64_REG_RAX)
				{
					impl_reg_swp(vtop, &vtop[-1]);
				}else{
					v_freeup_reg(&rtmp[0], 2);
					impl_reg_cp(&vtop[-1], &rtmp[0]);
					vtop[-1].bits.regoff.reg.idx = X86_64_REG_RAX;
				}

				rdiv.idx = vtop[-1].bits.regoff.reg.idx;
			}

			UCC_ASSERT(rdiv.idx == X86_64_REG_RAX,
					"register A not chosen for idiv (%s)", x86_intreg_str(rdiv.idx, NULL));

			/* idiv takes either a reg or memory address */
			switch(vtop->type){
				default:
					v_to_reg(vtop);
					/* fall */

				case V_REG:
					if(vtop->bits.regoff.reg.idx == X86_64_REG_RDX){
						/* prevent rdx in division operand */
						struct vreg r;
						v_unused_reg(1, 0, &r);
						impl_reg_cp(vtop, &r);
						memcpy_safe(&vtop->bits.regoff.reg, &r);
					}

					out_asm(octx, "cqto");
					out_asm(octx, "idiv%s %s",
							x86_suffix(vtop->t),
							vstack_str(vtop, 0));
			}

			v_unreserve_reg(&rtmp[1]); /* free rdx */

			vpop();

			/* this is fine - we always use int-sized arithmetic or higher
			 * (in the char case, we would need ah:al
			 */

			v_clear(vtop, vtop->t);
			v_set_reg_i(vtop, op == op_modulus ? X86_64_REG_RDX : X86_64_REG_RAX);
			return;
#endif
		}

		case op_eq:
		case op_ne:
		case op_le:
		case op_lt:
		case op_ge:
		case op_gt:
			UCC_ASSERT(!type_is_floating(l->t),
					"float cmp should be handled above");
		{
			const int is_signed = type_is_signed(l->t);
			char buf[VSTACK_STR_SZ];
			int inv = 0;
			out_val *vconst = NULL;
			enum flag_cmp cmp;

			l = v_to(octx, l, TO_REG | TO_CONST);
			r = v_to(octx, r, TO_REG | TO_CONST);

			if(l->type == V_CONST_I)
				vconst = l;
			else if(r->type == V_CONST_I)
				vconst = r;

			/* if we have a CONST try a test instruction */
			if((op == op_eq || op == op_ne)
			&& vconst && vconst->bits.val_i == 0)
			{
				out_val *vother = vconst == l ? r : l;
				const char *vstr = vstack_str(vother, 0); /* reg */
				out_asm(octx, "test%s %s, %s", x86_suffix(vother->t), vstr, vstr);
			}else{
				/* if we have a const, it must be the first arg */
				if(r->type == V_CONST_I){
					out_val *tmp = l;
					l = r, r = tmp;
					inv = 1;
				}

				/* still a const? */
				if(r->type == V_CONST_I)
					r = v_to_reg(octx, r);

				out_asm(octx, "cmp%s %s, %s",
						x86_suffix(l->t), /* pick the non-const one (for type-ing) */
						vstack_str(r, 0),
						vstack_str_r(buf, l, 0));
			}

			cmp = op_to_flag(op);

			if(inv){
				/* invert >, >=, < and <=, but not == and !=, aka
				 * the commutative operators.
				 *
				 * i.e. 5 == 2 is the same as 2 == 5, but
				 *      5 >= 2 is not the same as 2 >= 5
				 */
				cmp = v_inv_cmp(cmp, /*invert_eq:*/0);
			}

			if(l != min_retained) out_val_consume(octx, l);
			if(r != min_retained) out_val_consume(octx, r);

			return v_new_flag(
					octx, min_retained,
					cmp, is_signed ? flag_mod_signed : 0);
		}

		case op_orsc:
		case op_andsc:
			ICE("%s shouldn't get here", op_to_str(op));

		default:
			ICE("invalid op %s", op_to_str(op));
	}

	{
		char buf[VSTACK_STR_SZ];

		/* RHS    LHS
		 * r/m += r/imm;
		 * r   += m/imm;
		 *
		 * echo {r,m}/{r,imm}
		 * echo r/{m,imm}
		 */
		static const struct
		{
			enum out_val_store l, r;
		} ops[] = {
			/* try in order of most 'difficult' -> least */
			{ V_REG, V_CONST_I },
			{ V_LBL, V_CONST_I },
			{ V_REG_SPILT, V_CONST_I },

			{ V_REG, V_LBL },
			{ V_LBL, V_REG },

			{ V_REG_SPILT, V_REG },
			{ V_REG, V_REG_SPILT },

			{ V_REG, V_REG },
		};
		static const int ops_n = sizeof(ops) / sizeof(ops[0]);
		int i, need_swap = 0, satisfied = 0;

		for(i = 0; i < ops_n; i++){
			if(l->type == ops[i].l
			&& r->type == ops[i].r)
			{
				satisfied = 1;
				break;
			}

			if(op_is_commutative(op)
			&& r->type == ops[i].l
			&& l->type == ops[i].r)
			{
				need_swap = satisfied = 1;
				break;
			}
		}

		if(need_swap){
			SWAP(out_val *, l, r);
		}else if(!satisfied){
			/* try to keep rhs as const */
			l = v_to(octx, l, TO_REG | TO_MEM);
			r = v_to(octx, r, TO_REG | TO_MEM | TO_CONST);

			if(V_IS_MEM(l->type) && V_IS_MEM(r->type))
				r = v_to_reg(octx, r);
		}

		if(v_is_const_reg(l) || v_is_const_reg(r))
			ICE("adjusting base pointer in op");

		switch(op){
			case op_plus:
			case op_minus:
				/* use inc/dec if possible */
				if(r->type == V_CONST_I
				&& r->bits.val_i == 1
				&& l->type == V_REG)
				{
					out_asm(octx, "%s%s %s",
							op == op_plus ? "inc" : "dec",
							x86_suffix(r->t),
							vstack_str(l, 0));
					break;
				}
			default:
				/* NOTE: lhs and rhs are switched for AT&T syntax,
				 * we still use lhs for the v_dup_or_reuse() below */
				out_asm(octx, "%s%s %s, %s", opc,
						x86_suffix(l->t),
						vstack_str_r(buf, r, 0),
						vstack_str(l, 0));
		}

		out_val_consume(octx, r);
		return v_dup_or_reuse(octx, l, l->t);
	}
}

out_val *impl_deref(out_ctx *octx, out_val *vp, const struct vreg *reg)
{
	char ptr[VSTACK_STR_SZ];
	type *tpointed_to = type_pointed_to(vp->t);
	struct vreg backup_reg;

	if(!reg){
		v_unused_reg(
				octx,
				1,
				type_is_floating(tpointed_to),
				&backup_reg);

		reg = &backup_reg;
	}

	/* loaded the pointer, now we apply the deref change */
	out_asm(octx, "mov%s %s, %%%s",
			x86_suffix(tpointed_to),
			vstack_str_r(ptr, vp, 1),
			x86_reg_str(reg, tpointed_to));

	return v_new_reg(octx, vp, tpointed_to, reg);
}

out_val *impl_op_unary(out_ctx *octx, enum op_type op, out_val *val)
{
	const char *opc;

	val = v_to(octx, val, TO_REG | TO_CONST | TO_MEM);

	switch(op){
		default:
			ICE("invalid unary op %s", op_to_str(op));

		case op_plus:
			/* noop */
			return val;

		case op_minus:
			if(type_is_floating(val->t)){
				return out_op(
						octx, op_minus,
						out_new_zero(octx, val->t),
						val);
			}
			opc = "neg";
			break;

		case op_bnot:
			UCC_ASSERT(!type_is_floating(val->t), "~ on float");
			opc = "not";
			break;

		case op_not:
			return out_op(
					octx, op_eq,
					val, out_new_zero(octx, val->t));
	}

	out_asm(octx, "%s%s %s", opc,
			x86_suffix(val->t),
			vstack_str(val, 0));

	return v_dup_or_reuse(octx, val, val->t);
}

out_val *impl_cast_load(
		out_ctx *octx, out_val *vp,
		type *small, type *big,
		int is_signed)
{
	/* we are always up-casting here, i.e. int -> long */
	char buf_small[VSTACK_STR_SZ];

	UCC_ASSERT(!type_is_floating(small) && !type_is_floating(big),
			"we don't cast-load floats");

	switch(vp->type){
		case V_CONST_F:
			ICE("cast load float");

		case V_CONST_I:
		case V_LBL:
		case V_REG_SPILT: /* could do something like movslq -8(%rbp), %rax */
		case V_FLAG:
			vp = v_to_reg(octx, vp);
		case V_REG:
			snprintf(buf_small, sizeof buf_small,
					"%%%s",
					x86_reg_str(&vp->bits.regoff.reg, small));
	}

	{
		const char *suffix_big = x86_suffix(big),
		           *suffix_small = x86_suffix(small);
		struct vreg r;

		/* mov[zs][bwl][wlq]
		 * avoid movzx - it's ambiguous
		 *
		 * special case: movzlq is invalid, we use movl %r, %r instead
		 */

		v_unused_reg(octx, 1, 0, &r);
		vp = v_new_reg(octx, vp, vp->t, &r);

		if(!is_signed && *suffix_big == 'q' && *suffix_small == 'l'){
			out_comment("movzlq:");
			out_asm(octx, "movl %s, %%%s",
					buf_small,
					x86_reg_str(&r, small));

		}else{
			out_asm(octx, "mov%c%s%s %s, %%%s",
					"zs"[is_signed],
					suffix_small,
					suffix_big,
					buf_small,
					x86_reg_str(&r, big));
		}

		return vp;
	}
}

static void x86_fp_conv(
		out_ctx *octx,
		out_val *vp,
		struct vreg *r, type *tto,
		type *int_ty,
		const char *sfrom, const char *sto)
{
	char vbuf[VSTACK_STR_SZ];

	out_asm(octx, "cvt%s2%s%s %s, %%%s",
			/*truncate ? "t" : "",*/
			sfrom, sto,
			/* if we're doing an int-float conversion,
			 * see if we need to do 64 or 32 bit
			 */
			int_ty ? type_size(int_ty, NULL) == 8 ? "q" : "l" : "",
			vstack_str_r(vbuf, vp, vp->type == V_REG_SPILT),
			x86_reg_str(r, tto));
}

static out_val *x86_xchg_fi(
		out_ctx *octx, out_val *vp,
		type *tfrom, type *tto)
{
	struct vreg r;
	int to_float;
	const char *fp_s;
	type *ty_fp, *ty_int;

	if((to_float = type_is_floating(tto)))
		ty_fp = tto, ty_int = tfrom;
	else
		ty_fp = tfrom, ty_int = tto;

	fp_s = x86_suffix(ty_fp);

	v_unused_reg(octx, 1, to_float, &r);

	/* cvt*2* [mem|reg], xmm* */
	vp = v_to(octx, vp, TO_REG | TO_MEM);

	/* need to promote vp to int for cvtsi2ss */
	if(type_size(ty_int, NULL) < type_primitive_size(type_int)){
		/* need to pretend we're using an int */
		type *const ty = *(to_float ? &tfrom : &tto) = type_nav_btype(cc1_type_nav, type_int);

		if(to_float){
			/* cast up to int, then to float */
			vp = out_cast(octx, vp, ty, 0);
		}else{
			char buf[TYPE_STATIC_BUFSIZ];
			out_comment("%s to %s - truncated",
					type_to_str(tfrom),
					type_to_str_r(buf, tto));
		}
	}

	x86_fp_conv(octx, vp, &r, tto,
			to_float ? tfrom : tto,
			to_float ? "si" : fp_s,
			to_float ? fp_s : "si");

	return v_new_reg(octx, vp, tto, &r);
}

out_val *impl_i2f(out_ctx *octx, out_val *vp, type *t_i, type *t_f)
{
	return x86_xchg_fi(octx, vp, t_i, t_f);
}

out_val *impl_f2i(out_ctx *octx, out_val *vp, type *t_f, type *t_i)
{
	return x86_xchg_fi(octx, vp, t_f, t_i);
}

out_val *impl_f2f(out_ctx *octx, out_val *vp, type *from, type *to)
{
	struct vreg r;

	v_unused_reg(octx, 1, 1, &r);

	x86_fp_conv(octx, vp, &r, to, NULL,
			x86_suffix(from),
			x86_suffix(to));

	return v_new_reg(octx, vp, to, &r);
}

static const char *x86_call_jmp_target(
		out_ctx *octx, out_val **pvp,
		int prevent_rax)
{
	static char buf[VSTACK_STR_SZ + 2];

	switch((*pvp)->type){
		case V_LBL:
			if((*pvp)->bits.lbl.offset){
				snprintf(buf, sizeof buf, "%s + %ld",
						(*pvp)->bits.lbl.str, (*pvp)->bits.lbl.offset);
				return buf;
			}
			return (*pvp)->bits.lbl.str;

		case V_CONST_F:
		case V_FLAG:
			ICE("jmp flag/float?");

		case V_CONST_I:   /* jmp *5 */
			snprintf(buf, sizeof buf, "*%s", vstack_str((*pvp), 1));
			return buf;

		case V_REG_SPILT: /* load, then jmp */
		case V_REG: /* jmp *%rax */
			/* TODO: v_to_reg_given() ? */
			*pvp = v_to_reg(octx, *pvp);

			UCC_ASSERT(!(*pvp)->bits.regoff.reg.is_float, "jmp float?");

			if(prevent_rax && (*pvp)->bits.regoff.reg.idx == X86_64_REG_RAX){
				struct vreg r;

				v_unused_reg(octx, 1, 0, &r);
				impl_reg_cp(octx, *pvp, &r);
				memcpy_safe(&(*pvp)->bits.regoff.reg, &r);
			}

			snprintf(buf, sizeof buf, "*%%%s",
					x86_reg_str(&(*pvp)->bits.regoff.reg, NULL));
			return buf;
	}

	ICE("invalid jmp target type 0x%x", (*pvp)->type);
	return NULL;
}

void impl_jmp(FILE *f, const char *lbl)
{
	fprintf(f, "\tjmp %s\n", lbl);
}

void impl_branch(out_ctx *octx, out_val *cond, out_blk *bt, out_blk *bf)
{
	switch(cond->type){
		case V_REG:
		{
			const char *rstr = vstack_str(cond, 0);
			char *cmp;

			out_asm(octx, "test %s, %s", rstr, rstr);
			cmp = ustrprintf("jz %s", bf->lbl);

			blk_terminate_condjmp(octx, cmp, bf, bt);
			break;
		}

		case V_FLAG:
		{
			char *cmpjmp = ustrprintf(
					"\tj%s %s", x86_cmp(&cond->bits.flag),
					bt->lbl);

			blk_terminate_condjmp(octx, cmpjmp, bt, bf);
			break;
#if 0
			int parity_chk, parity_rev = 0;

			parity_chk = x86_need_fp_parity_p(&vtop->bits.flag, &parity_rev);

			if(parity_chk){
				/* nan means false, unless parity_rev */
				/* this is slightly hacky - need basic block
				 * support to do this properly - impl_jcond
				 * should give two labels
				 */
				if(!parity_rev){
					/* skip */
					bb_lbl = out_label_code("jmp_parity");
					out_asm(octx, "jp %s", lbl);
				}
			}

			out_asm(octx, "j%s %s", x86_cmp(&vtop->bits.flag), lbl);

			if(parity_chk && parity_rev){
				/* jump not taken, try parity */
				out_asm(octx, "jp %s", lbl);
			}
			if(bb_lbl){
				impl_lbl(bb_lbl);
				free(bb_lbl);
			}
			break;
#endif
		}

		default:
			ICE("TODO branch %s", v_store_to_str(cond->type));
#if 0
		case V_CONST_F:
			ICE("jcond float");
		case V_CONST_I:
			if(true == !!vtop->bits.val_i)
				out_asm(octx, "jmp %s", lbl);

			out_comment(
					"constant jmp condition %" NUMERIC_FMT_D " %staken",
					vtop->bits.val_i, vtop->bits.val_i ? "" : "not ");

			break;

		case V_LBL:
		case V_REG_SAVE:
			v_to_reg(vtop);

		case V_REG:
			out_normalise();
			UCC_ASSERT(vtop->type != V_REG,
					"normalise remained as a register");
			impl_jcond(true, lbl);
			break;
#endif
	}
}

out_val *impl_call(
		out_ctx *octx,
		out_val *fn, out_val **args,
		type *fnty)
{
	const unsigned pws = platform_word_size();
	const unsigned nargs = dynarray_count(args);
	char *const float_arg = umalloc(nargs);
	out_val **local_args = NULL;

	const struct vreg *call_iregs;
	unsigned n_call_iregs;

	unsigned nfloats = 0, nints = 0;
	unsigned arg_stack = 0, align_stack = 0;
	unsigned stk_snapshot = 0;
	unsigned i;

	dynarray_add_array(&local_args, args);

	x86_call_regs(fnty, &n_call_iregs, &call_iregs);

	/* pre-scan of arguments - eliminate flags
	 * (should only be one, since we can only have one flag at a time)
	 *
	 * also count floats and ints
	 */
	for(i = 0; i < nargs; i++){
		if(local_args[i]->type == V_FLAG)
			local_args[i] = v_to_reg(octx, local_args[i]);

		if((float_arg[i] = type_is_floating(local_args[i]->t)))
			nfloats++;
		else
			nints++;
	}

	/* do we need to do any stacking? */
	if(nints > n_call_iregs)
		arg_stack += nints - n_call_iregs;


	if(nfloats > N_CALL_REGS_F)
		arg_stack += nfloats - N_CALL_REGS_F;

	/* need to save regs before pushes/call */
	v_save_regs(octx, fnty, local_args, fn);

	if(arg_stack > 0){
		out_comment("stack space for %d arguments", arg_stack);
		/* this aligns the stack-ptr and returns arg_stack padded */
		arg_stack = v_alloc_stack(
				octx, arg_stack * pws,
				"call argument space");
	}

	/* align the stack to 16-byte, for sse insns */
	align_stack = v_stack_align(octx, 16, 0);

	if(arg_stack > 0){
		unsigned nfloats = 0, nints = 0; /* shadow */

		unsigned stack_pos;
		/* must be called after v_alloc_stack() */
		stk_snapshot = stack_pos = octx->stack_sz;
		out_comment("-- stack snapshot (%u) --", stk_snapshot);

		/* save in order */
		for(i = 0; i < nargs; i++){
			const int stack_this = float_arg[i]
				? nfloats++ >= N_CALL_REGS_F
				: nints++ >= n_call_iregs;

			if(stack_this){
				local_args[i] = v_to_stack_mem(octx, local_args[i], -stack_pos);

				/* XXX: we ensure any registers used ^ are freed
				 * by using the stack snapshot - the STACK_SAVE
				 * space they take up isn't used after the call
				 * and so we can mercilessly wipe it out just
				 * before the call instruction.
				 */

				/* nth argument is higher in memory */
				stack_pos -= pws;
			}
		}
	}

	nints = nfloats = 0;
	for(i = 0; i < nargs; i++){
		out_val *const vp = local_args[i];
		const int is_float = type_is_floating(vp->t);

		const struct vreg *rp = NULL;
		struct vreg r;

		if(is_float){
			if(nfloats < N_CALL_REGS_F){
				/* NOTE: don't need to use call_regs_float,
				 * since it's xmm0 ... 7 */
				r.idx = nfloats;
				r.is_float = 1;

				rp = &r;
			}
			nfloats++;

		}else{
			/* integral */
			if(nints < n_call_iregs)
				rp = &call_iregs[nints];

			nints++;
		}

		if(rp){
			/* only bother if it's not already in the register */
			if(vp->type != V_REG || !vreg_eq(rp, &vp->bits.regoff.reg)){
				/* need to free it up, as v_to_reg_given doesn't clobber check */
				v_freeup_reg(octx, rp);
				local_args[i] = v_to_reg_given(octx, local_args[i], rp);
			}
		}
		/* else already pushed */
	}

	if(stk_snapshot){
		/* May have touched the stack in shifting around
		 * registers above - need to clean up the stack here
		 * for our call.
		 *
		 * Should just be able to add to the stack pointer,
		 * since we save all non-call registers before we
		 * start anything.
		 */
		unsigned chg = octx->stack_sz - stk_snapshot;
		out_comment("-- restore snapshot (%u) --", chg);
		v_dealloc_stack(octx, chg);
	}

	{
		funcargs *args = type_funcargs(fnty);
		int need_float_count =
			args->variadic
			|| FUNCARGS_EMPTY_NOVOID(args);
		/* jtarget must be assigned before "movb $0, %al" */
		const char *jtarget = x86_call_jmp_target(octx, &fn, need_float_count);

		/* if x(...) or x() */
		if(need_float_count){
			/* movb $nfloats, %al */
			struct vreg r;

			r.idx = X86_64_REG_RAX;
			r.is_float = 0;

			/* only the register arguments - glibc's printf of x86_64 linux
			 * segfaults if this is 9 or greater */
			out_flush_volatile(
					octx,
					v_to_reg_given(
						octx,
						out_new_l(
							octx,
							type_nav_btype(cc1_type_nav, type_nchar),
							MIN(nfloats, N_CALL_REGS_F)),
						&r));
		}

		out_asm(octx, "callq %s", jtarget);
	}

	if(arg_stack && x86_caller_cleanup(fnty))
		v_dealloc_stack(octx, arg_stack);
	if(align_stack)
		v_dealloc_stack(octx, align_stack);

	for(i = 0; i < nargs; i++)
		out_val_consume(octx, local_args[i]);
	dynarray_free(out_val **, &local_args, NULL);

	free(float_arg);

	{
		type *retty = type_called(type_is_ptr(fnty), NULL);
		const int fp = type_is_floating(retty);
		struct vreg rr = VREG_INIT(fp ? REG_RET_F : REG_RET_I, fp);

		return v_new_reg(octx, fn, retty, &rr);
	}
}

void impl_undefined(out_ctx *octx)
{
	out_asm(octx, "ud2");
	octx->current_blk->type = BLK_TERMINAL;
	octx->current_blk = NULL;
}

#if 0
void impl_set_overflow(void)
{
	v_set_flag(vtop, flag_overflow, 0);
}
#endif

#if 0
void impl_set_nan(type *ty)
{
	UCC_ASSERT(type_is_floating(ty),
			"%s for non %s", __func__, type_to_str(ty));

	switch(type_size(ty, NULL)){
		case 4:
		{
			const union
			{
				unsigned l;
				float f;
			} u = { 0x7fc00000u };
			vtop->bits.val_f = u.f;
			break;
		}
		case 8:
		{
			const union
			{
				unsigned long l;
				double d;
			} u = { 0x7ff8000000000000u };
			vtop->bits.val_f = u.d;
			break;
		}
		default:
			ICE("TODO: long double nan");
	}

	vtop->type = V_CONST_F;
	/* vtop->t should be set */
	impl_load_fp(vtop);
}
#endif
