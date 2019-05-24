#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include "../../util/util.h"
#include "../../util/alloc.h"
#include "../../util/dynarray.h"
#include "../../util/platform.h"
#include "../../util/macros.h"
#include "../../util/str.h"

#include "../op.h"
#include "../decl.h"
#include "../type.h"
#include "../type_is.h"
#include "../type_nav.h"
#include "../funcargs.h"
#include "../defs.h"
#include "../pack.h"

#include "../fopt.h"
#include "../cc1.h"
#include "../cc1_target.h"
#include "../cc1_out.h"

#include "val.h"
#include "asm.h"
#include "impl.h"
#include "impl_jmp.h"
#include "impl_fp.h"
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

#define NUM_FMT "%lld"
/* format for movl $5, -0x6(%rbp) asm output
                        ^~~                    */

#define REG_STR_SZ 8

static const out_val *pointer_to_GOT(out_ctx *, const out_val *, const struct vreg *, int *hasoffset);

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
	unsigned n_call_regs_int;
	struct vreg call_regs[6 + 8];

	unsigned n_callee_save_regs;
	int callee_save_regs[6];
} calling_convs[] = {
	[conv_x64_sysv] = {
		1,
		6 + 8,
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

	[conv_stdcall]  = { 0 },

	[conv_fastcall] = {
		0,
		2,
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

		{  "bpl", "bp", "ebp", "rbp" },
		{  "spl", "sp", "esp", "rsp" },

		/* r[8 - 15] -> r8b, r8w, r8d,  r8 */
#define REG(x) {  "r" #x "b",  "r" #x "w", "r" #x "d", "r" #x  }
		REG(8),  REG(9),  REG(10), REG(11),
		REG(12), REG(13), REG(14), REG(15),
#undef REG
	};

	UCC_ASSERT(reg < countof(rnames), "invalid x86 int reg %d", reg);

	return rnames[reg][asm_table_lookup(r)];
}

static const char *x86_fpreg_str(unsigned i)
{
	static const char *nams[] = {
		"xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"
	};

	UCC_ASSERT(i < countof(nams), "bad fp reg index %d", i);

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

	switch(type_size(ty, NULL)){
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
		struct vreg regs[], enum regtype chosentype, int *regpair_idx)
{
	switch(chosentype){
		case NONE:
		case INT:
			regs[*regpair_idx].is_float = 0;

			regs[*regpair_idx].idx =
				(*regpair_idx == 0 || regs[0].is_float)
				? REG_RET_I_1
				: REG_RET_I_2;
			break;

		case FLOAT:
			assert(!platform_32bit() && "TODO: 32-bit float return");
			regs[*regpair_idx].is_float = 1;

			regs[*regpair_idx].idx =
				(*regpair_idx == 0 || !regs[0].is_float)
				? REG_RET_F_1
				: REG_RET_F_2;
			break;
	}

	++*regpair_idx;
}

static void x86_overlay_regpair(
		struct vreg regpair[/*2*/], int *const nregs, type *retty)
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

	*nregs = 0;

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
					regpair,
					current_type,
					&regpair_idx);

			++*nregs;

			current_type = NONE;
			current_size_bits = 0;
		}
	}

	if(regpair_idx < 2){
		x86_overlay_regpair_1(
				regpair,
				current_type,
				&regpair_idx);

		++*nregs;
	}
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

const char *impl_val_str_r(
		char buf[VAL_STR_SZ], const out_val *vs, const int deref)
{
	switch(vs->type){
		case V_CONST_I:
		{
			char *p = buf;
			/* we should never get a 64-bit value here
			 * since movabsq should load those in
			 */
			UCC_ASSERT(integral_high_bit(vs->bits.val_i, vs->t) < AS_MAX_MOV_BIT,
					"can't load 64-bit constants here (0x%llx)", vs->bits.val_i);

			if(deref == 0)
				*p++ = '$';

			integral_str(p, VAL_STR_SZ - (deref == 0 ? 1 : 0),
					vs->bits.val_i, vs->t);
			break;
		}

		case V_CONST_F:
			ICE("can't stringify float here");

		case V_FLAG:
			ICE("%s shouldn't be called with cmp-flag data", __func__);

		case V_LBL:
		{
			const char *pre = deref ? "" : "$";
			const char *picstr = "";

			if(deref && (vs->bits.lbl.pic_type & OUT_LBL_PIC)){
				int local_sym = vs->bits.lbl.pic_type & OUT_LBL_PICLOCAL;

				/* if it's local, we can access the symbol at a fixed offset.
				 * otherwise it's in another module, so we need the GOT to access it
				 */
				picstr = local_sym ? "(%rip)" : "@GOTPCREL(%rip)";
			}

			if(vs->bits.lbl.offset){
				xsnprintf(buf, VAL_STR_SZ, "%s%s+%ld%s",
						pre,
						vs->bits.lbl.str,
						vs->bits.lbl.offset,
						picstr);
			}else{
				xsnprintf(buf, VAL_STR_SZ, "%s%s%s",
						pre, vs->bits.lbl.str, picstr);
			}
			break;
		}

		case V_REG:
		case V_REGOFF:
		case V_SPILT:
		{
			long off = vs->bits.regoff.offset;
			const char *rstr = x86_reg_str(
					&vs->bits.regoff.reg, deref ? NULL : vs->t);

			UCC_ASSERT(!deref || !vs->bits.regoff.reg.is_float,
					"dereference float reg");

			if(off){
				UCC_ASSERT(1||deref,
						"can't add to a register in %s",
						__func__);

				xsnprintf(buf, VAL_STR_SZ,
						"%s" NUM_FMT "(%%%s)",
						off < 0 ? "-" : "",
						llabs(off),
						rstr);
			}else{
				xsnprintf(buf, VAL_STR_SZ,
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

int impl_reg_to_idx(const struct vreg *r)
{
	if(r->is_float){
		int nints;
		impl_regs(NULL, &nints, NULL);
		return r->idx + nints;
	}else{
		return r->idx;
	}
}

void impl_scratch_to_reg(int scratch, struct vreg *r)
{
	if(r->is_float){
		int nints;
		impl_regs(NULL, &nints, NULL);
		r->idx = scratch - nints;
	}else{
		r->idx = scratch;
	}
}

void impl_regs(
		const struct vreg **const out_regs,
		int *const nints,
		int *const nfloats)
{
	if(platform_32bit()){
		static const struct vreg regs[] = {
			{ X86_64_REG_RAX, 0 },
			{ X86_64_REG_RBX, 0 },
			{ X86_64_REG_RCX, 0 },
			{ X86_64_REG_RDX, 0 },

			{ X86_64_REG_RDI, 0 },
			{ X86_64_REG_RSI, 0 },

			{ X86_64_REG_RBP, 0 },
			{ X86_64_REG_RSP, 0 },
		};

		if(out_regs)
			*out_regs = regs;
		if(nints)
			*nints = countof(regs);
		if(nfloats)
			*nfloats = 0;
	}else{
		static const struct vreg regs[] = {
			{ X86_64_REG_RAX, 0 },
			{ X86_64_REG_RBX, 0 },
			{ X86_64_REG_RCX, 0 },
			{ X86_64_REG_RDX, 0 },

			{ X86_64_REG_RDI, 0 },
			{ X86_64_REG_RSI, 0 },

			{ X86_64_REG_R8 , 0 },
			{ X86_64_REG_R9 , 0 },
			{ X86_64_REG_R10, 0 },
			{ X86_64_REG_R11, 0 },
			{ X86_64_REG_R12, 0 },
			{ X86_64_REG_R13, 0 },
			{ X86_64_REG_R14, 0 },
			{ X86_64_REG_R15, 0 },

			{ X86_64_REG_RBP, 0 },
			{ X86_64_REG_RSP, 0 },

			{ X86_64_REG_XMM0, 1 },
			{ X86_64_REG_XMM1, 1 },
			{ X86_64_REG_XMM2, 1 },
			{ X86_64_REG_XMM3, 1 },
			{ X86_64_REG_XMM4, 1 },
			{ X86_64_REG_XMM5, 1 },
			{ X86_64_REG_XMM6, 1 },
			{ X86_64_REG_XMM7, 1 },
		};

		if(out_regs)
			*out_regs = regs;
		if(nints)
			*nints = 16;
		if(nfloats)
			*nfloats = 8;
	}
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
		type *fr, unsigned *pints, unsigned *pfloats,
		const struct vreg **par)
{
	const struct calling_conv_desc *ent = x86_conv_lookup(fr);

	*pints = ent->n_call_regs_int;
	*pfloats = ent->n_call_regs - ent->n_call_regs_int;

	if(par)
		*par = ent->call_regs;
}

static int x86_func_nargs(type *rf)
{
	return dynarray_count(type_funcargs(rf)->arglist);
}

const int *impl_callee_save_regs(type *fnty, unsigned *pn)
{
	const struct calling_conv_desc *ent = x86_conv_lookup(fnty);

	*pn = ent->n_callee_save_regs;
	return ent->callee_save_regs;
}

int impl_reg_frame_const(const struct vreg *r, int sp)
{
	if(r->is_float)
		return 0;
	switch(r->idx){
		case X86_64_REG_RBP:
			return 1;
		case X86_64_REG_RSP:
			/* TODO: this could be a frame constant if we know
			 * alloca() and vlas aren't used in this function */
			if(sp)
				return 1;
	}
	return 0;
}

int impl_reg_is_scratch(type *fnty, const struct vreg *r)
{
	return r->is_float || !impl_reg_is_callee_save(fnty, r);
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
	char reg = platform_32bit() ? 'e' : 'r';

	out_asm(octx, "push %%%cbp", reg);
	out_asm(octx, "mov %%%csp, %%%cbp", reg, reg);

	/* a v_alloc_stack_n() is done later to align,
	 * but not interfere with argument locations */
}

void impl_func_prologue_save_call_regs(
		out_ctx *octx,
		type *rf, unsigned nargs,
		const out_val *arg_vals[/*nargs*/])
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

		funcargs *fa;
		type *retty;

		unsigned n_call_i, n_call_f;
		const struct vreg *call_regs;

		retty = type_called(rf, &fa);

		x86_call_regs(rf, &n_call_i, &n_call_f, &call_regs);

		{
			/* trim by the number of args */
			unsigned fp_cnt, int_cnt;

			funcargs_ty_calc(fa, &int_cnt, &fp_cnt);

			int_cnt += is_stret;

			n_call_i = MIN(n_call_i, int_cnt);
			n_call_f = MIN(n_call_f, fp_cnt);
		}

		/* two cases
		 * - for all integral arguments, we can just push them
		 * - if we have floats, we must mov them to the stack
		 * each argument takes a full word for now - subject to change
		 * (e.g. long double, struct/union args, etc)
		 */
		{
			unsigned i_arg, i_arg_stk, i_i, i_f;
			const out_val *stack_loc;
			type *const arithty = type_nav_btype(cc1_type_nav, type_intptr_t);

			stack_loc = out_aalloc(octx, (n_call_f + n_call_i) * ws, ws, arithty);

			for(i_arg = i_i = i_f = i_arg_stk = 0;
					i_arg < nargs;
					i_arg++)
			{
				type *ty;
				const struct vreg *rp;
				const out_val **store;

				if(is_stret && i_arg == 0){
					ty = type_ptr_to(retty);
					assert(!octx->current_stret);
					store = &octx->current_stret;
				}else{
					ty = fa->arglist[i_arg - is_stret]->ref;
					store = &arg_vals[i_arg - is_stret];
				}

				if(type_is_floating(ty)){
					if(i_f >= n_call_f)
						goto pass_via_stack;

					rp = &call_regs[n_call_i + i_f++];
				}else{
					if(i_i >= n_call_i)
						goto pass_via_stack;

					rp = &call_regs[i_i++];
				}

				{
					stack_loc = out_change_type(octx, stack_loc, ty);
					out_val_retain(octx, stack_loc);

					*store = out_change_type(octx,
							v_reg_to_stack_mem(octx, rp, stack_loc),
							type_ptr_to(ty));

					stack_loc = out_op(octx, op_plus,
							out_change_type(octx, stack_loc, arithty),
							out_new_l(octx, arithty, ws));
				}

				continue;
pass_via_stack:
				*store = v_new_bp3_above(
						octx, NULL, type_ptr_to(ty), (i_arg_stk++ + 2) * ws);
			}


			/* note: this isn't broken by the stack reclaim code as
			 * we're still in the prologue */
			assert(octx->in_prologue);
			out_adealloc(octx, &stack_loc);
		}

		if(octx->current_stret && cc1_fopt.verbose_asm){
			const out_val *stret = octx->current_stret;

			out_comment(octx, "stret pointer '%s' @ %s",
					type_to_str(stret->t), out_val_str(stret, 1));
		}

	}
}

void impl_func_prologue_save_variadic(out_ctx *octx, type *rf)
{
	const unsigned pws = platform_word_size();

	unsigned n_call_regs_i, n_call_regs_f;
	const struct vreg *call_regs;

	type *const arithty = type_nav_btype(cc1_type_nav, type_intptr_t);
	type *const ty_dbl = type_nav_btype(cc1_type_nav, type_double);
	type *const ty_integral = type_nav_btype(cc1_type_nav, type_intptr_t);

	unsigned n_int_args, n_fp_args;

	const out_val *stk_spill;
	unsigned i;

	x86_call_regs(rf, &n_call_regs_i, &n_call_regs_f, &call_regs);

	funcargs_ty_calc(type_funcargs(rf), &n_int_args, &n_fp_args);

	/* space for all call regs */
	stk_spill = out_aalloc(octx,
			(n_int_args + n_fp_args * 2) * pws,
			pws, ty_integral);

	/* go backwards, as we want registers pushed in reverse
	 * so we can iterate positively.
	 *
	 * note: we don't push call regs, just variadic ones after,
	 */
	for(i = n_int_args; i < n_call_regs_i; i++){
		/* intergral call regs go _below_ floating */
		const struct vreg *vr = &call_regs[i];
		const out_val *stk_ptr;

		out_val_retain(octx, stk_spill);
		stk_ptr = out_op(octx, op_plus,
				out_change_type(octx, stk_spill, arithty),
				out_new_l(octx, ty_integral, i * pws));

		/* integral args are at the lowest address */
		out_val_release(octx, v_reg_to_stack_mem(octx, vr, stk_ptr));
	}

	{
		out_blk *va_shortcircuit_join = out_blk_new(octx, "va_shortc");
		out_blk *save_fp = out_blk_new(octx, "va_save");
		type *const ty_ch = type_nav_btype(cc1_type_nav, type_nchar);
		struct vreg eax = { 0 };
		const out_val *veax;
		const out_val *eaxcond;

		/* testb %al, %al ; jz vfin */
		eax.idx = X86_64_REG_RAX;
		veax = v_new_reg(octx, NULL, ty_ch, &eax);
		eaxcond = out_op(octx, op_eq, veax, out_new_zero(octx, ty_ch));
		out_ctrl_branch(octx, eaxcond, va_shortcircuit_join, save_fp);

		out_current_blk(octx, save_fp);
		for(i = 0; i < n_call_regs_f; i++){
			const struct vreg *vr;
			const out_val *stk_ptr;

			vr = &call_regs[n_call_regs_i + i];

			out_val_retain(octx, stk_spill);
			stk_ptr = out_op(octx, op_plus,
					out_change_type(octx, stk_spill, arithty),
					out_new_l(octx, arithty, (i * 2 + n_call_regs_i) * pws));

			stk_ptr = out_change_type(octx, stk_ptr, ty_dbl);

			/* we go above the integral regs */
			out_val_release(octx, v_reg_to_stack_mem(octx, vr, stk_ptr));
		}

		out_ctrl_transfer_make_current(octx, va_shortcircuit_join);
	}

	out_adealloc(octx, &stk_spill);
}

void impl_func_epilogue(out_ctx *octx, type *rf, int clean_stack)
{
	if(clean_stack)
		out_asm(octx, "leave");

	if(cc1_fopt.verbose_asm)
		out_comment(octx, "stack at %lu bytes", octx->cur_stack_sz);

	/* callee cleanup */
	if(!x86_caller_cleanup(rf)){
		const int nargs = x86_func_nargs(rf);

		out_asm(octx, "ret $%d", nargs * platform_word_size());
	}else{
		out_asm(octx, "ret");
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

	stret_p = octx->current_stret;

	out_val_retain(octx, stret_p);
	stret_p = out_deref(octx, stret_p);

	out_flush_volatile(octx,
			out_memcpy(octx, stret_p, from, type_size(called, NULL)));

	/* return the stret pointer argument */
	ret_reg->is_float = 0;
	ret_reg->idx = REG_RET_I_1;

	return out_deref(octx, out_val_retain(octx, octx->current_stret));
}

static void x86_func_ret_regs(
		out_ctx *octx, type *called, const out_val *from)
{
	const unsigned sz = type_size(called, NULL);
	struct vreg regs[2];
	int nregs;

	x86_overlay_regpair(regs, &nregs, called);

	/* read from the stack to registers */
	impl_overlay_mem2regs(octx, sz, nregs, regs, from);
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
			r.idx = r.is_float ? REG_RET_F_1 : REG_RET_I_1;
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

	assert(0 && "unreachable - unknown cmp");
	return NULL;
}

static const out_val *x86_load_iv(
		out_ctx *octx, const out_val *from,
		const struct vreg *reg /* may be null */)
{
	const int high_bit = integral_high_bit(from->bits.val_i, from->t);
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
				impl_val_str(from, 0),
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
			&& cc1_fopt.integral_float_load)
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
			struct section sec = SECTION_INIT(SECTION_DATA);
			struct section_output orig_section;

			memcpy_safe(&orig_section, &cc1_current_section_output);
			{
				asm_nam_begin3(&sec, lbl, type_align(from->t, NULL));
				asm_out_fp(&sec, from->t, from->bits.val_f);
			}
			memcpy_safe(&cc1_current_section_output, &orig_section);

			from = mut = v_dup_or_reuse(octx, from, from->t);

			v_unused_reg(octx,
					1, type_is_floating(from->t),
					&r,
					from);

			/* must treat this as a pointer, and dereference here,
			 * as we currently don't have V_LBL_SPILT, for e.g. */
			mut->type = V_LBL;
			mut->bits.lbl.str = lbl;
			mut->bits.lbl.pic_type = OUT_LBL_PIC | OUT_LBL_PICLOCAL;
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
	if(cc1_fopt.finite_math_only)
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

		case V_SPILT: /* actually a pointer to T, impl_deref() handles this */
			return impl_deref(octx, from, reg, NULL);

		case V_REG:
			if(from->bits.regoff.offset == 0){
				/* optimisation: */
				impl_reg_cp_no_off(octx, from, reg);
				break;
			}
			goto lea;

		case V_CONST_I:
			from = x86_load_iv(octx, from, reg);
			break;

lea:
		case V_REGOFF:
		case V_LBL:
		{
			const int fp = type_is_floating(from->t);
			type *chosen_ty = fp ? from->t : NULL;
			const int from_GOT = from->type == V_LBL
				&& (from->bits.lbl.pic_type & OUT_LBL_PIC)
				&& !(from->bits.lbl.pic_type & OUT_LBL_PICLOCAL);
			const out_val *from_new;

			if(from_GOT){
				struct vreg gotreg = *reg;
				const out_val *gotslot;
				int hasoffset;

				gotslot = pointer_to_GOT(octx, from, &gotreg, &hasoffset);

				/* optimisation for [movq lbl@GOTPCREL(%rip), %rax;] lea (%rax), %rax */
				if(!hasoffset && vreg_eq(&gotreg, reg))
					return gotslot;

				from_new = gotslot;
			}else{
				from_new = from;
			}

			/* just go with leaq for small sizes */
			out_asm(octx, "%s%s %s, %%%s",
					fp ? "mov" : "lea",
					x86_suffix(type_nav_btype(cc1_type_nav, type_intptr_t)),
					impl_val_str(from_new, 1),
					x86_reg_str(reg, from_GOT ? NULL : chosen_ty));

			return v_new_reg(octx, from_new, from_new->t, reg);
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
	char vbuf[VAL_STR_SZ];

	/* from must be either a reg, value or flag */
	if(from->type == V_FLAG
	&& to->type == V_REG)
	{
		/* the register we're storing into is an lvalue */
		struct vreg evalreg;
		type *dest_ty;

		/* setne %evalreg */
		v_unused_reg(octx, 1, 0, &evalreg, NULL);
		from = impl_load(octx, from, &evalreg);

		/* ensure we are storing the flag as an extended type */
		dest_ty = type_is_ptr(to->t);
		if(type_size(from->t, NULL) < type_size(dest_ty, NULL))
			from = v_dup_or_reuse(octx, from, dest_ty);

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

		case V_SPILT:
		{
			struct vreg reg;
			v_unused_reg(octx, 1, 0, &reg, to);
			to = impl_load(octx, to, &reg);
			break;
		}

		case V_REGOFF:
		case V_REG:
		case V_LBL:
			break;

		case V_CONST_I:
			break;
	}

	/* if storing to something through the GOT, need double-indirection */
	if(v_needs_GOT(to)){
		const out_val *gotslot = pointer_to_GOT(octx, to, NULL, NULL);
		to = gotslot;
	}

	out_asm(octx, "mov%s %s, %s",
			x86_suffix(from->t),
			impl_val_str_r(vbuf, from, 0),
			impl_val_str(to, 1));

	out_val_consume(octx, from);
	out_val_consume(octx, to);
}

static void x86_reg_cp(
		out_ctx *octx,
		const struct vreg *to,
		const struct vreg *from,
		type *typ)
{
	assert(!impl_reg_frame_const(to, /*disallow sp: alloca_pop needs this*/0));

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
		l = v_to_reg_given_freeup_no_off(octx, l, &rax);
		l = v_reg_apply_offset(octx, l);

		/* idiv takes either a reg or memory address */
		r = v_to(octx, r, TO_REG | TO_MEM);

		assert(r->type != V_REG
				|| r->bits.regoff.reg.idx != X86_64_REG_RDX);

		if(type_is_signed(r->t)){
			const char *ext;
			switch(type_size(r->t, NULL)){
				default:
					assert(0);

				/* C frontends will only use case 4 and 8
				 *
				 * type     operand     div          quot     input
				 * byte     r/m8        AL           AH       AX
				 * word     r/m16       AX           DX       DX:AX
				 * dword    r/m32       EAX          EDX      EDX:EAX
				 */
				case 1:
				case 2:
					assert(0 && "idiv with short/char?");
				case 4:
					ext = "cltd";
					break;
				case 8:
					ext = "cqto";
					break;
			}
			out_asm(octx, "%s", ext);
		}else{
			/* unsigned - don't sign extend into rdx:
			 * mov $0, %rdx */
			out_flush_volatile(
					octx, v_to_reg_given(
						octx, out_new_zero(octx, r->t), &rdx));
		}

		out_asm(octx, "idiv%s %s",
				x86_suffix(r->t),
				impl_val_str(r, 0));

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
	char bufv[VAL_STR_SZ], bufs[VAL_STR_SZ];
	type *nchar;

	/* sh[lr] [r/m], [c/r]
	 * where            ^ must be %cl
	 */
	if(r->type != V_CONST_I){
		struct vreg cl;

		cl.is_float = 0;
		cl.idx = X86_64_REG_RCX;

		r = v_to_reg_given_freeup_no_off(octx, r, &cl);
		r = v_reg_apply_offset(octx, r);
	}

	/* force %cl: */
	nchar = type_nav_btype(cc1_type_nav, type_nchar);
	if(type_cmp(r->t, nchar, 0) & ~TYPE_EQUAL_ANY)
		r = v_dup_or_reuse(octx, r, nchar); /* change type */

	l = v_to(octx, l, TO_MEM | TO_REG);

	impl_val_str_r(bufv, l, 0);
	impl_val_str_r(bufs, r, 0);

	out_asm(octx, "%s%s %s, %s",
			op == op_shiftl
				? "shl"
				: type_is_signed(l->t) ? "sar" : "shr",
			x86_suffix(l->t),
			bufs, bufv);

	out_val_consume(octx, r);
	return v_dup_or_reuse(octx, l, l->t);
}

static const out_val *min_retained(
		out_ctx *octx,
		const out_val *a, const out_val *b)
{
	if(a->retains > b->retains){
		out_val_consume(octx, a);
		return b;
	}else{
		out_val_consume(octx, b);
		return a;
	}
}

static void maybe_promote(out_ctx *octx, const out_val **pl, const out_val **pr)
{
	const out_val *l = *pl;
	const out_val *r = *pr;

	unsigned sz_l = type_size(l->t, NULL);
	unsigned sz_r = type_size(r->t, NULL);

	if(sz_l == sz_r)
		return;

	if(sz_l < sz_r)
		*pl = out_cast(octx, l, r->t, /*normalise*/0);
	else
		*pr = out_cast(octx, r, l->t, /*normalise*/0);
}

const out_val *impl_op(out_ctx *octx, enum op_type op, const out_val *l, const out_val *r)
{
	const char *opc;

	l = x86_check_ivfp(octx, l);
	r = x86_check_ivfp(octx, r);

	if(type_is_floating(l->t)){
		if(op_is_comparison(op)){
			/* ucomi%s reg_or_mem, reg */
			char b1[VAL_STR_SZ], b2[VAL_STR_SZ];

			l = v_to(octx, l, TO_REG | TO_MEM);
			r = v_to_reg(octx, r);

			out_asm(octx, "ucomi%s %s, %s",
					x86_suffix(l->t),
					impl_val_str_r(b1, r, 0),
					impl_val_str_r(b2, l, 0));

			/* not flag_mod_signed - we want seta, not setgt */
			return v_new_flag(
					octx, min_retained(octx, l, r),
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
			char b1[VAL_STR_SZ], b2[VAL_STR_SZ];

			out_asm(octx, "%s%s %s, %s",
					opc, x86_suffix(l->t),
					impl_val_str_r(b1, r, 0),
					impl_val_str_r(b2, l, 0));

			out_val_consume(octx, r);
			return v_dup_or_reuse(octx, l, l->t);
		}
	}

	switch(op){
		/* two-operand only touches mentions regs,
		 * one-operand uses edx:eax, like idiv */
		OP(multiply, "imul");

		OP(plus, "add");
		OP(minus, "sub");
		OP(xor, "xor");
		OP(or, "or");
		OP(and, "and");
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
			char buf[VAL_STR_SZ];
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
				const char *vstr = impl_val_str(vother, 0); /* reg */
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

				maybe_promote(octx, &l, &r);

				out_asm(octx, "cmp%s %s, %s",
						x86_suffix(l->t), /* pick the non-const one (for type-ing) */
						impl_val_str(r, 0),
						impl_val_str_r(buf, l, 0));
			}

			cmp = op_to_flag(op);

			if(inv){
				/* invert >, >=, < and <=, but not == and !=, aka
				 * the commutative operators.
				 *
				 * i.e. 5 == 2 is the same as 2 == 5, but
				 *      5 >= 2 is not the same as 2 >= 5
				 */
				cmp = v_commute_cmp(cmp);
			}

			return v_new_flag(
					octx, min_retained(octx, l, r),
					cmp, is_signed ? flag_mod_signed : 0);
		}

		case op_orsc:
		case op_andsc:
			ICE("%s shouldn't get here", op_to_str(op));

		default:
			ICE("invalid op %s", op_to_str(op));
	}

	{
		char buf[VAL_STR_SZ];
		type *ret_ty;

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
			{ V_SPILT, V_CONST_I },

			{ V_REG, V_LBL },
			{ V_LBL, V_REG },

			{ V_SPILT, V_REG },
			{ V_REG, V_SPILT },

			{ V_REG, V_REG },
		};
		int need_swap = 0, satisfied = 0;
		unsigned i;

#define OP_MATCH(vp, op) (   \
		vp->type == ops[i].op && \
		((vp->type != V_REG && vp->type != V_SPILT) || !vp->bits.regoff.offset))

		for(i = 0; i < countof(ops); i++){
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

#define V_IS_MEM(ty) ((ty) == V_SPILT || (ty) == V_LBL)
			if(V_IS_MEM(l->type) && V_IS_MEM(r->type))
				r = v_to_reg(octx, r);
#undef V_IS_MEM
		}

		if(FOPT_PIC(&cc1_fopt)){
			/* pic mode - can't have direct memory references in add, etc
			 * e.g. addl $a, %eax
			 *
			 * This could be fixed by emitting addl a@GOTPCREL(%rip), %eax further down
			 */
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

		/* ensure types match - may have upgraded from V_FLAG / _Bool */
		maybe_promote(octx, &l, &r);

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
							impl_val_str(l, 0));
					break;
				}
			default:
				/* NOTE: lhs and rhs are switched for AT&T syntax,
				 * we still use lhs for the v_dup_or_reuse() below */
				out_asm(octx, "%s%s %s, %s", opc,
						x86_suffix(l->t),
						impl_val_str_r(buf, r, 0),
						impl_val_str(l, 0));
		}

		ret_ty = l->t;
		out_val_release(octx, r);
		return v_dup_or_reuse(octx, l, ret_ty);
	}
}

static const out_val *pointer_to_GOT(
		out_ctx *octx,
		const out_val *vp,
		const struct vreg *maybe_reg,
		int *const hasoffset)
{
	long offset;
	out_val *gotslot;
	struct vreg gotreg;

	assert(vp->type == V_LBL);

	if(maybe_reg)
		gotreg = *maybe_reg;
	else
		v_unused_reg(octx, 1, 0, &gotreg, NULL);

	offset = vp->bits.lbl.offset;
	if(offset){
		out_val *vp_mut = v_dup_or_reuse(octx, vp, vp->t);
		vp_mut->bits.lbl.offset = 0;
		vp = vp_mut;
	}
	if(hasoffset)
		*hasoffset = offset != 0;

	out_asm(octx, "mov%s %s, %%%s",
			x86_suffix(type_nav_btype(cc1_type_nav, type_intptr_t)),
			impl_val_str(vp, 1),
			x86_reg_str(&gotreg, NULL));

	gotslot = v_new_reg(octx, vp, vp->t, &gotreg);
	if(offset){
		assert(gotslot->type == V_REG);
		gotslot->bits.regoff.offset = offset;
	}
	return gotslot;
}

const out_val *impl_deref(
		out_ctx *octx,
		const out_val *vp,
		const struct vreg *reg,
		int *const done_out_deref)
{
	type *tpointed_to = type_dereference_decay(vp->t);
	const int via_GOT = v_needs_GOT(vp);

	if(via_GOT){
		const out_val *gotslot = pointer_to_GOT(octx, vp, NULL, NULL);

		if(done_out_deref)
			*done_out_deref = 1;
		return out_deref(octx, gotslot);
	}

	out_asm(octx, "mov%s %s, %%%s",
			x86_suffix(tpointed_to),
			impl_val_str(vp, 1),
			x86_reg_str(reg, tpointed_to));

	if(done_out_deref)
		*done_out_deref = 0;

	return v_new_reg(octx, vp, tpointed_to, reg);
}

const out_val *impl_op_unary(out_ctx *octx, enum op_type op, const out_val *val)
{
	const char *opc;

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

	val = v_to(octx, val, TO_REG | TO_MEM);

	out_asm(octx, "%s%s %s", opc,
			x86_suffix(val->t),
			impl_val_str(val, 0));

	return v_dup_or_reuse(octx, val, val->t);
}

const out_val *impl_cast_load(
		out_ctx *octx, const out_val *vp,
		type *small, type *big,
		int is_signed)
{
	/* we are always up-casting here, i.e. int -> long */
	char buf_small[VAL_STR_SZ];

	UCC_ASSERT(!type_is_floating(small) && !type_is_floating(big),
			"we don't cast-load floats");

	switch(vp->type){
		case V_CONST_F:
			ICE("cast load float");

		case V_CONST_I:
		case V_LBL:
		case V_REGOFF:
		case V_SPILT: /* could do something like movslq -8(%rbp), %rax */
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

static const out_val *x86_fp_conv(
		out_ctx *octx,
		const out_val *vp,
		struct vreg *r, type *tto,
		type *int_ty,
		const char *sfrom, const char *sto)
{
	char vbuf[VAL_STR_SZ];
	int truncate = type_is_integral(tto); /* going to int? */

	switch(vp->type){
		case V_CONST_F:
			vp = x86_load_fp(octx, vp);
			break;
		case V_SPILT:
			vp = v_to_reg(octx, vp);
			break;
		case V_CONST_I:
		case V_REG:
		case V_REGOFF:
		case V_LBL:
		case V_FLAG:
			break;
	}

	out_asm(octx, "cvt%s%s2%s%s %s, %%%s",
			truncate ? "t" : "",
			sfrom, sto,
			/* if we're doing an int-float conversion,
			 * see if we need to do 64 or 32 bit
			 */
			int_ty ? type_size(int_ty, NULL) == 8 ? "q" : "l" : "",
			impl_val_str_r(vbuf, vp, vp->type == V_REGOFF),
			x86_reg_str(r, tto));

	return v_new_reg(octx, vp, tto, r);
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

	return x86_fp_conv(octx, vp, &r, tto,
			to_float ? tfrom : tto,
			to_float ? "si" : fp_s,
			to_float ? fp_s : "si");
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

	return x86_fp_conv(octx, vp, &r, to, NULL,
			x86_suffix(from),
			x86_suffix(to));
}

static char *x86_call_jmp_target(
		out_ctx *octx, const out_val **pvp,
		int prevent_rax,
		int *const use_plt, int *const is_alloc)
{
	static char buf[VAL_STR_SZ + 2];

	*use_plt = 0;
	*is_alloc = 0;

	switch((*pvp)->type){
		case V_LBL:
			assert((*pvp)->bits.lbl.offset == 0 && "non-zero label offset in call");

			if(cc1_target_details.ld_indirect_call_via_plt && v_needs_GOT(*pvp)){
				if(!cc1_fopt.plt){
					/* must load from GOT */
					*is_alloc = 1;
					return ustrprintf("*%s", impl_val_str(*pvp, 1));
				}

				*use_plt = 1;
			}

			return (char *)(*pvp)->bits.lbl.str;

		case V_CONST_F:
		case V_FLAG:
			ICE("jmp flag/float?");

		case V_CONST_I:   /* jmp *5 */
			snprintf(buf, sizeof buf, "*%s", impl_val_str((*pvp), 1));
			return buf;

		case V_SPILT: /* load, then jmp */
		case V_REGOFF: /* load, then jmp */
		case V_REG: /* jmp *%rax */
			*pvp = v_reg_apply_offset(octx, v_to_reg(octx, *pvp));

			UCC_ASSERT(!(*pvp)->bits.regoff.reg.is_float, "jmp float?");

			if(prevent_rax && (*pvp)->bits.regoff.reg.idx == X86_64_REG_RAX){
				struct vreg r;

				v_unused_reg(octx, 1, 0, &r, /*don't want rax:*/NULL);
				impl_reg_cp_no_off(octx, *pvp, &r);

				assert((*pvp)->retains == 1);
				memcpy_safe(&((out_val *)*pvp)->bits.regoff.reg, &r);
			}

			*buf = '*';
			impl_val_str_r(buf + 1, *pvp, 0);
			/* FIXME: derereference: 0 - this should check for an lvalue and if so, pass 1 */
			return buf;
	}

	ICE("invalid jmp target type 0x%x", (*pvp)->type);
	return NULL;
}

void impl_jmp(const char *lbl)
{
	asm_out_section(&section_text, "\tjmp %s\n", lbl);
}

void impl_jmp_expr(out_ctx *octx, const out_val *v)
{
	int use_plt, is_alloc;
	char *jmp = x86_call_jmp_target(octx, &v, 0, &use_plt, &is_alloc);
	assert(!use_plt && "local jumps shouldn't be PIC");
	out_asm(octx, "jmp %s", jmp);
	out_val_consume(octx, v);
	if(is_alloc)
		free(jmp);
}

void impl_branch(
		out_ctx *octx, const out_val *cond,
		out_blk *bt, out_blk *bf,
		int unlikely)
{
	int flag;

	switch(cond->type){
		case V_REG:
		{
			const char *rstr;
			char *cmp;

			cond = v_reg_apply_offset(octx, cond);
			rstr = impl_val_str(cond, 0);

			if(type_is_floating(cond->t))
				cond = out_normalise(octx, cond);
			else
				out_asm(octx, "test %s, %s", rstr, rstr);

			cmp = ustrprintf("jz %s", bf->lbl);

			blk_terminate_condjmp(octx, cmp, bf, bt, unlikely);

			out_val_consume(octx, cond);
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

			blk_terminate_condjmp(octx, cmpjmp, bt, bf, unlikely);

			out_val_consume(octx, cond);
			break;
		}

		case V_CONST_F:
			flag = !!cond->bits.val_f;
			if(0){
		case V_CONST_I:
				flag = !!cond->bits.val_i;
			}
			out_comment(octx,
					"constant jmp condition %staken",
					flag ? "" : "not ");

			out_val_consume(octx, cond);

			out_ctrl_transfer(octx, flag ? bt : bf, NULL, NULL, 0);
			break;

		case V_LBL:
		case V_REGOFF:
		case V_SPILT:
			cond = v_to_reg(octx, cond);

			UCC_ASSERT(cond->type == V_REG,
					"normalise remained as spilt/stack/mem reg");

			cond = out_normalise(octx, cond);
			impl_branch(octx, cond, bt, bf, unlikely);
			break;
	}
}

const out_val *impl_call(
		out_ctx *octx,
		const out_val *fn, const out_val **args,
		type *fnty)
{
	type *const retty = type_called(type_is_ptr_or_block(fnty), NULL);
	type *const arithty = type_nav_btype(cc1_type_nav, type_intptr_t);
	const unsigned pws = platform_word_size();
	const unsigned nargs = dynarray_count(args);
	char *const float_arg = umalloc(nargs);
	const out_val **local_args = NULL;
	const out_val *retval_stret;
	const out_val *stret_spill;

	const struct vreg *call_regs;
	unsigned n_call_regs_i, n_call_regs_f;

	struct
	{
		v_stackt bytesz;
		const out_val *vptr;
		enum
		{
			CLEANUP_NO,
			CLEANUP_RESTORE_PUSHBACK = 1 << 0,
			CLEANUP_POP = 1 << 1
		} need_cleanup;
	} arg_stack = { 0 };

	unsigned nfloats = 0, nints = 0;
	unsigned i;
	enum stret stret_kind;
	unsigned stret_stack;

	dynarray_add_array(&local_args, args);

	x86_call_regs(fnty, &n_call_regs_i, &n_call_regs_f, &call_regs);

	/* pre-scan of arguments - eliminate flags
	 * (should only be one, since we can only have one flag at a time)
	 *
	 * also count floats and ints
	 */
	for(i = 0; i < nargs; i++){
		type *argty;

		assert(local_args[i]->retains > 0);

		if(local_args[i]->type == V_FLAG)
			local_args[i] = v_to_reg(octx, local_args[i]);

		argty = local_args[i]->t;
		if(local_args[i]->type == V_SPILT)
			argty = type_dereference_decay(argty);

		float_arg[i] = type_is_floating(argty);

		if(float_arg[i])
			nfloats++;
		else
			nints++;
	}

	/* hidden stret argument */
	switch((stret_kind = x86_stret(retty, &stret_stack))){
		case stret_memcpy:
			nints++; /* only an extra pointer arg for stret_memcpy */
			/* fall */
		case stret_regs:
			stret_spill = out_aalloc(octx, stret_stack, type_align(retty, NULL), retty);
		case stret_scalar:
			break;
	}

	/* do we need to do any stacking? */
	if(nints > n_call_regs_i)
		arg_stack.bytesz += pws * (nints - n_call_regs_i);


	if(nfloats > n_call_regs_f)
		arg_stack.bytesz += pws * (nfloats - n_call_regs_f);

	/* need to save regs before pushes/call */
	v_save_regs(octx, fnty, local_args, fn);

	/* 16 byte for SSE - special case here as mstack_align may be less */
	if(!IS_32_BIT())
		v_stack_needalign(octx, 16);

	if(arg_stack.bytesz > 0){
		out_comment(octx, "stack space for %lu arguments",
				arg_stack.bytesz / pws);

		if(x86_caller_cleanup(fnty))
			arg_stack.need_cleanup = CLEANUP_NO;
		else
			arg_stack.need_cleanup |= CLEANUP_RESTORE_PUSHBACK;

		/* see comment about stack_iter below */
		if(octx->alloca_count){
			arg_stack.bytesz = pack_to_align(arg_stack.bytesz, octx->max_align);

			v_stack_adj(octx, arg_stack.bytesz, /*sub:*/1);
			arg_stack.vptr = NULL;

			arg_stack.need_cleanup |= CLEANUP_POP;

		}else{
			/* this aligns the stack-ptr and returns arg_stack padded */
			arg_stack.vptr = out_aalloc(octx, arg_stack.bytesz, pws, arithty);

			if(octx->stack_callspace < arg_stack.bytesz)
				octx->stack_callspace = arg_stack.bytesz;
		}
	}

	if(arg_stack.bytesz > 0){
		unsigned nfloats = 0, nints = 0; /* shadow */
		const out_val *stack_iter;

		/* Rather than spilling the registers based on %rbp, we spill
		 * them based as offsets from %rsp, that way they're always
		 * at the bottom of the stack, regardless of future changes
		 * to octx->cur_stack_sz
		 *
		 * VLAs (and alloca()) unfortunately break this.
		 * For this we special case and don't reuse existing stack.
		 * Instead, we allocate stack explicitly, use it for the call,
		 * then free it (done above).
		 */
		stack_iter = v_new_sp(octx, NULL);

		/* save in order */
		for(i = 0; i < nargs; i++){
			const int stack_this = float_arg[i]
				? nfloats++ >= n_call_regs_f
				: nints++ >= n_call_regs_i;

			if(stack_this){
				type *storety = type_ptr_to(local_args[i]->t);

				assert(stack_iter->retains > 0);

				stack_iter = out_change_type(octx, stack_iter, storety);
				out_val_retain(octx, stack_iter);
				local_args[i] = v_to_stack_mem(octx, local_args[i], stack_iter, V_REGOFF);

				assert(local_args[i]->retains > 0);

				stack_iter = out_op(octx, op_plus,
						out_change_type(octx, stack_iter, arithty),
						out_new_l(octx, arithty, pws));
			}
		}

		out_val_release(octx, stack_iter);
	}

	/* must be set before stret pointer argument */
	nints = nfloats = 0;

	/* setup hidden stret pointer argument */
	if(stret_kind == stret_memcpy){
		const struct vreg *stret_reg = &call_regs[nints];
		nints++;

		if(cc1_fopt.verbose_asm){
			out_comment(octx, "stret spill space '%s' @ %s, %u bytes",
					type_to_str(stret_spill->t),
					out_val_str(stret_spill, 1),
					stret_stack);
		}

		out_val_retain(octx, stret_spill);
		v_freeup_reg(octx, stret_reg);
		out_flush_volatile(octx, v_to_reg_given(octx, stret_spill, stret_reg));
	}

	for(i = 0; i < nargs; i++){
		const out_val *const vp = local_args[i];
		/* we use float_arg[i] since vp->t may now be float *,
		 * if it's been spilt */
		const int is_float = float_arg[i];
		const struct vreg *rp = NULL;

		if(is_float){
			if(nfloats < n_call_regs_f)
				rp = &call_regs[n_call_regs_i + i];
			nfloats++;
		}else{
			if(nints < n_call_regs_i)
				rp = &call_regs[nints];
			nints++;
		}

		if(rp){
			/* only bother if it's not already in the register */
			if(vp->type != V_REG || !vreg_eq(rp, &vp->bits.regoff.reg)){
				/* need to free it up, as v_to_reg_given doesn't clobber check */
				v_freeup_reg(octx, rp);

#if 0
				/* argument retainedness doesn't matter here -
				 * local arguments and autos are held onto, and
				 * inline functions hold onto their arguments one extra
				 */
				UCC_ASSERT(local_args[i]->retains == 1,
						"incorrectly retained arg %d: %d",
						i, local_args[i]->retains);
#endif


				local_args[i] = v_to_reg_given(octx, local_args[i], rp);
			}
			if(local_args[i]->type == V_REG && local_args[i]->bits.regoff.offset){
				/* need to ensure offsets are flushed */
				local_args[i] = v_reg_apply_offset(octx, local_args[i]);
			}
		}
		/* else already pushed */
	}

	{
		funcargs *args = type_funcargs(fnty);
		int need_float_count =
			args->variadic
			|| FUNCARGS_EMPTY_NOVOID(args);
		/* jtarget must be assigned before "movb $0, %al" */
		int use_plt, is_alloc;
		char *jtarget = x86_call_jmp_target(octx, &fn, need_float_count, &use_plt, &is_alloc);

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
							MIN(nfloats, n_call_regs_f)),
						&r));
		}

		out_asm(octx, "call %s%s", jtarget, use_plt ? "@PLT" : "");
		if(is_alloc)
			free(jtarget);
	}

	if(arg_stack.bytesz){
		if(arg_stack.need_cleanup & CLEANUP_RESTORE_PUSHBACK){
			/* callee cleanup - the callee will have popped
			 * args from the stack, so we need a stack alloc
			 * to restore what we expect */

			if(arg_stack.need_cleanup & CLEANUP_POP){
				/* we wanted to pop anyway - callee did it for us */
			}else{
				v_stack_adj(octx, arg_stack.bytesz, /*sub:*/1);
			}

		}else if(arg_stack.need_cleanup & CLEANUP_POP){
			/* caller cleanup - we reuse the stack for other purposes */
				v_stack_adj(octx, arg_stack.bytesz, /*sub:*/0);
		}

		if(arg_stack.vptr)
			out_adealloc(octx, &arg_stack.vptr);
	}

	for(i = 0; i < nargs; i++)
		out_val_consume(octx, local_args[i]);
	dynarray_free(const out_val **, local_args, NULL);

	if(stret_kind != stret_scalar){
		if(stret_kind == stret_regs){
			/* we behave the same as stret_memcpy(),
			 * but we must spill the regs out */
			struct vreg regpair[2];
			int nregs;

			x86_overlay_regpair(regpair, &nregs, retty);

			retval_stret = out_val_retain(octx, stret_spill);
			out_val_retain(octx, retval_stret);

			/* spill from registers to the stack */
			impl_overlay_regs2mem(octx, stret_stack, nregs, regpair, retval_stret);
		}

		assert(stret_spill);
		out_val_release(octx, stret_spill);
	}

	/* return type */
	free(float_arg);

	if(stret_kind != stret_regs){
		/* rax / xmm0, otherwise the return has
		 * been set to a local stack address */
		const int fp = type_is_floating(retty);
		struct vreg rr = VREG_INIT(fp ? REG_RET_F_1 : REG_RET_I_1, fp);

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

	/* representation can be host-side here,
	 * we swap to native machine-side if/when
	 * we save to a label */
	v->bits.val_f = NAN;

	v->type = V_CONST_F;
}

void impl_fp_bits(char *buf, size_t bufsize, enum type_primitive prim, floating_t fp)
{
	switch(prim){
		case type_float:
		{
			const float f = fp;
			assert(bufsize >= sizeof f);
			memcpy(buf, &f, sizeof f);
			break;
		}
		case type_double:
		{
			const double f = fp;
			assert(bufsize >= sizeof f);
			memcpy(buf, &f, sizeof f);
			break;
		}
		case type_ldouble:
			ICE("TODO");
		default:
			assert(0 && "unreachable");
	}
}

static void reserve_unreserve_retregs(out_ctx *octx, int reserve)
{
	static const struct vreg retregs[] = {
		{ REG_RET_I_1, 0 },
		{ REG_RET_I_2, 0 },
		{ REG_RET_F_1, 1 },
		{ REG_RET_F_2, 1 },
	};
	unsigned i;

	for(i = 0; i < countof(retregs); i++){
		const struct vreg *r = &retregs[i];
		if(reserve)
			v_reserve_reg(octx, r);
		else
			v_unreserve_reg(octx, r);
	}
}

void impl_reserve_retregs(out_ctx *octx)
{
	reserve_unreserve_retregs(octx, 1);
}

void impl_unreserve_retregs(out_ctx *octx)
{
	reserve_unreserve_retregs(octx, 0);
}
