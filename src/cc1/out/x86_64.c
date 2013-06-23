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
#include "impl.h"
#include "../cc1.h"
#include "asm.h"
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

	int n_call_regs;
	struct vreg call_regs[6 + 8];

	int n_callee_save_regs;
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
		/* TODO: float, double etc */
		ty = type_ref_is_type(ty, type_unknown);
		switch(ty->bits.type->primitive){
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
	UCC_ASSERT(reg->type == REG, "non-reg %d", reg->type);
	return x86_reg_str(&reg->bits.reg, reg->t);
}

static const char *vstack_str_r_ptr(char buf[VSTACK_STR_SZ], struct vstack *vs, int ptr)
{
	switch(vs->type){
		case CONST_I:
		{
			char *p = buf;
			/* we should never get a 64-bit value here
			 * since movabsq should load those in
			 */
			UCC_ASSERT(!integral_is_64_bit(vs->bits.val_i, vs->t),
					"can't load 64-bit constants here (0x%llx)",
					vs->bits.val_i);

			if(!ptr)
				*p++ = '$';

			integral_str(p, VSTACK_STR_SZ - (!ptr ? 1 : 0),
					vs->bits.val_i, vs->t);
			break;
		}

		case CONST_F:
			ICE("TODO: str float");

		case FLAG:
			ICE("%s shouldn't be called with cmp-flag data", __func__);

		case LBL:
		{
			const int pic = fopt_mode & FOPT_PIC && vs->bits.lbl.pic;

			SNPRINTF(buf, VSTACK_STR_SZ, "%s%s%s",
					ptr ? "$" : "",
					vs->bits.lbl.str,
					pic ? "(%rip)" : "");
			break;
		}

		case REG:
			snprintf(buf, VSTACK_STR_SZ, "%s%%%s%s",
					ptr ? "(" : "", reg_str(vs), ptr ? ")" : "");
			break;

		case STACK:
		case STACK_SAVE:
		{
			int n = vs->bits.off_from_bp;
			SNPRINTF(buf, VSTACK_STR_SZ, "%s" NUM_FMT "(%%rbp)", n < 0 ? "-" : "", abs(n));
			break;
		}
	}

	return buf;
}

static const char *vstack_str_r(char buf[VSTACK_STR_SZ], struct vstack *vs)
{
	return vstack_str_r_ptr(buf, vs, 0);
}

static const char *vstack_str(struct vstack *vs)
{
	static char buf[VSTACK_STR_SZ];
	return vstack_str_r(buf, vs);
}

static const char *vstack_str_ptr(struct vstack *vs, int ptr)
{
	static char buf[VSTACK_STR_SZ];
	return vstack_str_r_ptr(buf, vs, ptr);
}

int impl_reg_to_scratch(const struct vreg *r)
{
	return r->idx + (r->is_float ? N_SCRATCH_REGS_I : 0);
}

void impl_scratch_to_reg(int scratch, struct vreg *r)
{
	if(scratch >= N_SCRATCH_REGS_I){
		r->idx = scratch - N_SCRATCH_REGS_I;
		r->is_float = 1;
	}else{
		r->idx = scratch;
		r->is_float = 0;
	}
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
		DIE_AT(&fr->where, "variadic functions can't be callee cleanup");

	return cr_clean;
}

static void x86_call_regs(type_ref *fr, int *pn, const struct vreg **par)
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
	int i;

	if(r->is_float)
		return 0;

	ent = x86_conv_lookup(fr);
	for(i = 0; i < ent->n_callee_save_regs; i++)
		if(ent->callee_save_regs[i] == r->idx)
			return 1;

	return 0;
}

int impl_n_call_regs(type_ref *rf)
{
	int n;
	x86_call_regs(rf, &n, NULL);
	return n;
}

void impl_func_prologue_save_fp(void)
{
	out_asm("pushq %%rbp");
	out_asm("movq %%rsp, %%rbp");
}

void impl_func_prologue_save_call_regs(type_ref *rf, int nargs)
{
	if(nargs){
		int n_call_regs;
		const struct vreg *call_regs;

		int i, i_f, i_i;
		int n_reg_args;

		funcargs *const fa = type_ref_funcargs(rf);

		x86_call_regs(rf, &n_call_regs, &call_regs);

		n_reg_args = MIN(nargs, n_call_regs);

		for(i = i_i = i_f = 0; i < n_reg_args; i++){
			type_ref *const ty = fa->arglist[i]->ref;

			if(type_ref_is_floating(ty)){
				unsigned sz = type_ref_size(ty, NULL);
				unsigned pos = v_alloc_stack(sz, &sz);

				out_asm("mov%s %%%s, -" NUM_FMT "(%%rbp)",
						x86_suffix(ty),
						/* just pass idx for the float index */
						x86_fpreg_str(i_f++),
						pos - sz);

			}else{
				out_asm("push%s %%%s",
						x86_suffix(NULL),
						x86_reg_str(&call_regs[i_i++], NULL));
			}
		}
	}
}

int impl_func_prologue_save_variadic(type_ref *rf, int nargs)
{
	int n_call_regs;
	const struct vreg *call_regs;
	char *vfin = out_label_code("va_skip_float");
	int sz = 0;
	int i;

	x86_call_regs(rf, &n_call_regs, &call_regs);

	/* go backwards, as we want registers pushed in reverse
	 * so we can iterate positively */
	for(i = n_call_regs - 1; i >= nargs; i--){
		/* TODO: do this with out_save_reg */
		out_asm("push%s %%%s",
				x86_suffix(NULL),
				x86_intreg_str(i, NULL));
		sz += platform_word_size();
	}

	/* TODO: do this with out_* */
	out_asm("testb %%al, %%al");
	out_asm("jz %s", vfin);

	out_asm(IMPL_COMMENT "pushq %%xmm0 TODO - float regs");
	/* TODO: add to sz */

	out_label(vfin);
	free(vfin);

	return sz;
}

void impl_func_epilogue(type_ref *rf)
{
	out_asm("leaveq");

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
#define OP(e, s, u) case flag_ ## e: return flag->is_signed ? s : u
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

static void x86_load(struct vstack *from, const struct vreg *reg, int lea)
{
	switch(from->type){
		case FLAG:
			UCC_ASSERT(!lea, "lea FLAG");

			out_comment("zero for set");
			out_asm("mov%s $0, %%%s",
					x86_suffix(from->t),
					x86_reg_str(reg, from->t));

			/* XXX: memleak */
			from->t = type_ref_cached_CHAR(); /* force set%s to set the low byte */
			out_asm("set%s %%%s",
					x86_cmp(&from->bits.flag),
					x86_reg_str(reg, from->t));
			return;

		case CONST_F:
			ICE("TODO: load float");
		case REG:
			UCC_ASSERT(!lea, "lea REG");
		case STACK:
		case LBL:
		case STACK_SAVE:
		case CONST_I:
			/* XXX: do we really want to use from->t here? (when lea)
			 * the middle-end should take care of it in folds */
			out_asm("%s%s %s, %%%s",
					lea ? "lea" : "mov",
					x86_suffix(lea && type_ref_is_floating(from->t)
						? NULL
						: from->t),
					vstack_str(from),
					x86_reg_str(reg, from->t));
			break;
	}
}

void impl_load_iv(struct vstack *vp)
{
	if(integral_is_64_bit(vp->bits.val_i, vp->t)){
		char buf[INTEGRAL_BUF_SIZ];
		struct vreg r;
		v_unused_reg(1, 0, &r);

		/* TODO: 64-bit registers in general on 32-bit */
		UCC_ASSERT(!cc1_m32, "TODO: 32-bit 64-literal loads");

		UCC_ASSERT(type_ref_size(vp->t, NULL) == 8,
				"loading 64-bit literal (%lld) for non-long? (%s)",
				vp->bits.val_i, type_ref_to_str(vp->t));

		integral_str(buf, sizeof buf,
				vp->bits.val_i, vp->t);

		out_asm("movabsq $%s, %%%s",
				buf, x86_reg_str(&r, vp->t));

		vp->type = REG;
		memcpy_safe(&vp->bits.reg, &r);
	}
}

void impl_load(struct vstack *from, const struct vreg *reg)
{
	/* TODO: push down logic? */
	if(from->type == REG && vreg_eq(reg, &from->bits.reg))
		return;

	x86_load(from, reg, 0);
}

void impl_lea(struct vstack *of, const struct vreg *reg)
{
	x86_load(of, reg, 1);
}

void impl_store(struct vstack *from, struct vstack *to)
{
	char vbuf[VSTACK_STR_SZ];
	int ptr = 1;

	/* from must be either a reg, value or flag */
	if(from->type == FLAG && to->type == REG){
		/* setting a register from a flag - easy */
		impl_load(from, &to->bits.reg);
		return;
	}

	if(from->type != CONST_I)
		v_to_reg(from);

	switch(to->type){
		case FLAG:
		case STACK_SAVE:
		case CONST_F:
			ICE("invalid store %d", to->type);

		case LBL:
			ptr = 0;
		case REG:
		case CONST_I:
		case STACK:
			out_asm("mov%s %s, %s",
					x86_suffix(from->t),
					vstack_str_r(vbuf, from),
					vstack_str_ptr(to, ptr));
			break;
	}
}

void impl_reg_swp(struct vstack *a, struct vstack *b)
{
	struct vreg tmp;

	UCC_ASSERT(a->type == b->type && a->type == REG,
			"%s without regs (%d and %d)", __func__, a->type, b->type);

	out_asm("xchg %%%s, %%%s",
			reg_str(a), reg_str(b));

	tmp = a->bits.reg;
	a->bits.reg = b->bits.reg;
	b->bits.reg = tmp;
}

void impl_reg_cp(struct vstack *from, const struct vreg *r)
{
	char buf_v[VSTACK_STR_SZ];
	const char *regstr;

	if(from->type == REG && vreg_eq(&from->bits.reg, r))
		return;

	regstr = x86_reg_str(r, from->t);

	out_asm("mov%s %s, %%%s",
			x86_suffix(from->t),
			vstack_str_r(buf_v, from),
			regstr);
}

void impl_op(enum op_type op)
{
	const char *opc;

	switch(op){
#define OP(e, s) case op_ ## e: opc = s; break
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

				case REG:
					free_this = vtop->t = type_ref_cached_CHAR();

					rtmp.is_float = 0, rtmp.idx = X86_64_REG_RCX;
					if(!vreg_eq(&vtop->bits.reg, &rtmp)){
						impl_reg_cp(vtop, &rtmp);
						memcpy_safe(&vtop->bits.reg, &rtmp);
					}
					break;

				case CONST_F:
					ICE("float shift");
				case CONST_I:
					break;
			}

			vstack_str_r(bufs, vtop);
			vstack_str_r(bufv, &vtop[-1]);

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
				if(vtop->type == REG && vtop->bits.reg.idx == X86_64_REG_RAX){
					impl_reg_swp(vtop, &vtop[-1]);
				}else{
					v_freeup_reg(&rtmp[0], 2);
					impl_reg_cp(&vtop[-1], &rtmp[0]);
					vtop[-1].bits.reg.idx = X86_64_REG_RAX;
				}

				rdiv.idx = vtop[-1].bits.reg.idx;
			}

			UCC_ASSERT(rdiv.idx == X86_64_REG_RAX,
					"register A not chosen for idiv (%s)", x86_intreg_str(rdiv.idx, NULL));

			/* idiv takes either a reg or memory address */
			switch(vtop->type){
				default:
					v_to_reg(vtop);
					/* fall */

				case REG:
					if(vtop->bits.reg.idx == X86_64_REG_RDX){
						/* prevent rdx in division operand */
						struct vreg r;
						v_unused_reg(1, 0, &r);
						impl_reg_cp(vtop, &r);
						memcpy_safe(&vtop->bits.reg, &r);
					}

				case STACK:
					out_asm("cqto");
					out_asm("idiv%s %s", x86_suffix(vtop->t), vstack_str(vtop));
			}

			v_unreserve_reg(&rtmp[1]); /* free rdx */

			vpop();

			v_clear(vtop, vtop->t);
			vtop->type = REG;

			/* this is fine - we always use int-sized arithmetic or higher
			 * (in the char case, we would need ah:al
			 */
			vtop->bits.reg.idx = op == op_modulus ? X86_64_REG_RDX : X86_64_REG_RAX;
			vtop->bits.reg.is_float = 0;
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

			v_to_reg_const(vtop);
			v_to_reg_const(vtop - 1);

			/* if we have a const, it must be the first arg */
			if(vtop[-1].type == CONST_I){
				vswap();
				inv = 1;
			}

			/* if we have a CONST, it'll be in vtop,
			 * try a test instruction */
			if((op == op_eq || op == op_ne)
			&& vtop->type == CONST_I
			&& vtop->bits.val_i == 0)
			{
				const char *vstr = vstack_str(vtop - 1); /* vtop[-1] is REG */
				out_asm("test%s %s, %s", x86_suffix(vtop[-1].t), vstr, vstr);
			}else{
				out_asm("cmp%s %s, %s",
						x86_suffix(vtop[-1].t), /* pick the non-const one (for type-ing) */
						vstack_str(       vtop),
						vstack_str_r(buf, vtop - 1));
			}

			vpop();
			v_clear(vtop, type_ref_cached_BOOL()); /* cmp creates an int/bool */
			vtop->type = FLAG;
			vtop->bits.flag.cmp = op_to_flag(op);
			vtop->bits.flag.is_signed = is_signed;
			if(inv)
				v_inv_cmp(vtop);
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

		v_to_reg_const(vtop);
		v_to_reg_const(vtop - 1);

		/* vtop[-1] is a constant - needs to be in a reg */
		if(vtop[-1].type != REG){
			/* if the op is commutative, swap */
			if(op_is_commutative(op))
				out_swap();
			else
				v_to_reg(vtop - 1);
		}

		/* if both are constants, v_to_reg one */
		if(vtop->type == CONST_I && vtop[-1].type == CONST_I)
			/* -1 is where the op is going (see end of this block) */
			v_to_reg(vtop - 1);

		/* TODO: -O1
		 * if the op is commutative and we have REG_RET,
		 * make it the result reg
		 */

		switch(op){
			case op_plus:
			case op_minus:
				/* use inc/dec if possible */
				if(vtop->type == CONST_I
				&& vtop->bits.val_i == 1
				&& vtop[-1].type == REG)
				{
					out_asm("%s%s %s",
							op == op_plus ? "inc" : "dec",
							x86_suffix(vtop->t),
							vstack_str(&vtop[-1]));
					break;
				}
			default:
				out_asm("%s%s %s, %s", opc,
						x86_suffix(vtop->t),
						vstack_str_r(buf, &vtop[ 0]),
						vstack_str(       &vtop[-1]));
		}

		/* remove first operand - result is then in vtop (already in a reg) */
		vpop();
	}
}

void impl_deref_reg(const struct vreg *to)
{
	char ptr[VSTACK_STR_SZ];
	type_ref *ty;

	UCC_ASSERT(vtop->type == REG, "not reg (%d)", vtop->type);

	vstack_str_r_ptr(ptr, vtop, 1);

	ty = type_ref_ptr_depth_dec(vtop->t, NULL);

	/* loaded the pointer, now we apply the deref change */
	out_asm("mov%s %s, %%%s",
			x86_suffix(ty),
			ptr,
			x86_reg_str(to, ty));
}

void impl_op_unary(enum op_type op)
{
	const char *opc;

	v_to_reg_const(vtop);

	switch(op){
		default:
			ICE("invalid unary op %s", op_to_str(op));

		case op_plus:
			/* noop */
			return;

#define OP(o, s) case op_ ## o: opc = #s; break
		OP(minus, neg);
		OP(bnot, not);
#undef OP

		case op_not:
			out_push_i(vtop->t, 0);
			out_op(op_eq);
			return;
	}

	out_asm("%s %s", opc, vstack_str(vtop));
}

void impl_cast_load(struct vstack *vp, type_ref *small, type_ref *big, int is_signed)
{
	/* we are always up-casting here, i.e. int -> long */
	const unsigned int_sz = type_primitive_size(type_int);
	char buf_small[VSTACK_STR_SZ];

	UCC_ASSERT(type_ref_is_floating(small) == type_ref_is_floating(big),
			"float <-> int mismatch");

	/* temp assert */
	UCC_ASSERT(!type_ref_is_floating(small), "can't cast-load floats yet");

	switch(vp->type){
		case STACK:
		case STACK_SAVE:
		case LBL:
			/* something like movsx -8(%rbp), %rax */
			vstack_str_r(buf_small, vp);
			break;

		case CONST_F:
			ICE("TODO: cast load float");
		case CONST_I:
		case FLAG:
			v_to_reg(vp);
		case REG:
			strcpy(buf_small, x86_reg_str(&vp->bits.reg, small));

			if(!is_signed
			&& type_ref_size(big,   NULL) > int_sz
			&& type_ref_size(small, NULL) == int_sz)
			{
				/*
				 * movzx %eax, %rax is invalid since movl %eax, %eax
				 * automatically zeros the top half of rax in x64 mode
				 */
				out_asm("movl %%%s, %%%s", buf_small, buf_small);
				return;
			}
	}

	{
		struct vreg r;

		v_unused_reg(1, 0, &r);

		out_asm("mov%cx %%%s, %%%s",
				"zs"[is_signed],
				buf_small,
				x86_reg_str(&r, big));

		vp->type = REG;
		memcpy_safe(&vp->bits.reg, &r);
	}
}

static void x86_xchg_fi(struct vstack *vp, type_ref *tfrom, type_ref *tto)
{
	const int to_float = type_ref_is_floating(tto);
	struct vreg r;

	UCC_ASSERT(type_ref_is_type(to_float ? tto : tfrom, type_float),
			"TODO: non-float cast");

	v_unused_reg(1, to_float, &r);

	v_to_reg(vp);

	out_asm("cvt%s2%s %%%s, %%%s",
			to_float ? "si" : "ss",
			to_float ? "ss" : "si",
			x86_reg_str(&vp->bits.reg, tfrom),
			x86_reg_str(&r, tto));

	vp->type = REG;
	memcpy_safe(&vp->bits.reg, &r);
}

void impl_i2f(struct vstack *vp, type_ref *t_i, type_ref *t_f)
{
	x86_xchg_fi(vp, t_i, t_f);
}

void impl_f2i(struct vstack *vp, type_ref *t_f, type_ref *t_i)
{
	x86_xchg_fi(vp, t_f, t_i);
}

static const char *x86_call_jmp_target(struct vstack *vp, int no_rax)
{
	static char buf[VSTACK_STR_SZ + 2];

	switch(vp->type){
		case LBL:
			return vp->bits.lbl.str;

		case STACK:
			/* jmp *-8(%rbp) */
			*buf = '*';
			vstack_str_r(buf + 1, vp);
			return buf;

		case CONST_F:
		case FLAG:
			ICE("jmp flag/float?");
		case STACK_SAVE:
		case REG:
		case CONST_I:
			v_to_reg(vp); /* again, v_to_reg_preferred(), except that we don't want a reg */

			UCC_ASSERT(!vp->bits.reg.is_float, "jmp float?");

			if(no_rax && vp->bits.reg.idx == X86_64_REG_RAX){
				struct vreg r;
				v_unused_reg(1, 0, &r);
				impl_reg_cp(vp, &r);
				memcpy_safe(&vp->bits.reg, &r);
			}

			snprintf(buf, sizeof buf, "*%%%s", reg_str(vp));

			return buf;
	}

	ICE("invalid jmp target");
	return NULL;
}

void impl_jmp_lbl(const char *lbl)
{
	out_asm("jmp %s", lbl);
}

void impl_jmp_reg(const struct vreg *r)
{
	UCC_ASSERT(!r->is_float, "jmp float?");
	out_asm("jmp *%s", x86_intreg_str(r->idx, NULL));
}

void impl_jcond(int true, const char *lbl)
{
	switch(vtop->type){
		case FLAG:
			UCC_ASSERT(true, "jcond(false) for flag - should've been inverted");

			out_asm("j%s %s", x86_cmp(&vtop->bits.flag), lbl);
			break;

		case CONST_F:
			ICE("jcond float");
		case CONST_I:
			if(true == !!vtop->bits.val_i){
				out_asm("jmp %s", lbl);
				out_comment("// constant jmp condition %" NUMERIC_FMT_D,
						vtop->bits.val_i);
			}
			break;

		case STACK:
		case STACK_SAVE:
		case LBL:
			v_to_reg(vtop);

		case REG:
		{
			const char *rstr = reg_str(vtop);

			out_asm("test %%%s, %%%s", rstr, rstr);
			out_asm("j%sz %s", true ? "n" : "", lbl);
		}
	}
}

void impl_call(const int nargs, type_ref *r_ret, type_ref *r_func)
{
	int n_call_regs;
	const struct vreg *call_regs;
	int i, ncleanup;
	int nfloats = 0, nints = 0;

	x86_call_regs(r_func, &n_call_regs, &call_regs);

	(void)r_ret;

	/* pre-scan of arguments - eliminate flags
	 * (should only be one,
	 * since we can only have one flag at a time)
	 */
	for(i = 0; i < MIN(nargs, n_call_regs); i++)
		if(vtop->type == FLAG){
			v_to_reg(vtop);
			break;
		}

	for(i = 0; i < MIN(nargs, n_call_regs); i++){
		const struct vreg *rp;
		struct vreg r;

		if(type_ref_is_floating(vtop->t)){
			r.idx = nfloats++;
			r.is_float = 1;

			rp = &r;
		}else{
			rp = &call_regs[nints++];
		}

		v_freeup_reg(rp, 0);
		v_to_reg_given(vtop, rp);
		v_reserve_reg(rp); /* we vpop but we don't want this reg clobbering */
		vpop();
	}

	/* amount of remaining arguments */
	ncleanup = nargs - i;

	/* save all registers before pushing remaining args
	 * otherwise we may have a vstack entry in a call
	 * register, which will mess everything up
	 */
	v_save_regs(ncleanup, r_func);

	/* push remaining args onto the stack, left to right */
	for(; i < nargs; i++){
		struct vstack *vp = &vtop[-(nargs - i) + 1]; /* reverse order for stack push */
#define INC_NFLOATS(t) if(t && type_ref_is_floating(t)) ++nfloats

		INC_NFLOATS(vp->t);

		/* can't push non-word sized vtops */
		if(vp->t && type_ref_size(vp->t, NULL) != platform_word_size())
			v_cast(vp, type_ref_cached_VOID_PTR());

		switch(vtop->type){
			case STACK_SAVE:
			case FLAG:
			case LBL:
			case CONST_F:
				/* can't push the vstack_str repr. of this */
				v_to_reg(vtop);

			case STACK:
			case CONST_I:
			case REG:
				break;
		}

		out_asm("pushq %s", vstack_str(vp));
	}
	for(i = 0; i < ncleanup; i++)
		vpop();

	{
		funcargs *args = type_ref_funcargs(r_func);
		int need_float_count = args->variadic || (!args->arglist && !args->args_void);
		/* jtarget must be assigned before "movb $0, %al" */
		const char *jtarget = x86_call_jmp_target(vtop, need_float_count);

		/* if x(...) or x() */
		if(need_float_count)
			out_asm("movb $%d, %%al", nfloats); /* we can never have a funcptr in rax, so we're fine */

		out_asm("callq %s", jtarget);
	}

	for(i = 0; i < MIN(nargs, n_call_regs); i++)
		v_unreserve_reg(&call_regs[i]);

	if(ncleanup && x86_caller_cleanup(r_func))
		out_asm("addq $" NUM_FMT ", %%rsp", ncleanup * platform_word_size());
}

void impl_undefined(void)
{
	out_asm("ud2");
}

void impl_set_overflow(void)
{
	vtop->type = FLAG;
	vtop->bits.flag.cmp = flag_overflow;
}

int impl_frame_ptr_to_reg(int nframes)
{
	const char *rstr;
	struct vreg r;

	v_unused_reg(1, 0, &r);

	rstr = x86_reg_str(&r, NULL);

	out_asm("movq %%rbp, %%%s", rstr);
	while(--nframes > 0)
		out_asm("movq (%%%s), %%%s", rstr, rstr);

	return r.idx;
}
