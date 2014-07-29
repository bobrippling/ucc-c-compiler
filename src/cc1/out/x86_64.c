#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

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
#include "../defs.h"
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

enum stret
{
	stret_scalar,
	stret_regs,
	stret_memcpy
};

static enum stret x86_stret(type *ty, unsigned *stack_space)
{
	/*
	 * in short:
	 * rdx:rax
	 *   all integer structs from char <--> long long
	 *
	 * xmm0:xmm1
	 *   all float structs from float <--> double[2] / double+float[2]
	 *
	 * x87 (TODO)
	 *   struct { long double; } // anything larger is in memory
	 *   return (_Complex long double)0 // x87
	 *   return (struct { _Complex long double cld; }){ 0 }; // memory
	 *
	 * xmm0:rax
	 *   struct { char<-->long, float[1..2]/double }
	 * rax:xmm0
	 *   inverse of above
	 *
	 * memory:
	 *   struct { char[17..]/short[9..]/etc }
	 *   struct { char, short, int, char } // final char busts it due to padding
	 */
	unsigned sz;
	struct_union_enum_st *su = type_is_s_or_u(ty);

	if(!su){
		if(stack_space)
			*stack_space = 0;
		return stret_scalar;
	}

	if(IS_32_BIT())
		ICE("TODO: 32-bit stret");

	sz = type_size(ty, NULL);

	/* We unconditionally want to spill rdx:rax to the stack on return.
	 * This could be optimised in the future
	 * (in a similar vein as long long on x86/32-bit)
	 * so that we can handle vtops with structure/union type
	 * and multiple registers.
	 *
	 * Hence, space needed for both reg and memcpy returns
	 */
	if(stack_space)
		*stack_space = sz;

	/* rdx:rax? */
	if(sz > 2 * platform_word_size())
		return stret_memcpy;

	return stret_regs;
}

enum regtype
{
	NONE,
	INT,
	FLOAT
};

static void x86_overlay_regpair_1(
		struct vreg regs[], enum regtype chosentype, int *regpair_idx,
		const struct vreg from_intregs[])
{
	switch(chosentype){
		case NONE:
		case INT:
			regs[*regpair_idx].is_float = 0;

			regs[*regpair_idx].idx =
				(*regpair_idx == 0 || regs[0].is_float)
				? from_intregs[0].idx
				: from_intregs[1].idx;
			break;

		case FLOAT:
			regs[*regpair_idx].is_float = 1;

			regs[*regpair_idx].idx =
				(*regpair_idx == 0 || !regs[0].is_float)
				? X86_64_REG_XMM0
				: X86_64_REG_XMM1;
			break;
	}

	++*regpair_idx;
}

static void x86_overlay_regpair(
		struct vreg out_regpair[ucc_static_param 2], type *retty,
		const struct vreg int_regpair[ucc_static_param 2])
{
	/* if we have two floats at either 0-1 or 2-3, then we can do
	 * a xmm0:rax or rax:xmm0 return. Otherwise we fallback to rdx:rax overlay
	 *
	 * x86_64 ABI, 3.2.3:
	 *
	 * 4. Each field [...] classified recursively so that always two fields
	 * are considered.
	 *   (a) If both classes are equal, this is the resulting class.
	 *   // adjacent floats or ints
	 *   (b) If one of the classes is NO_CLASS, the resulting class is the other.
	 *   // empty structs - ignored
	 *   (c) If one of the classes is MEMORY, the result is the MEMORY class.
	 *   // big struct, etc
	 *   (d) If one of the classes is INTEGER, the result is the INTEGER.
	 *   // float,int -> INTEGER; int,int -> INTEGER
	 *   (e) If one of the classes is X87, X87UP, COMPLEX_X87, MEMORY is used.
	 *   // ignored for now
	 *   (f) Otherwise class SSE is used.
	 *   // ignored for now
	 */

	struct_union_enum_st *su = type_is_s_or_u(retty);
	sue_member **mi;
	unsigned current_size_bits = 0;
	enum regtype current_type = NONE;
	int regpair_idx = 0;

	UCC_ASSERT(su->primitive != type_enum, "enum?");

	for(mi = su->members; mi && *mi; mi++){
		decl *mem = (*mi)->struct_member;
		type *ty = mem->ref;

		enum regtype this_type = type_is_floating(ty) ? FLOAT : INT;

		if(current_type == NONE)
			current_type = this_type;
		else if(this_type != current_type)
			current_type = INT; /* floats defer to ints */

		if(mem->bits.var.field_width){
			const unsigned bits = const_fold_val_i(
					mem->bits.var.field_width);

			current_size_bits += bits;

		}else{
			current_size_bits += CHAR_BIT * type_size(ty, NULL);
		}

		/* if we pass 64... */
		if(current_size_bits >= CHAR_BIT * 8){
			UCC_ASSERT(regpair_idx < 2, "too many regpairs");

			/* hit one eightbyte, decide how we pass this group */
			x86_overlay_regpair_1(
					out_regpair,
					current_type,
					&regpair_idx,
					int_regpair);

			current_type = NONE;
			current_size_bits = 0;
		}
	}

	if(regpair_idx < 2){
		x86_overlay_regpair_1(
				out_regpair,
				current_type,
				&regpair_idx,
				int_regpair);
	}
}

static void x86_overlay_regpair_ret(
		struct vreg out_regpair[ucc_static_param 2], type *retty)
{
	struct vreg int_retregs[] = {
		{ X86_64_REG_RAX, 0 },
		{ X86_64_REG_RDX, 0 },
	};

	x86_overlay_regpair(out_regpair, retty, int_retregs);
}

static void x86_overlay_regpair_args(
		struct vreg out_regpair[ucc_static_param 2],
		type *arg_suty,
		unsigned *const reg_idx, const struct vreg *call_regs)
{
	x86_overlay_regpair(out_regpair, arg_suty, call_regs + *reg_idx);

	*reg_idx += 2;
}

static const out_val *x86_stret_ptr(out_ctx *octx)
{
	type *voidpp
		= type_ptr_to(
				type_ptr_to(
					type_nav_btype(cc1_type_nav, type_void)));

	return out_deref(octx,
			v_new_bp3(octx, NULL, voidpp,
				/* stret is always the first argument:*/
				- (long) platform_word_size()));
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
		char buf[VSTACK_STR_SZ], const out_val *vs, const int deref)
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
			const char *pre = deref ? "" : "$";
			const char *picstr = pic && deref ? "(%rip)" : "";

			if(vs->bits.lbl.offset){
				SNPRINTF(buf, VSTACK_STR_SZ, "%s%s+%ld%s",
						pre,
						vs->bits.lbl.str,
						vs->bits.lbl.offset,
						picstr);
			}else{
				SNPRINTF(buf, VSTACK_STR_SZ, "%s%s%s",
						pre, vs->bits.lbl.str, picstr);
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
				UCC_ASSERT(1||deref,
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

static const char *vstack_str(const out_val *vs, int deref)
{
	static char buf[VSTACK_STR_SZ];
	return vstack_str_r(buf, vs, deref);
}

int impl_reg_to_idx(const struct vreg *r)
{
	return r->idx + (r->is_float ? N_SCRATCH_REGS_I : 0);
}

void impl_scratch_to_reg(int scratch, struct vreg *r)
{
	r->idx = scratch - (r->is_float ? N_SCRATCH_REGS_I : 0);
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

int impl_reg_frame_const(const struct vreg *r, int sp)
{
	if(r->is_float)
		return 0;
	switch(r->idx){
		case X86_64_REG_RBP:
			return 1;
		case X86_64_REG_RSP:
			if(sp)
				return 1;
	}
	return 0;
}

int impl_reg_is_scratch(const struct vreg *r)
{
	return r->is_float || r->idx <= X86_64_REG_R15;
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

static void set_arg_offset(
		int arg_offsets[/*nargs*/],
		unsigned idx,
		unsigned stret,
		int val)
{
	if(idx >= stret)
		arg_offsets[idx - stret] = val;
}

void impl_func_prologue_save_call_regs(
		out_ctx *octx,
		type *rf, unsigned nargs,
		int arg_offsets[/*nargs*/])
{
	int is_stret = 0;

	/* save the stret hidden argument */
	switch(x86_stret(type_func_call(rf, NULL), NULL)){
		case stret_regs:
		case stret_scalar:
			break;
		case stret_memcpy:
			nargs++;
			is_stret = 1;
	}

	if(nargs){
		const unsigned ws = platform_word_size();

		funcargs *const fa = type_funcargs(rf);

		unsigned n_call_i, n_call_f;
		const struct vreg *call_regs;
		unsigned i_arg, i_stk, i_arg_stk, i_i, i_f;

		n_call_f = N_CALL_REGS_F;
		x86_call_regs(rf, &n_call_i, &call_regs);

		{
			/* trim by the number of args */
			unsigned fp_cnt, int_cnt;

			funcargs_ty_calc(fa, &int_cnt, &fp_cnt);

			int_cnt += is_stret;

			n_call_i = MIN(n_call_i, int_cnt);
			n_call_f = MIN(n_call_f, fp_cnt);
		}

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

				set_arg_offset(arg_offsets, i_arg, is_stret, -off);
			}

			continue;
pass_via_stack:
			set_arg_offset(arg_offsets, i_arg, is_stret, (i_arg_stk++ + 2) * ws);
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

	stk_top = octx->var_stack_sz;

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
		out_comment(octx, "stack at %u bytes", octx->max_stack_sz);

	/* callee cleanup */
	if(!x86_caller_cleanup(rf)){
		const int nargs = x86_func_nargs(rf);

		out_asm(octx, "retq $%d", nargs * platform_word_size());
	}else{
		out_asm(octx, "retq");
	}
}

static ucc_wur const out_val *
x86_func_ret_memcpy(
		out_ctx *octx,
		struct vreg *ret_reg,
		type *called,
		const out_val *from)
{
	const out_val *stret_p;

	/* copy from *%rax to *%rdi (first argument) */
	out_comment(octx, "stret copy");

	stret_p = x86_stret_ptr(octx);

	out_val_retain(octx, stret_p);

	out_flush_volatile(octx,
			out_memcpy(octx, stret_p, from, type_size(called, NULL)));

	/* return the stret pointer argument */
	ret_reg->is_float = 0;
	ret_reg->idx = REG_RET_I;

	return stret_p;
}

static void x86_func_ret_regs(
		out_ctx *octx, type *called, const out_val *from)
{
	const unsigned sz = type_size(called, NULL);
	struct vreg regs[2];

	x86_overlay_regpair_ret(regs, called);

	/* read from the stack to registers */
	impl_overlay_mem2regs(octx, sz, 2, regs, from);
}

void impl_to_retreg(out_ctx *octx, const out_val *val, type *called)
{
	struct vreg r;

	switch(x86_stret(called, NULL)){
		case stret_memcpy:
			/* this function is responsible for memcpy()ing
			 * the struct back */
			val = x86_func_ret_memcpy(octx, &r, called, val);
			break;

		case stret_scalar:
			r.is_float = type_is_floating(called);
			r.idx = r.is_float ? REG_RET_F : REG_RET_I;
			break;

		case stret_regs:
			/* this function returns in regs, the caller is
			 * responsible doing what it wants,
			 * i.e. memcpy()ing the regs into stack space */
			x86_func_ret_regs(octx, called, val);
			return;
	}

	/* not done for stret_regs: */

	/* v_to_reg since we don't handle lea/load ourselves */
	out_flush_volatile(octx, v_to_reg_given(octx, val, &r));
}

static const char *x86_cmp(const struct flag_opts *flag)
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

static const out_val *x86_load_iv(
		out_ctx *octx, const out_val *from,
		const struct vreg *reg /* may be null */)
{
	const int high_bit = integral_high_bit_ABS(from->bits.val_i, from->t);
	struct vreg r;

	assert(from->type == V_CONST_I);
	assert(!type_is_floating(from->t));

	if(high_bit >= AS_MAX_MOV_BIT){
		char buf[INTEGRAL_BUF_SIZ];

		if(!reg){
			reg = &r;
			v_unused_reg(octx, 1, 0, &r, /*from isn't a reg:*/NULL);
		}

		/* TODO: 64-bit registers in general on 32-bit */
		UCC_ASSERT(!IS_32_BIT(), "TODO: 32-bit 64-literal loads");

		if(high_bit > 31 /* not necessarily AS_MAX_MOV_BIT */){
			/* must be loading a long */
			if(type_size(from->t, NULL) != 8){
				/* FIXME: enums don't auto-size currently */
				ICW("loading 64-bit literal (%lld) for non-8-byte type? (%s)",
						from->bits.val_i, type_to_str(from->t));
			}
		}

		integral_str(buf, sizeof buf, from->bits.val_i, NULL);

		out_asm(octx, "movabsq $%s, %%%s", buf, x86_reg_str(reg, NULL));
	}else{
		if(!reg)
			return from; /* V_CONST_I is fine */

		out_asm(octx, "mov%s %s, %%%s",
				x86_suffix(from->t),
				vstack_str(from, 0),
				x86_reg_str(reg, from->t));
	}

	return v_new_reg(octx, from, from->t, reg);
}

static const out_val *x86_load_fp(out_ctx *octx, const out_val *from)
{
	switch(from->type){
		case V_CONST_I:
			/* CONST_I shouldn't be entered,
			 * type prop. should cast */
			ICE("load int into float?");

		case V_CONST_F:
			/* if it's an int-const, we can load without a label */
			if(from->bits.val_f == (integral_t)from->bits.val_f
			&& fopt_mode & FOPT_INTEGRAL_FLOAT_LOAD)
			{
				type *const ty_fp = from->t;
				out_val *mut = v_dup_or_reuse(octx, from, from->t);

				from = mut;

				mut->type = V_CONST_I;
				mut->bits.val_i = from->bits.val_f;
				/* TODO: use just an int if we can get away with it */
				mut->t = type_nav_btype(cc1_type_nav, type_llong);

				return out_cast(octx, mut, ty_fp, /*normalise_bool:*/1);
			}
			/* fall */

		default:
		{
			/* save to a label */
			char *lbl = out_label_data_store(STORE_FLOAT);
			struct vreg r;
			out_val *mut;

			asm_nam_begin3(SECTION_DATA, lbl, type_align(from->t, NULL));
			asm_out_fp(SECTION_DATA, from->t, from->bits.val_f);

			from = mut = v_dup_or_reuse(octx, from, from->t);

			v_unused_reg(octx,
					1, type_is_floating(from->t),
					&r,
					from);

			/* must treat this as a pointer, and dereference here,
			 * as we currently don't have V_LBL_SPILT, for e.g. */
			mut->type = V_LBL;
			mut->bits.lbl.str = lbl;
			mut->bits.lbl.pic = 1;
			mut->bits.lbl.offset = 0;
			mut->t = type_ptr_to(mut->t);

			return out_deref(octx, mut);
		}
	}
	/* unreachable */
}

static int x86_need_fp_parity_p(
		struct flag_opts const *fopt, int *flip_result)
{
	if(!(fopt->mods & flag_mod_float))
		return 0;

	*flip_result = 0;

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
			*flip_result = 1; /* a != a is true if a == nan */
			/* fall */
		default:
			return 1;
	}
}

const out_val *impl_load(
		out_ctx *octx,
		const out_val *from,
		const struct vreg *reg)
{
	if(from->type == V_REG
	&& vreg_eq(reg, &from->bits.regoff.reg))
	{
		return from;
	}

	switch(from->type){
		case V_FLAG:
		{
			type *int_ty = type_nav_btype(cc1_type_nav, type_int);
			type *char_ty = type_nav_btype(cc1_type_nav, type_nchar);
			const char *rstr;
			int flip_parity_ret;
			int chk_parity;

			rstr = x86_reg_str(reg, char_ty);

			/* check float/orderedness */
			chk_parity = x86_need_fp_parity_p(&from->bits.flag, &flip_parity_ret);

			/* movl $0, %eax */
			out_flush_volatile(octx,
					impl_load(
						octx,
						out_new_l(octx, int_ty, 0),
						reg));

			/* actual cmp */
			out_asm(octx, "set%s %%%s", x86_cmp(&from->bits.flag), rstr);

			if(chk_parity){
				struct vreg parity_reg;
				const char *parity_rstr;

				v_reserve_reg(octx, reg);
				v_unused_reg(octx, 1, 0, &parity_reg, NULL);
				v_unreserve_reg(octx, reg);

				parity_rstr = x86_reg_str(&parity_reg, char_ty);

				out_asm(octx, "set%sp %%%s",
						flip_parity_ret ? "" : "n",
						parity_rstr);

				out_asm(octx, "%sb %%%s, %%%s",
						flip_parity_ret ? "or" : "and",
						parity_rstr, rstr);

				out_asm(octx, "andb $1, %%%s", rstr);
			}

			if(type_size(from->t, NULL) != type_size(char_ty, NULL)){
				/* need to promote, since we forced char type */
				type *tto = from->t;

				/* 'from' is currently in a char type */
				from = v_new_reg(octx, from, char_ty, reg);

				/* convert to the type we had passed in */
				from = out_cast(octx, from, tto, 1);
			}
			break;
		}

		case V_REG_SPILT:
			/* actually a pointer to T */
			return impl_deref(octx, from, reg);

		case V_REG:
			if(from->bits.regoff.offset)
				goto lea;

			impl_reg_cp_no_off(octx, from, reg);
			break;

		case V_CONST_I:
			from = x86_load_iv(octx, from, reg);
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

			/* 'from' is now in a reg */
			break;
		}

		case V_CONST_F:
			from = x86_load_fp(octx, from);
			if(from->type != V_REG)
				from = impl_load(octx, from, reg);
	}

	return v_new_reg(octx, from, from->t, reg);
}

static const out_val *x86_check_ivfp(out_ctx *octx, const out_val *from)
{
	switch(from->type){
		case V_CONST_I:
			from = x86_load_iv(octx, from, NULL);
			break;
		case V_CONST_F:
			from = x86_load_fp(octx, from);
			break;
		default:
			break;
	}
	return from;
}

void impl_store(out_ctx *octx, const out_val *to, const out_val *from)
{
	char vbuf[VSTACK_STR_SZ];

	/* from must be either a reg, value or flag */
	if(from->type == V_FLAG
	&& to->type == V_REG)
	{
		/* the register we're storing into is an lvalue */
		struct vreg evalreg;

		/* setne %evalreg */
		v_unused_reg(octx, 1, 0, &evalreg, NULL);
		from = impl_load(octx, from, &evalreg);

		/* mov %evalreg, (from) */
		impl_store(octx, to, from);

		return;
	}

	from = v_to(octx, from, TO_REG | TO_CONST);
	from = x86_check_ivfp(octx, from);

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
			break;

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

static void x86_reg_cp(
		out_ctx *octx,
		const struct vreg *to,
		const struct vreg *from,
		type *typ)
{
	assert(!impl_reg_frame_const(to, 1));

	if(vreg_eq(to, from))
		return;

	out_asm(octx, "mov%s %%%s, %%%s",
			x86_suffix(typ),
			x86_reg_str(from, typ),
			x86_reg_str(to, typ));
}

void impl_reg_cp_no_off(
		out_ctx *octx, const out_val *from, const struct vreg *to_reg)
{
	UCC_ASSERT(from->type == V_REG,
			"reg_cp on non register type 0x%x", from->type);

	if(!from->bits.regoff.offset && vreg_eq(&from->bits.regoff.reg, to_reg))
		return;

	/* offset normalisation isn't handled here - caller's responsibility */
	x86_reg_cp(octx, to_reg, &from->bits.regoff.reg, from->t);
}

static const out_val *x86_idiv(
		out_ctx *octx, enum op_type op,
		const out_val *l, const out_val *r)
{
	/*
	 * divide the 64 bit integer edx:eax
	 * by the operand
	 * quotient  -> eax
	 * remainder -> edx
	 */
	const struct vreg rdx = { X86_64_REG_RDX, 0 };
	const struct vreg rax = { X86_64_REG_RAX, 0 };
	struct vreg result;

	/* freeup rdx. rax is freed as below: */
	v_freeup_reg(octx, &rdx);
	v_reserve_reg(octx, &rdx);
	{
		/* need to move 'l' into eax
		 * then sign extended later - cqto */
		l = v_to_reg_given_freeup(octx, l, &rax);

		/* idiv takes either a reg or memory address */
		r = v_to(octx, r, TO_REG | TO_MEM);

		assert(r->type != V_REG
				|| r->bits.regoff.reg.idx != X86_64_REG_RDX);

		out_asm(octx, "cqto");
		out_asm(octx, "idiv%s %s",
				x86_suffix(r->t),
				vstack_str(r, 0));

	}
	v_unreserve_reg(octx, &rdx);

	out_val_release(octx, r);

	/* this is fine - we always use int-sized arithmetic or higher
	 * (otherwise in the char case, we would need ah:al) */
	result.idx = (op == op_modulus ? X86_64_REG_RDX : X86_64_REG_RAX);
	result.is_float = 0;

	return v_new_reg(octx, l, l->t, &result);
}

static const out_val *x86_shift(
		out_ctx *octx, enum op_type op,
		const out_val *l, const out_val *r)
{
	char bufv[VSTACK_STR_SZ], bufs[VSTACK_STR_SZ];
	type *nchar;

	/* sh[lr] [r/m], [c/r]
	 * where            ^ must be %cl
	 */
	if(r->type != V_CONST_I){
		struct vreg cl;

		cl.is_float = 0;
		cl.idx = X86_64_REG_RCX;

		r = v_to_reg_given_freeup(octx, r, &cl);
	}

	/* force %cl: */
	nchar = type_nav_btype(cc1_type_nav, type_nchar);
	if(type_cmp(r->t, nchar, 0))
		r = v_dup_or_reuse(octx, r, nchar); /* change type */

	l = v_to(octx, l, TO_MEM | TO_REG);

	vstack_str_r(bufv, l, 0);
	vstack_str_r(bufs, r, 0);

	out_asm(octx, "%s%s %s, %s",
			op == op_shiftl
				? "shl"
				: type_is_signed(l->t) ? "sar" : "shr",
			x86_suffix(l->t),
			bufs, bufv);

	out_val_consume(octx, r);
	return v_dup_or_reuse(octx, l, l->t);
}

const out_val *impl_op(out_ctx *octx, enum op_type op, const out_val *l, const out_val *r)
{
	const char *opc;
	const out_val *min_retained = l->retains < r->retains ? l : r;

	l = x86_check_ivfp(octx, l);
	r = x86_check_ivfp(octx, r);

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
			const out_val *tmp = l;
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

			out_val_consume(octx, r);
			return v_dup_or_reuse(octx, l, l->t);
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
			return x86_shift(octx, op, l, r);

		case op_modulus:
		case op_divide:
			return x86_idiv(octx, op, l, r);

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
			const out_val *vconst = NULL;
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
				const out_val *vother = vconst == l ? r : l;
				const char *vstr = vstack_str(vother, 0); /* reg */
				out_asm(octx, "test%s %s, %s", x86_suffix(vother->t), vstr, vstr);
			}else{
				/* if we have a const, it must be the first arg */
				if(l->type == V_CONST_I){
					const out_val *tmp = l;
					l = r, r = tmp;
					inv = 1;
				}

				/* still a const? */
				if(l->type == V_CONST_I)
					l = v_to_reg(octx, l);

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
			/* XXX: currently implicit here that V_REG means w/no offset */
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

#define OP_MATCH(vp, op) (   \
		vp->type == ops[i].op && \
		(vp->type != V_REG || !vp->bits.regoff.offset))

		for(i = 0; i < ops_n; i++){
			if(OP_MATCH(l, l) && OP_MATCH(r, r)){
				satisfied = 1;
				break;
			}

			if(op_is_commutative(op)
			&& OP_MATCH(r, l) && OP_MATCH(l, r))
			{
				need_swap = satisfied = 1;
				break;
			}
		}

		if(need_swap){
			SWAP(const out_val *, l, r);
		}else if(!satisfied){
			/* try to keep rhs as const */
			l = v_to(octx, l, TO_REG | TO_MEM);
			r = v_to(octx, r, TO_REG | TO_MEM | TO_CONST);

			if(V_IS_MEM(l->type) && V_IS_MEM(r->type))
				r = v_to_reg(octx, r);
		}

		if(fopt_mode & FOPT_PIC){
			l = v_to(octx, l, TO_REG);
			r = v_to(octx, r, TO_REG | TO_CONST);
		}

		if(v_is_const_reg(l)){
			/* ^ only check 'l' - 'r' is an rvalue and not changed */
			struct vreg new_reg, old_reg;

			memcpy_safe(&old_reg, &l->bits.regoff.reg);

			v_unused_reg(octx, 1, 0, &new_reg, l);
			l = v_new_reg(octx, l, l->t, &new_reg);

			x86_reg_cp(octx, &new_reg, &old_reg, l->t);
		}

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

const out_val *impl_deref(out_ctx *octx, const out_val *vp, const struct vreg *reg)
{
	type *tpointed_to = vp->type == V_REG_SPILT
		? vp->t
		: type_dereference_decay(vp->t);

	/* loaded the pointer, now we apply the deref change */
	out_asm(octx, "mov%s %s, %%%s",
			x86_suffix(tpointed_to),
			vstack_str(vp, 1),
			x86_reg_str(reg, tpointed_to));

	return v_new_reg(octx, vp, tpointed_to, reg);
}

const out_val *impl_op_unary(out_ctx *octx, enum op_type op, const out_val *val)
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

const out_val *impl_cast_load(
		out_ctx *octx, const out_val *vp,
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
		out_val *vp_mut;

		/* mov[zs][bwl][wlq]
		 * avoid movzx - it's ambiguous
		 *
		 * special case: movzlq is invalid, we use movl %r, %r instead
		 */

		v_unused_reg(octx, 1, 0, &r, vp);
		vp = vp_mut = v_new_reg(octx, vp, vp->t, &r);

		if(!is_signed && *suffix_big == 'q' && *suffix_small == 'l'){
			out_comment(octx, "movzlq:");
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

		vp_mut->t = big;

		return vp;
	}
}

static void x86_fp_conv(
		out_ctx *octx,
		const out_val *vp,
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

static const out_val *x86_xchg_fi(
		out_ctx *octx, const out_val *vp,
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

	v_unused_reg(octx, 1, to_float, &r, vp);

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
			out_comment(octx, "%s to %s - truncated",
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

const out_val *impl_i2f(out_ctx *octx, const out_val *vp, type *t_i, type *t_f)
{
	return x86_xchg_fi(octx, vp, t_i, t_f);
}

const out_val *impl_f2i(out_ctx *octx, const out_val *vp, type *t_f, type *t_i)
{
	return x86_xchg_fi(octx, vp, t_f, t_i);
}

const out_val *impl_f2f(out_ctx *octx, const out_val *vp, type *from, type *to)
{
	struct vreg r;

	v_unused_reg(octx, 1, 1, &r, vp);
	assert(r.is_float);

	x86_fp_conv(octx, vp, &r, to, NULL,
			x86_suffix(from),
			x86_suffix(to));

	return v_new_reg(octx, vp, to, &r);
}

static const char *x86_call_jmp_target(
		out_ctx *octx, const out_val **pvp,
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

				*pvp = v_reg_apply_offset(octx, *pvp);

				v_unused_reg(octx, 1, 0, &r, /*don't want rax:*/NULL);
				impl_reg_cp_no_off(octx, *pvp, &r);

				assert((*pvp)->retains == 1);
				memcpy_safe(&((out_val *)*pvp)->bits.regoff.reg, &r);
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

void impl_jmp_expr(out_ctx *octx, const out_val *v)
{
	const char *jmp = x86_call_jmp_target(octx, &v, 0);
	out_asm(octx, "jmp %s", jmp);
	out_val_consume(octx, v);
}

void impl_branch(out_ctx *octx, const out_val *cond, out_blk *bt, out_blk *bf)
{
	switch(cond->type){
		case V_REG:
		{
			const char *rstr;
			char *cmp;

			cond = v_reg_apply_offset(octx, cond);
			rstr = vstack_str(cond, 0);

			out_asm(octx, "test %s, %s", rstr, rstr);
			cmp = ustrprintf("jz %s", bf->lbl);

			blk_terminate_condjmp(octx, cmp, bf, bt);
			break;
		}

		case V_FLAG:
		{
			char *cmpjmp;
			int parity_chk, flip_parity_ret;

			parity_chk = x86_need_fp_parity_p(&cond->bits.flag, &flip_parity_ret);

			if(parity_chk){
				/* nan means false, unless flip_parity_ret */
				char *parity_insn, *cmp_insn;

				cmp_insn = ustrprintf(
						"j%s %s",
						x86_cmp(&cond->bits.flag),
						bt->lbl);

				parity_insn = ustrprintf("jp %s",
						flip_parity_ret ? bt->lbl : bf->lbl);

				cmpjmp = ustrprintf(
						"%s\n\t%s",
						parity_insn,
						cmp_insn);

				free(cmp_insn);
				free(parity_insn);
			}else{
				cmpjmp = ustrprintf("j%s %s", x86_cmp(&cond->bits.flag), bt->lbl);
				/* fall thru to false block */
			}

			blk_terminate_condjmp(octx, cmpjmp, bt, bf);
			break;
		}

		case V_CONST_F:
			ICE("jcond float");

		case V_CONST_I:
			out_comment(octx,
					"constant jmp condition %staken",
					cond->bits.val_i ? "" : "not ");

			out_ctrl_transfer(octx, cond->bits.val_i ? bt : bf, NULL, NULL);
			break;

		case V_LBL:
		case V_REG_SPILT:
			cond = v_to_reg(octx, cond);

			UCC_ASSERT(cond->type != V_REG_SPILT,
					"normalise remained as spilt reg");

			cond = out_normalise(octx, cond);
			impl_branch(octx, cond, bt, bf);
			break;
	}
}

static const out_val *spill_struct_to_stack(
		out_ctx *octx, const out_val *struct_ptr, unsigned *arg_stack)
{
	unsigned su_sz = type_size(struct_ptr->t, NULL);

	(void)octx;

	if(su_sz < 2 * platform_word_size())
		return struct_ptr; /* spill to registers - done later */

	/* copy to stack */
	arg_stack += su_sz;

	return struct_ptr;
}

static const out_val *spill_struct_to_regs(
		out_ctx *octx, const out_val *struct_ptr,
		unsigned *callreg_idx, const struct vreg *call_regs)
{
	unsigned su_sz = type_size(struct_ptr->t, NULL);
	struct vreg regpair[2];

	if(su_sz > 2 * platform_word_size())
		return struct_ptr; /* already spilt to stack */

	x86_overlay_regpair_args(
			regpair, struct_ptr->t,
			callreg_idx, call_regs);

	struct_ptr = v_to_reg(octx, struct_ptr);

	out_val_retain(octx, struct_ptr);
	impl_overlay_mem2regs(octx, su_sz, 2, regpair, struct_ptr);

	return struct_ptr;
}

const out_val *impl_call(
		out_ctx *octx,
		const out_val *fn, const out_val **args,
		type *fnty)
{
	type *const voidp = type_ptr_to(type_nav_btype(cc1_type_nav, type_void));
	type *const retty = type_called(type_is_ptr_or_block(fnty), NULL);
	const unsigned pws = platform_word_size();
	const unsigned nargs = dynarray_count(args);
	char *const float_arg = umalloc(nargs);
	const out_val **local_args = NULL;
	const out_val *retval_stret;

	const struct vreg *call_iregs;
	unsigned n_call_iregs;

	unsigned nfloats = 0, nints = 0;
	unsigned arg_stack = 0;
	unsigned stk_snapshot = 0;
	unsigned i;
	unsigned stret_stack = 0, stret_pos = 0;
	enum stret stret_kind;

	dynarray_add_array(&local_args, args);

	x86_call_regs(fnty, &n_call_iregs, &call_iregs);

	/* pre-scan of arguments - eliminate flags, emplace structs
	 * (should only be one flag)
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

	for(i = 0; i < nargs; i++){
		if(!type_is_s_or_u(local_args[i]->t))
			continue;

		local_args[i] = spill_struct_to_stack(octx, local_args[i], &arg_stack);
	}


	/* hidden stret argument */
	switch((stret_kind = x86_stret(retty, &stret_stack))){
		case stret_memcpy:
			nints++; /* only an extra pointer arg for stret_memcpy */
			/* fall */
		case stret_regs:
			stret_stack = v_alloc_stack(octx, stret_stack, "stret space");
			stret_pos = octx->var_stack_sz;
		case stret_scalar:
			break;
	}

	/* do we need to do any stacking? */
	if(nints > n_call_iregs)
		arg_stack += pws * (nints - n_call_iregs);


	if(nfloats > N_CALL_REGS_F)
		arg_stack += pws * (nfloats - N_CALL_REGS_F);

	/* need to save regs before pushes/call */
	v_save_regs(octx, fnty, local_args, fn);

	if(arg_stack > 0){
		out_comment(octx, "stack space for %d arguments", arg_stack);
		/* this aligns the stack-ptr and returns arg_stack padded */
		arg_stack = v_alloc_stack(
				octx, arg_stack,
				"call argument space");
	}

	/* 16 byte for SSE - special case here as mstack_align may be less */
	if(!IS_32_BIT())
		v_stack_align(octx, 16, 0);

	if(arg_stack > 0){
		unsigned nfloats = 0, nints = 0; /* shadow */

		unsigned stack_pos;
		/* must be called after v_alloc_stack() */
		stk_snapshot = stack_pos = octx->var_stack_sz;
		out_comment(octx, "-- stack snapshot (%u) --", stk_snapshot);

		/* save in order */
		for(i = 0; i < nargs; i++){
			const int stack_this = float_arg[i]
				? nfloats++ >= N_CALL_REGS_F
				: nints++ >= n_call_iregs;

			if(stack_this){
				local_args[i] = v_to_stack_mem(octx,
						local_args[i], -(long)stack_pos);

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
	/* setup hidden stret pointer argument */
	if(stret_kind == stret_memcpy){
		const struct vreg *stret_reg = &call_iregs[nints];
		const out_val *stret_ptr;
		nints++;

		out_comment(octx, "stret pointer @ %u-%u for struct, size %u",
				stret_pos, stret_pos - stret_stack, stret_stack);

		stret_ptr = v_new_bp3(octx, NULL, voidp, -(long)stret_pos);

		v_freeup_reg(octx, stret_reg);
		out_flush_volatile(octx, v_to_reg_given(octx, stret_ptr, stret_reg));
	}

	for(i = 0; i < nargs; i++){
		const out_val *const vp = local_args[i];
		const int is_float = type_is_floating(vp->t);

		const struct vreg *rp = NULL;
		struct vreg r;

		if(type_is_s_or_u(vp->t))
			continue;

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

				UCC_ASSERT(local_args[i]->retains == 1,
						"incorrectly retained arg %d: %d",
						i, local_args[i]->retains);


				local_args[i] = v_to_reg_given(octx, local_args[i], rp);
			}
		}
		/* else already pushed */
	}

	for(i = 0; i < nargs; i++){
		if(!type_is_s_or_u(local_args[i]->t))
			continue;

		if(nints >= n_call_iregs){
			/* rest are spilt to the stack */
			ICE("TODO: not enough regs - spill structs to stack");
		}else{
			local_args[i] = spill_struct_to_regs(
					octx, local_args[i],
					&nints, call_iregs);
		}
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
		unsigned chg = octx->var_stack_sz - stk_snapshot;
		out_comment(octx, "-- restore snapshot (%u) --", chg);
		v_stack_adj(octx, chg, /*sub:*/0);
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

	if(arg_stack && !x86_caller_cleanup(fnty)){
		/* callee cleanup - the callee will have popped
		 * args from the stack, so we need a stack alloc
		 * to restore what we expect */
		v_stack_adj(octx, arg_stack, /*sub:*/1);
	}

	for(i = 0; i < nargs; i++)
		out_val_consume(octx, local_args[i]);
	dynarray_free(const out_val **, &local_args, NULL);

	if(stret_stack){
		if(stret_kind == stret_regs){
			/* we behave the same as stret_memcpy(),
			 * but we must spill the regs out */
			struct vreg regpair[2];

			x86_overlay_regpair_ret(regpair, retty);

			retval_stret = v_new_bp3(octx, NULL, voidp, -(long)stret_pos);
			out_val_retain(octx, retval_stret);

			/* spill from registers to the stack */
			impl_overlay_regs2mem(octx, stret_stack, 2, regpair, retval_stret);
		}
	}

	/* return type */
	free(float_arg);

	if(stret_kind != stret_regs){
		/* rax / xmm0, otherwise the return has
		 * been set to a local stack address */
		const int fp = type_is_floating(retty);
		struct vreg rr = VREG_INIT(fp ? REG_RET_F : REG_RET_I, fp);

		return v_new_reg(octx, fn, retty, &rr);
	}else{
		out_val_consume(octx, fn);
		return retval_stret;
	}
}

void impl_undefined(out_ctx *octx)
{
	out_asm(octx, "ud2");
	blk_terminate_undef(octx->current_blk);
}

const out_val *impl_test_overflow(out_ctx *octx, const out_val **eval)
{
	/* whenever creating a V_FLAG we need to ensure instructions are flushed */
	*eval = v_reg_apply_offset(octx, v_to_reg(octx, *eval));

	out_val_retain(octx, *eval);
	return v_new_flag(octx, *eval, flag_overflow, /*mod:*/0);
}

void impl_set_nan(out_ctx *octx, out_val *v)
{
	type *ty = v->t;

	(void)octx;

	UCC_ASSERT(type_is_floating(ty),
			"%s for %s", __func__, type_to_str(ty));

	assert(v->retains == 1);

	switch(type_size(ty, NULL)){
		case 4:
		{
			const union
			{
				unsigned l;
				float f;
			} u = { 0x7fc00000u };
			v->bits.val_f = u.f;
			break;
		}
		case 8:
		{
			const union
			{
				unsigned long l;
				double d;
			} u = { 0x7ff8000000000000u };
			v->bits.val_f = u.d;
			break;
		}
		default:
			ICE("TODO: long double nan");
	}

	v->type = V_CONST_F;
	/*impl_load_fp(v);*/
}
