#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "../../util/util.h"
#include "../../util/alloc.h"
#include "../../util/dynarray.h"
#include "../../util/platform.h"
#include "../data_structs.h"
#include "vstack.h"
#include "asm.h"
#include "impl.h"
#include "../cc1.h"
#include "common.h"
#include "out.h"
#include "lbl.h"
#include "../funcargs.h"


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

	[conv_cdecl]    = { 1, 0 },
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

static const char *x86_intreg_str(unsigned reg, type_ref *r)
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

static const char *x86_suffix(type_ref *ty)
{
	if(type_ref_is_floating(ty)){
		switch(type_ref_primitive(ty)){
			case type_float:
				return "ss";
			case type_double:
				return "sd";
			case type_ldouble:
				ICE("TODO");
			default:
				ICE("bad float");
		}
	}

	switch(ty ? type_ref_size(ty, NULL) : 8){
		case 1: return "b";
		case 2: return "w";
		case 4: return "l";
		case 8: return "q";
	}
	ICE("no suffix for %s", type_ref_to_str(ty));
}

static const char *x86_reg_str(const struct vreg *reg, type_ref *r)
{
	/* must be sync'd with header */
	if(reg->is_float){
		return x86_fpreg_str(reg->idx);
	}else{
		return x86_intreg_str(reg->idx, r);
	}
}

static const char *reg_str(struct vstack *reg)
{
	return x86_reg_str(&reg->bits.regoff.reg, reg->t);
}

static const char *vstack_str_r(
		char buf[VSTACK_STR_SZ], struct vstack *vs, const int deref)
{
	switch(vs->type){
		case V_CONST_I:
		{
			char *p = buf;
			/* we should never get a 64-bit value here
			 * since movabsq should load those in
			 */
			UCC_ASSERT(!integral_is_64_bit(vs->bits.val_i, vs->t),
					"can't load 64-bit constants here (0x%llx)",
					vs->bits.val_i);

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
		case V_REG_SAVE:
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

static const char *vstack_str(struct vstack *vs, int deref)
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

static const struct calling_conv_desc *x86_conv_lookup(type_ref *fr)
{
	funcargs *fa = type_ref_funcargs(fr);

	return &calling_convs[fa->conv];
}

static int x86_caller_cleanup(type_ref *fr)
{
	const int cr_clean = x86_conv_lookup(fr)->caller_cleanup;

	if(!cr_clean && type_ref_is_variadic_func(fr))
		die_at(&fr->where, "variadic functions can't be callee cleanup");

	return cr_clean;
}

static void x86_call_regs(
		type_ref *fr, unsigned *pn,
		const struct vreg **par)
{
	const struct calling_conv_desc *ent = x86_conv_lookup(fr);
	*pn = ent->n_call_regs;
	if(par)
		*par = ent->call_regs;
}

static int x86_func_nargs(type_ref *rf)
{
	return dynarray_count(type_ref_funcargs(rf)->arglist);
}

int impl_reg_is_callee_save(const struct vreg *r, type_ref *fr)
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

unsigned impl_n_call_regs(type_ref *rf)
{
	unsigned n;
	x86_call_regs(rf, &n, NULL);
	return n;
}

void impl_func_prologue_save_fp(void)
{
	out_asm("pushq %%rbp");
	out_asm("movq %%rsp, %%rbp");
	/* a v_alloc_stack_n() is done later to align,
	 * but not interfere with argument locations */
}

static void reg_to_stack(
		const struct vreg *vr,
		type_ref *ty, long where)
{
	vpush(ty);
	v_set_reg(vtop, vr);
	v_to_mem_given(vtop, -where);
	vpop();
}

void impl_func_prologue_save_call_regs(
		type_ref *rf, unsigned nargs,
		int arg_offsets[/*nargs*/])
{
	if(nargs){
		const unsigned ws = platform_word_size();

		funcargs *const fa = type_ref_funcargs(rf);

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

			v_alloc_stack(
					(n_call_f + n_call_i) * platform_word_size(),
					"save call regs float+integral");

			for(i_arg = i_i = i_f = i_stk = i_arg_stk = 0;
					i_arg < nargs;
					i_arg++)
			{
				type_ref *const ty = fa->arglist[i_arg]->ref;
				const struct vreg *rp;
				struct vreg vr;

				if(type_ref_is_floating(ty)){
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

					reg_to_stack(rp, ty, off);

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
					out_asm("push%s %%%s",
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
			v_alloc_stack_n(
					n_call_i * platform_word_size(),
					"save call regs push-version");
		}
	}
}

void impl_func_prologue_save_variadic(type_ref *rf)
{
	const unsigned pws = platform_word_size();

	unsigned n_call_regs;
	const struct vreg *call_regs;

	type_ref *const ty_dbl = type_ref_cached_DOUBLE();
	type_ref *const ty_integral = type_ref_cached_INTPTR_T();

	unsigned n_int_args, n_fp_args;

	unsigned stk_top;
	unsigned i;

	x86_call_regs(rf, &n_call_regs, &call_regs);

	funcargs_ty_calc(type_ref_funcargs(rf), &n_int_args, &n_fp_args);

	/* space for all call regs */
	v_alloc_stack(
			(N_CALL_REGS_I + N_CALL_REGS_F * 2) * platform_word_size(),
			"stack call arguments");

	stk_top = v_stack_sz();

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
		reg_to_stack(&vr, ty_integral,
				stk_top - i * pws);
	}

	{
		char *vfin = out_label_code("va_skip_float");
		type_ref *const ty_ch = type_ref_cached_CHAR(n);

		/* testb %al, %al ; jz vfin */
		vpush(ty_ch);
		v_set_reg_i(vtop, X86_64_REG_RAX);
		out_push_zero(ty_ch);
		out_op(op_eq);
		out_jtrue(vfin);

		for(i = 0; i < N_CALL_REGS_F; i++){
			struct vreg vr;

			vr.is_float = 1;
			vr.idx = i;

			/* we go above the integral regs */
			reg_to_stack(&vr, ty_dbl,
					stk_top - (i * 2 + n_call_regs) * pws);
		}

		out_label(vfin);
		free(vfin);
	}
}

void impl_func_epilogue(type_ref *rf)
{
	out_asm("leaveq");

	if(fopt_mode & FOPT_VERBOSE_ASM)
		out_comment("stack at %u bytes", v_stack_sz());

	/* callee cleanup */
	if(!x86_caller_cleanup(rf)){
		const int nargs = x86_func_nargs(rf);

		out_asm("retq $%d", nargs * platform_word_size());
	}else{
		out_asm("retq");
	}
}

void impl_pop_func_ret(type_ref *ty)
{
	struct vreg r;

	/* FIXME: merge with mips */

	r.idx =
		(r.is_float = type_ref_is_floating(ty))
		? REG_RET_F
		: REG_RET_I;

	/* v_to_reg since we don't handle lea/load ourselves */
	v_to_reg_given(vtop, &r);
	vpop();
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

void impl_load_iv(struct vstack *vp)
{
	if(integral_is_64_bit(vp->bits.val_i, vp->t)){
		char buf[INTEGRAL_BUF_SIZ];
		struct vreg r;
		v_unused_reg(1, 0, &r);

		/* TODO: 64-bit registers in general on 32-bit */
		UCC_ASSERT(!IS_32_BIT(), "TODO: 32-bit 64-literal loads");

		UCC_ASSERT(type_ref_size(vp->t, NULL) == 8,
				"loading 64-bit literal (%lld) for non-long? (%s)",
				vp->bits.val_i, type_ref_to_str(vp->t));

		integral_str(buf, sizeof buf,
				vp->bits.val_i, vp->t);

		out_asm("movabsq $%s, %%%s",
				buf, x86_reg_str(&r, vp->t));

		v_set_reg(vp, &r);
	}
}

void impl_load_fp(struct vstack *from)
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
				type_ref *const ty_fp = from->t;

				from->type = V_CONST_I;
				from->bits.val_i = from->bits.val_f;
				/* TODO: use just an int if we can get away with it */
				from->t = type_ref_cached_LLONG();

				out_cast(ty_fp);
				break;
			}
			/* fall */

		default:
		{
			/* save to a label */
			char *lbl = out_label_data_store(0);
			struct vreg r;

			asm_label(SECTION_DATA, lbl, type_ref_align(from->t, NULL));
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

void impl_load(struct vstack *from, const struct vreg *reg)
{
	/* load - convert vstack to a register - if it's a pointer,
	 * the register is a pointer. for a dereference, call impl_deref()
	 */

	if(from->type == V_REG && vreg_eq(reg, &from->bits.regoff.reg))
		return;

	switch(from->type){
		case V_FLAG:
		{
			struct vstack vtmp_val = VSTACK_INIT(V_CONST_I);
			char *parity = NULL;
			int parity_default = 0;

			vtmp_val.t = from->t;

			/* check float/orderedness */
			if(x86_need_fp_parity_p(&from->bits.flag, &parity_default))
				parity = out_label_code("parity");

			vtmp_val.bits.val_i = parity_default;
			impl_load(&vtmp_val, reg);

			if(parity)
				out_asm("jp %s", parity);

			/* XXX: memleak */
			from->t = type_ref_cached_CHAR(n); /* force set%s to set the low byte */

			/* actual cmp */
			out_asm("set%s %%%s",
					x86_cmp(&from->bits.flag),
					x86_reg_str(reg, from->t));

			if(parity){
				/* don't use out_label - this does a vstack flush */
				impl_lbl(parity);
				free(parity);
			}
			break;
		}

		case V_REG_SAVE:
			/* v_reg_save loads are actually pointers to T */
			impl_deref(from, reg, from->t);
			break;

		case V_REG:
			if(from->bits.regoff.offset)
				goto lea;
			/* fall */

		case V_CONST_I:
			out_asm("mov%s %s, %%%s",
					x86_suffix(from->t),
					vstack_str(from, 0),
					x86_reg_str(reg, from->t));
			break;

lea:
		case V_LBL:
		{
			const int fp = type_ref_is_floating(from->t);
			out_asm("%s%s %s, %%%s",
					fp ? "mov" : "lea",
					x86_suffix(fp ? NULL : from->t),
					vstack_str(from, 1),
					x86_reg_str(reg, from->t));
			break;
		}

		case V_CONST_F:
			ICE("trying to load fp constant - should've been labelled");
	}
}

void impl_store(struct vstack *from, struct vstack *to)
{
	char vbuf[VSTACK_STR_SZ];

	/* from must be either a reg, value or flag */
	if(from->type == V_FLAG
	&& to->type == V_REG)
	{
		/* setting a register from a flag - easy */
		impl_load(from, &to->bits.regoff.reg);
		return;
	}

	v_to(from, TO_REG | TO_CONST);

	switch(to->type){
		case V_FLAG:
		case V_CONST_F:
			ICE("invalid store lvalue 0x%x", to->type);

		case V_REG_SAVE:
			if(to->is_lval){
				/* store to lval, fine */
			}else{
				/* need to load the store value from memory
				 * aka. double indir */
				v_to_reg(to);
			}
			break;

		case V_REG:
		case V_LBL:
		case V_CONST_I:
			break;
	}

	out_asm("mov%s %s, %s",
			x86_suffix(from->t),
			vstack_str_r(vbuf, from, 0),
			vstack_str(to, 1));
}

void impl_reg_swp(struct vstack *a, struct vstack *b)
{
	struct vreg tmp;

	UCC_ASSERT(
			a->type == b->type
			&& a->type == V_REG,
			"%s without regs (%d and %d)", __func__,
			a->type, b->type);

	out_asm("xchg %%%s, %%%s",
			reg_str(a), reg_str(b));

	tmp = a->bits.regoff.reg;
	a->bits.regoff.reg = b->bits.regoff.reg;
	b->bits.regoff.reg = tmp;
}

void impl_reg_cp(struct vstack *from, const struct vreg *r)
{
	char buf_v[VSTACK_STR_SZ];
	const char *regstr;

	UCC_ASSERT(from->type == V_REG,
			"reg_cp on non register type 0x%x", from->type);

	if(!from->bits.regoff.offset && vreg_eq(&from->bits.regoff.reg, r))
		return;

	v_to(from, TO_REG); /* force offset normalisation */

	regstr = x86_reg_str(r, from->t);

	out_asm("mov%s %s, %%%s",
			x86_suffix(from->t),
			vstack_str_r(buf_v, from, 0),
			regstr);
}

void impl_op(enum op_type op)
{
#define OP(e, s) case op_ ## e: opc = s; break
	const char *opc;

	if(type_ref_is_floating(vtop->t)){
		if(op_is_comparison(op)){
			/* ucomi%s reg_or_mem, reg */
			char b1[VSTACK_STR_SZ], b2[VSTACK_STR_SZ];

			v_to(vtop, TO_REG | TO_MEM);
			v_to_reg(&vtop[-1]);

			out_asm("ucomi%s %s, %s",
					x86_suffix(vtop->t),
					vstack_str_r(b1, vtop, 0),
					vstack_str_r(b2, &vtop[-1], 0));

			vpop();
			v_set_flag(vtop, op_to_flag(op), flag_mod_float);
			/* not flag_mod_signed - we want seta, not setgt */
			return;
		}

		switch(op){
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

		if(vtop->type != V_REG && op_is_commutative(op))
			out_swap();

		/* memory or register */
		v_to(vtop,      TO_REG);
		v_to(&vtop[-1], TO_REG | TO_MEM);

		{
			char b1[VSTACK_STR_SZ], b2[VSTACK_STR_SZ];

			out_asm("%s%s %s, %s",
					opc, x86_suffix(vtop->t),
					vstack_str_r(b1, &vtop[-1], 0),
					vstack_str_r(b2, vtop, 0));

			/* result in vtop */
			vswap();
			vpop();

			return;
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
			char bufv[VSTACK_STR_SZ], bufs[VSTACK_STR_SZ];
			type_ref *free_this = NULL;
			struct vreg rtmp;

			/* value to shift must be a register */
			v_to_reg(&vtop[-1]);

			rtmp.is_float = 0, rtmp.idx = X86_64_REG_RCX;
			v_freeup_reg(&rtmp, 2); /* shift by rcx... x86 sigh */

			switch(vtop->type){
				default:
					v_to_reg(vtop); /* TODO: v_to_reg_preferred(vtop, X86_64_REG_RCX) */

				case V_REG:
					free_this = vtop->t = type_ref_cached_CHAR(n);

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

			out_asm("%s%s %s, %s",
					op == op_shiftl      ? "shl" :
					type_ref_is_signed(vtop[-1].t) ? "sar" : "shr",
					x86_suffix(vtop[-1].t),
					bufs, bufv);

			vpop();

			type_ref_free_1(free_this);
			return;
		}

		case op_modulus:
		case op_divide:
		{
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

					out_asm("cqto");
					out_asm("idiv%s %s",
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
		}

		case op_eq:
		case op_ne:
		case op_le:
		case op_lt:
		case op_ge:
		case op_gt:
			UCC_ASSERT(!type_ref_is_floating(vtop->t), "TODO float cmp");
		{
			const int is_signed = type_ref_is_signed(vtop->t);
			char buf[VSTACK_STR_SZ];
			int inv = 0;

			v_to(vtop,     TO_REG | TO_CONST);
			v_to(vtop - 1, TO_REG | TO_CONST);

			/* if we have a const, it must be the first arg */
			if(vtop[-1].type == V_CONST_I){
				vswap();
				inv = 1;
			}

			/* if we have a CONST, it'll be in vtop,
			 * try a test instruction */
			if((op == op_eq || op == op_ne)
			&& vtop->type == V_CONST_I
			&& vtop->bits.val_i == 0)
			{
				const char *vstr = vstack_str(vtop - 1, 0); /* vtop[-1] is REG */
				out_asm("test%s %s, %s", x86_suffix(vtop[-1].t), vstr, vstr);
			}else{
				out_asm("cmp%s %s, %s",
						x86_suffix(vtop[-1].t), /* pick the non-const one (for type-ing) */
						vstack_str(       vtop, 0),
						vstack_str_r(buf, vtop - 1, 0));
			}

			vpop();

			v_set_flag(vtop, op_to_flag(op), is_signed ? flag_mod_signed : 0);
			if(inv)
				v_inv_cmp(&vtop->bits.flag);
			return;
		}

		case op_orsc:
		case op_andsc:
			ICE("%s shouldn't get here", op_to_str(op));

		default:
			ICE("invalid op %s", op_to_str(op));
	}

	{
		char buf[VSTACK_STR_SZ];

		v_to(vtop,     TO_REG | TO_CONST | TO_MEM);
		v_to(vtop - 1, TO_REG | TO_CONST | TO_MEM);

		/* vtop[-1] is a constant - needs to be in a reg */
		if(vtop[-1].type != V_REG){
			/* if the op is commutative, swap */
			if(op_is_commutative(op))
				out_swap();
			else
				v_to_reg(vtop - 1);
		}

		/* if neither are registers, v_to_reg one */
		if(vtop->type != V_REG
		&& vtop[-1].type != V_REG)
		{
			/* -1 is where the op is going (see end of this block) */
			v_to_reg(vtop - 1);
		}

		/* TODO: -O1
		 * if the op is commutative and we have REG_RET,
		 * make it the result reg
		 */

#define IS_RBP(vp) ((vp)->type == V_REG \
		&& (vp)->bits.regoff.reg.idx == X86_64_REG_RBP)
		if(IS_RBP(&vtop[-1]) || IS_RBP(vtop))
			ICE("adjusting base pointer in op");
#undef IS_RBP

		switch(op){
			case op_plus:
			case op_minus:
				/* use inc/dec if possible */
				if(vtop->type == V_CONST_I
				&& vtop->bits.val_i == 1
				&& vtop[-1].type == V_REG)
				{
					out_asm("%s%s %s",
							op == op_plus ? "inc" : "dec",
							x86_suffix(vtop[-1].t),
							vstack_str(&vtop[-1], 0));
					break;
				}
			default:
				out_asm("%s%s %s, %s", opc,
						x86_suffix(vtop->t),
						vstack_str_r(buf, &vtop[ 0], 0),
						vstack_str(       &vtop[-1], 0));
		}

		/* remove first operand - result is then in vtop (already in a reg) */
		vpop();
	}
}

void impl_deref(
		struct vstack *vp,
		const struct vreg *to,
		type_ref *tpointed_to)
{
	char ptr[VSTACK_STR_SZ];

	/* loaded the pointer, now we apply the deref change */
	out_asm("mov%s %s, %%%s",
			x86_suffix(tpointed_to),
			vstack_str_r(ptr, vp, 1),
			x86_reg_str(to, tpointed_to));
}

void impl_op_unary(enum op_type op)
{
	const char *opc;

	v_to(vtop, TO_REG | TO_CONST | TO_MEM);

	switch(op){
		default:
			ICE("invalid unary op %s", op_to_str(op));

		case op_plus:
			/* noop */
			return;

		case op_minus:
			if(type_ref_is_floating(vtop->t)){
				out_push_zero(vtop->t);
				out_op(op_minus);
				return;
			}
			opc = "neg";
			break;

		case op_bnot:
			UCC_ASSERT(!type_ref_is_floating(vtop->t), "~ on float");
			opc = "not";
			break;

		case op_not:
			out_push_zero(vtop->t);
			out_op(op_eq);
			return;
	}

	out_asm("%s%s %s", opc,
			x86_suffix(vtop->t),
			vstack_str(vtop, 0));
}

void impl_cast_load(struct vstack *vp, type_ref *small, type_ref *big, int is_signed)
{
	/* we are always up-casting here, i.e. int -> long */
	char buf_small[VSTACK_STR_SZ];

	UCC_ASSERT(!type_ref_is_floating(small) && !type_ref_is_floating(big),
			"we don't cast-load floats");

	switch(vp->type){
		case V_CONST_F:
			ICE("cast load float");
		case V_CONST_I:
		case V_LBL:
			/* something like movslq -8(%rbp), %rax */
			vstack_str_r(buf_small, vp, 1);
			break;

		case V_REG_SAVE:
		case V_FLAG:
			v_to_reg(vp);
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

		v_unused_reg(1, 0, &r);

		if(!is_signed && *suffix_big == 'q' && *suffix_small == 'l'){
			out_comment("movzlq:");
			out_asm("movl %s, %%%s",
					buf_small,
					x86_reg_str(&r, small));

		}else{
			out_asm("mov%c%s%s %s, %%%s",
					"zs"[is_signed],
					suffix_small,
					suffix_big,
					buf_small,
					x86_reg_str(&r, big));
		}

		v_set_reg(vp, &r);
	}
}

static void x86_fp_conv(
		struct vstack *vp,
		struct vreg *r, type_ref *tto,
		type_ref *int_ty,
		const char *sfrom, const char *sto)
{
	char vbuf[VSTACK_STR_SZ];

	out_asm("cvt%s2%s%s %s, %%%s",
			/*truncate ? "t" : "",*/
			sfrom, sto,
			/* if we're doing an int-float conversion,
			 * see if we need to do 64 or 32 bit
			 */
			int_ty ? type_ref_size(int_ty, NULL) == 8 ? "q" : "l" : "",
			vstack_str_r(vbuf, vp, vp->type == V_REG_SAVE),
			x86_reg_str(r, tto));
}

static void x86_xchg_fi(struct vstack *vp, type_ref *tfrom, type_ref *tto)
{
	struct vreg r;
	int to_float;
	const char *fp_s;
	type_ref *ty_fp, *ty_int;

	if((to_float = type_ref_is_floating(tto)))
		ty_fp = tto, ty_int = tfrom;
	else
		ty_fp = tfrom, ty_int = tto;

	fp_s = x86_suffix(ty_fp);

	v_unused_reg(1, to_float, &r);

	/* cvt*2* [mem|reg], xmm* */
	v_to(vp, TO_REG | TO_MEM);

	/* need to promote vp to int for cvtsi2ss */
	if(type_ref_size(ty_int, NULL) < type_primitive_size(type_int)){
		/* need to pretend we're using an int */
		type_ref *const ty = *(to_float ? &tfrom : &tto) = type_ref_cached_INT();

		if(to_float){
			/* cast up to int, then to float */
			v_cast(vp, ty);
		}else{
			char buf[TYPE_REF_STATIC_BUFSIZ];
			out_comment("%s to %s - truncated",
					type_ref_to_str(tfrom),
					type_ref_to_str_r(buf, tto));
		}
	}

	x86_fp_conv(vp, &r, tto,
			to_float ? tfrom : tto,
			to_float ? "si" : fp_s,
			to_float ? fp_s : "si");

	v_set_reg(vp, &r);
	/* type set later in v_cast */
}

void impl_i2f(struct vstack *vp, type_ref *t_i, type_ref *t_f)
{
	x86_xchg_fi(vp, t_i, t_f);
}

void impl_f2i(struct vstack *vp, type_ref *t_f, type_ref *t_i)
{
	x86_xchg_fi(vp, t_f, t_i);
}

void impl_f2f(struct vstack *vp, type_ref *from, type_ref *to)
{
	struct vreg r;

	v_unused_reg(1, 1, &r);
	x86_fp_conv(vp, &r, to, NULL,
			x86_suffix(from),
			x86_suffix(to));

	v_set_reg(vp, &r);
}

static const char *x86_call_jmp_target(struct vstack *vp, int prevent_rax)
{
	static char buf[VSTACK_STR_SZ + 2];

	switch(vp->type){
		case V_LBL:
			if(vp->bits.lbl.offset){
				snprintf(buf, sizeof buf, "%s + %ld",
						vtop->bits.lbl.str, vtop->bits.lbl.offset);
				return buf;
			}
			return vp->bits.lbl.str;

		case V_CONST_F:
		case V_FLAG:
			ICE("jmp flag/float?");

		case V_CONST_I:   /* jmp *5 */
			snprintf(buf, sizeof buf, "*%s", vstack_str(vp, 1));
			break;

		case V_REG_SAVE: /* load, then jmp */
		case V_REG: /* jmp *%rax */
			/* TODO: v_to_reg_given() ? */
			v_to_reg(vp);

			UCC_ASSERT(!vp->bits.regoff.reg.is_float, "jmp float?");

			if(prevent_rax && vp->bits.regoff.reg.idx == X86_64_REG_RAX){
				struct vreg r;
				v_unused_reg(1, 0, &r);
				impl_reg_cp(vp, &r);
				memcpy_safe(&vp->bits.regoff.reg, &r);
			}

			snprintf(buf, sizeof buf, "*%%%s", reg_str(vp));
			return buf;
	}

	ICE("invalid jmp target");
	return NULL;
}

void impl_jmp(void)
{
	out_asm("jmp %s", x86_call_jmp_target(vtop, 0));
}

void impl_jcond(int true, const char *lbl)
{
	switch(vtop->type){
		case V_FLAG:
		{
			const int inv = !true;
			int parity_chk, parity_rev = 0;
			char *bb_lbl = NULL;

			if(inv)
				v_inv_cmp(&vtop->bits.flag);

			parity_chk = x86_need_fp_parity_p(&vtop->bits.flag, &parity_rev);

			parity_rev ^= inv;

			if(parity_chk){
				/* nan means false, unless parity_rev */
				/* this is slightly hacky - need basic block
				 * support to do this properly - impl_jcond
				 * should give two labels
				 */
				if(!parity_rev){
					/* skip */
					bb_lbl = out_label_code("jmp_parity");
					out_asm("jp %s", lbl);
				}
			}

			out_asm("j%s %s", x86_cmp(&vtop->bits.flag), lbl);

			if(parity_chk && parity_rev){
				/* jump not taken, try parity */
				out_asm("jp %s", lbl);
			}
			if(bb_lbl){
				impl_lbl(bb_lbl);
				free(bb_lbl);
			}
			break;
		}

		case V_CONST_F:
			ICE("jcond float");
		case V_CONST_I:
			if(true == !!vtop->bits.val_i)
				out_asm("jmp %s", lbl);

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
	}
}

void impl_call(const int nargs, type_ref *r_ret, type_ref *r_func)
{
	const unsigned pws = platform_word_size();
	char *const float_arg = umalloc(nargs);

	const struct vreg *call_iregs;
	unsigned n_call_iregs;

	unsigned nfloats = 0, nints = 0;
	unsigned arg_stack = 0;
	unsigned stk_snapshot = 0;
	int i;

	x86_call_regs(r_func, &n_call_iregs, &call_iregs);

	(void)r_ret;

	/* pre-scan of arguments - eliminate flags
	 * (should only be one, since we can only have one flag at a time)
	 *
	 * also count floats and ints
	 */
	for(i = 0; i < nargs; i++){
		struct vstack *const vp = &vtop[-i];

		if(vp->type == V_FLAG)
			v_to_reg(&vtop[-i]);

		if((float_arg[i] = type_ref_is_floating(vp->t)))
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
	v_save_regs(nargs, r_func);

	/* align the stack to 16-byte, for sse insns */
	v_stack_align(16, 0);

	if(arg_stack > 0){
		unsigned nfloats = 0, nints = 0; /* shadow */
		unsigned stack_pos;

		out_comment("stack space for %d arguments", arg_stack);
		/* this aligns the stack-ptr and returns arg_stack padded */
		arg_stack = v_alloc_stack(arg_stack * pws,
				"call argument space");

		/* must be called after v_alloc_stack() */
		stk_snapshot = stack_pos = v_stack_sz();
		out_comment("-- stack snapshot (%u) --", stk_snapshot);

		/* save in order */
		for(i = 0; i < nargs; i++){
			const int stack_this = float_arg[i]
				? nfloats++ >= N_CALL_REGS_F
				: nints++ >= n_call_iregs;

			if(stack_this){
				struct vstack *const vp = &vtop[-i];

				static int w;
				if(!w){
					w = 1;
					ICW("check correct stack layout/calculation for stack args");
				}

				/* v_to_mem* does v_to_reg first if needed */
				v_to_mem_given(vp, -stack_pos);

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
		struct vstack *const vp = &vtop[-i];
		const int is_float = type_ref_is_floating(vp->t);

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
				v_freeup_reg(rp, 0);
				v_to_reg_given(vp, rp);
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
		unsigned chg = v_stack_sz() - stk_snapshot;
		out_comment("-- restore snapshot (%u) --", chg);
		v_dealloc_stack(chg);
	}

	for(i = 0; i < nargs; i++)
		vpop();

	{
		funcargs *args = type_ref_funcargs(r_func);
		int need_float_count = args->variadic || (!args->arglist && !args->args_void);
		/* jtarget must be assigned before "movb $0, %al" */
		const char *jtarget = x86_call_jmp_target(vtop, need_float_count);

		/* if x(...) or x() */
		if(need_float_count){
			/* movb $nfloats, %al */
			struct vreg r;

			r.idx = X86_64_REG_RAX;
			r.is_float = 0;

			out_push_l(type_ref_cached_CHAR(n), nfloats);
			v_to_reg_given(vtop, &r);
			vpop();
		}

		out_asm("callq %s", jtarget);
	}

	if(arg_stack && x86_caller_cleanup(r_func))
		v_dealloc_stack(arg_stack);

	free(float_arg);
}

void impl_undefined(void)
{
	out_asm("ud2");
}

void impl_set_overflow(void)
{
	v_set_flag(vtop, flag_overflow, 0);
}

int impl_frame_ptr_to_reg(int nframes)
{
	/* XXX: memleak */
	type_ref *const void_pp = type_ref_ptr_depth_inc(
			type_ref_cached_VOID_PTR());

	struct vreg r;

	vpush(void_pp);
	v_set_reg_i(vtop, REG_BP);

	v_unused_reg(1, 0, &r);

	impl_reg_cp(vtop, &r); /* movq %rbp, <reg> */
	while(--nframes > 0){
		v_set_reg(vtop, &r);
		impl_deref(vtop, &r, void_pp); /* movq (<reg>), <reg> */
	}

	vpop();

	return r.idx;
}

void impl_set_nan(type_ref *ty)
{
	const union
	{
		unsigned l;
		float f;
	} u = { 0x7fc00000U };

	ICW("using nan of float type for %s", type_ref_to_str(ty));
	(void)ty;

	vtop->type = V_CONST_F;
	vtop->bits.val_f = u.f;
	/* vtop->t should be set */
	impl_load_fp(vtop);
}
