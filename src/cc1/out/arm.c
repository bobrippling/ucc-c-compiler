#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include "../../util/util.h"
#include "../../util/dynarray.h"
#include "../../util/platform.h"
#include "../../util/str.h"
#include "../../util/macros.h"
#include "../../util/alloc.h"

#include "../type.h"
#include "../type_nav.h"
#include "../type_is.h"
#include "../op.h"
#include "../num.h"
#include "../funcargs.h"

#include "forwards.h"
#include "val.h"
#include "asm.h"
#include "impl.h"
#include "impl_jmp.h"
#include "impl_fp.h"
#include "virt.h"
#include "write.h"
#include "out.h"
#include "blk.h"
#include "func.h"

#include "arm.h"

#define N_CALL_REGS_I 4

#define UNUSED_ARG(a) (void)a

const struct asm_type_table asm_type_table[ASM_TABLE_LEN] = {
	{ 1, "byte" },
	{ 2, "word" },
	{ 4, "long" },
};

static int is_armv7_or_above(void)
{
	switch(platform_subarch()){
		case SUBARCH_none:
		case SUBARCH_armv6:
			break;
		case SUBARCH_armv7:
			return 1;
	}
	return 0;
}

static int is_8bit_rotated_or_zero(unsigned x)
{
	/* see if one byte is set, and no others */
	unsigned i;

	if(!x)
		return 1;

	for(i = 0; i < sizeof(x); i++){
		integral_t mask = 0xff << (i * 8);
		if((x & mask) && (x & ~mask) == 0)
			return 1;
	}

	if(x & 0xf000000f && (x & 0x0ffffff0) == 0) /* split byte */
		return 1;

	return 0;
}

static const char *arm_reg_to_str(int i)
{
	static const char *rnames[] = {
		"r0",
		"r1",
		"r2",
		"r3",
		"r4",
		"r5",
		"r6",
		"r7",
		"r8",
		"r9",
		"r10",
		"fp",
		"ip",
		"sp",
		"lr",
		"pc",
	};

	UCC_ASSERT(0 <= i && i < (int)(sizeof(rnames)/sizeof(rnames[0])),
			"reg idx oob %d", i);

	return rnames[i];
}

static const char *arm_cmp(const struct flag_opts *flag, int flip)
{
	enum flag_cmp cmp = flag->cmp;
	if(flip)
		cmp = v_not_cmp(cmp);

	switch(cmp){
#define OP(e, s, u)  \
		case flag_ ## e: \
			return flag->mods & flag_mod_signed ? s : u

		OP(eq, "eq", "eq");
		OP(ne, "ne", "ne");
		OP(le, "le", "ls"); /* signed-less-equal, unsigned-lower-same */
		OP(lt, "lt", "lo");
		OP(ge, "ge", "hs");
		OP(gt, "gt", "hi");

		OP(overflow, "vs", "cs");
		OP(no_overflow, "vc", "cc");

		OP(signbit, "mi", NULL);
		OP(no_signbit, "pl", NULL);
#undef OP
	}

	assert(0 && "unreachable");
	return NULL;
}

const char *impl_val_str_r(char buf[VAL_STR_SZ], const out_val *vs, int deref)
{
	/* just for debugging */
	UNUSED_ARG(deref);

	switch(vs->type){
		case V_CONST_I:
			xsnprintf(buf, VAL_STR_SZ, "#%" NUMERIC_FMT_D, vs->bits.val_i);
			break;
		case V_CONST_F:
			xsnprintf(buf, VAL_STR_SZ, "#%" NUMERIC_FMT_LD, vs->bits.val_f);
			break;
		case V_REG:
		case V_REGOFF:
		case V_SPILT:
			if(vs->bits.regoff.offset)
				xsnprintf(buf, VAL_STR_SZ, "%s+%ld", arm_reg_to_str(vs->bits.regoff.reg.idx), vs->bits.regoff.offset);
			else
				xsnprintf(buf, VAL_STR_SZ, "%s", arm_reg_to_str(vs->bits.regoff.reg.idx));
			break;
		case V_LBL:
			if(vs->bits.lbl.offset)
				xsnprintf(buf, VAL_STR_SZ, "%s+%ld", vs->bits.lbl.str, vs->bits.lbl.offset);
			else
				xsnprintf(buf, VAL_STR_SZ, "%s", vs->bits.lbl.str);
			break;
		case V_FLAG:
			xsnprintf(buf, VAL_STR_SZ, "%s", flag_cmp_to_str(vs->bits.flag.cmp));
			break;
	}

	return buf;
}

int impl_reg_frame_const(const struct vreg *r, int sp)
{
	if(r->is_float)
		return 0;

	switch(r->idx){
		case REG_BP:
			return 1;
		case REG_SP:
			return sp;
	}
	return 0;
}

int impl_reg_savable(const struct vreg *r)
{
	if(r->is_float)
		return 1;
	switch(r->idx){
		case REG_BP:
		case REG_SP:
			return 0;
	}
	return 1;
}

static void arm_jmp(out_ctx *octx, const out_val *target, const char *pre)
{
	switch(target->type){
		case V_CONST_F:
			ICE("call float");

		case V_FLAG:
		case V_SPILT:
		case V_REGOFF:
		case V_CONST_I:
			target = v_to_reg(octx, target);
		case V_REG:
			out_asm(octx, "%sx %s", pre, arm_reg_to_str(target->bits.regoff.reg.idx));
			break;

		case V_LBL:
			out_asm(octx, "%s %s", pre, target->bits.lbl.str);
	}
}

void impl_branch(
		out_ctx *octx, const out_val *cond,
		out_blk *bt, out_blk *bf,
		int unlikely)
{
	char *branch;

	for(;;){
		switch(cond->type){
			case V_FLAG:
				branch = ustrprintf(
					"b%s %s",
					arm_cmp(&cond->bits.flag, 0),
					bt->lbl);

				out_val_consume(octx, cond);
				break;

			case V_REG:
			{
				const out_val *flag = out_op(
						octx, op_ne, cond, out_new_l(octx, cond->t, 0));

				assert(flag->type == V_FLAG);
				cond = flag;
				continue;
			}

			default:
				ucc_unreach();
		}
		break;
	}

	blk_terminate_condjmp(octx, branch, bt, bf, unlikely);
}

void impl_jmp_expr(out_ctx *octx, const out_val *v)
{
	v = v_reg_apply_offset(octx, v_to_reg(octx, v));

	out_asm(octx, "bx %s", arm_reg_to_str(v->bits.regoff.reg.idx));
	out_val_consume(octx, v);
}

void impl_jmp(const char *lbl, const struct section *sec)
{
	asm_out_section(sec, "\tb %s\n", lbl);
}

void impl_func_call(
		out_ctx *octx, type *fnty, int nfloats,
		const out_val *fn, const out_val **args)
{
	UNUSED_ARG(fnty);
	UNUSED_ARG(nfloats);
	UNUSED_ARG(args);

	arm_jmp(octx, fn, "bl");
}

void impl_func_alignstack(out_ctx *octx)
{
	v_stack_needalign(octx, 8);
}

void impl_func_call_regs(type *retty, unsigned *pn, const struct vreg **regs)
{
	static const struct vreg callregs[] = {
		{ ARM_REG_R0, 0 },
		{ ARM_REG_R1, 0 },
		{ ARM_REG_R2, 0 },
		{ ARM_REG_R3, 0 }
	};

	UNUSED_ARG(retty);

	*pn = 4;
	*regs = callregs;
}

enum stret impl_func_stret(type *ty)
{
	struct_union_enum_st *su = type_is_s_or_u(ty);
	unsigned sz;

	if(!su)
		return stret_scalar;

	sz = type_size(ty, NULL);
	if(sz <= type_primitive_size(type_int))
		return stret_regs;
	return stret_memcpy;
}

int impl_func_caller_cleanup(type *ty)
{
	UNUSED_ARG(ty);
	return 1;
}

void impl_func_overlay_regpair(
		struct vreg regpair[/*2*/], int *const nregs, type *retty)
{
	(void)retty;

	*nregs = 1;
	regpair[0].idx = ARM_REG_R0;
	regpair[0].is_float = 0;
}

const out_val *impl_cast_extend(out_ctx *octx, const out_val *val, type *big_to)
{
	if(val->type != V_REG)
		val = v_to_reg(octx, val);

	if(type_size(val->t, NULL) < type_primitive_size(type_int))
		ICW("TODO: extend from %s", type_to_str(val->t));

	/* fully extended */
	return out_change_type(octx, val, big_to);
}

enum LDR_STR {
	LDR,
	STR
};

static void arm_ldr_str(
	out_ctx *octx,
	enum LDR_STR isn_enum,
	type *ty,
	int reg,
	const struct vreg_off *regoff)
{
	const char *isn = isn_enum == LDR ? "ldr" : "str";
	const int is_signed = type_is_signed(ty);
	const char *width;

	switch(type_size(ty, NULL)){
		case 1:
			width = isn_enum == LDR && is_signed ? "sb" : "b";
			break;
		case 2:
			width = isn_enum == LDR && is_signed ? "sh" : "h";
			break;
		case 4:
			width = "";
			break;
		default:
			ICE("invalid load size");
	}

	if(regoff->offset){
		out_asm(octx, "%s%s %s, [%s, #%ld]",
				isn,
				width,
				arm_reg_to_str(reg),
				arm_reg_to_str(regoff->reg.idx),
				regoff->offset);
	}else{
		out_asm(octx, "%s%s %s, [%s]",
				isn,
				width,
				arm_reg_to_str(reg),
				arm_reg_to_str(regoff->reg.idx));
	}
}

ucc_wur const out_val *impl_deref(
		out_ctx *octx, const out_val *vp,
		const struct vreg *reg,
		int *done_out_deref)
{
	// TODO: merge with x86
	type *tpointed_to = vp->type == V_SPILT
		? vp->t
		: type_dereference_decay(vp->t);

	vp = v_to_reg(octx, vp);
	arm_ldr_str(
			octx,
			LDR,
			type_is_ptr(vp->t),
			reg->idx,
			&vp->bits.regoff);

	if(done_out_deref)
		*done_out_deref = 0;

	return v_new_reg(octx, vp, tpointed_to, reg);
}

const out_val *impl_i2f(out_ctx *octx, const out_val *vp, type *t_f)
{
	UNUSED_ARG(octx);
	UNUSED_ARG(vp);
	UNUSED_ARG(t_f);
	ICW("TODO");
	return NULL;
}

const out_val *impl_f2i(out_ctx *octx, const out_val *vp, type *t_i)
{
	UNUSED_ARG(octx);
	UNUSED_ARG(vp);
	UNUSED_ARG(t_i);
	ICW("TODO");
	return NULL;
}

const out_val *impl_f2f(out_ctx *octx, const out_val *vp, type *to)
{
	UNUSED_ARG(octx);
	UNUSED_ARG(vp);
	UNUSED_ARG(to);
	ICW("TODO");
	return NULL;
}

void impl_func_prologue_save_fp(out_ctx *octx)
{
	out_asm(octx, "push { fp, lr }");
	out_asm(octx, "mov fp, sp");
}

void impl_func_prologue_save_call_regs(
		out_ctx *octx,
		type *fnty, unsigned nargs,
		const out_val *arg_offsets[/*nargs*/])
{
	funcargs *fa = type_funcargs(fnty);
	unsigned i;
	unsigned ws = platform_word_size();

	for(i = 0; i < nargs; i++){
		type *ty = fa->arglist[i]->ref;

		assert(!type_is_floating(ty));
		assert(!type_is_s_or_u(ty));

		if(i < N_CALL_REGS_I){
			/* numerical order, lowest regno at lowest address */
			/*
			 * 3-4 args:
			 *   arg 3 => fp - 4
			 *   arg 2 => fp - 8
			 *   arg 1 => fp - 12
			 *   arg 0 => fp - 16
			 * 1-2 args:
			 *   arg 1 => fp - 4
			 *   arg 0 => fp - 8
			 */
			int const bottom = nargs <= 2 ? 2 : 4;

			arg_offsets[i] = v_new_bp3_below(
					octx, NULL, type_ptr_to(ty), (bottom - i) * ws);
		}else{
			/* +2 to skip over saved fp & lr */
			arg_offsets[i] = v_new_bp3_above(
					octx, NULL, type_ptr_to(ty), (i - 4 + 2) * ws);
		}
	}

	if(nargs){
		/* maintain stack alignment with even number of pushes */
		int lastreg = MIN(nargs - 1, 3);
		if((lastreg & 1) == 0)
			lastreg++;

		out_asm(octx, "push { r0-%s }", arm_reg_to_str(ARM_REG_R0 + lastreg));
	}
}

const struct vreg *impl_callee_save_regs(type *fnty, unsigned *pn)
{
	static const struct vreg saves[] = {
		{ ARM_REG_R4, 0 },
		{ ARM_REG_R5, 0 },
		{ ARM_REG_R6, 0 },
		{ ARM_REG_R7, 0 },
		{ ARM_REG_R8, 0 },
	};
	UNUSED_ARG(fnty);
	*pn = countof(saves);
	return saves;
}

void impl_func_prologue_save_variadic(out_ctx *octx, type *rf)
{
	UNUSED_ARG(octx);
	UNUSED_ARG(rf);
	ICW("TODO");
}

void impl_func_epilogue(out_ctx *octx, type *ty, int clean_stack)
{
	UNUSED_ARG(ty);
	if(clean_stack){
		out_asm(octx, "mov sp, fp");
		out_asm(octx, "pop { fp, pc }");
	}else{
		out_asm(octx, "bx lr");
	}
}

ucc_nonnull((1, 2, 5, 6))
static void arm_op(
	out_ctx *octx,
	const char *opc,
	const char *opc_neg, /* nullable */
	const struct vreg *result_reg, /* null means two-args, e.g. cmp */
	const out_val *l,
	const out_val *r)
{
	assert(l->type == V_REG);

	if(r->type == V_CONST_I){
		const integral_t val = r->bits.val_i;

		if(val < ((integral_t)1 << 32)){
			if(is_8bit_rotated_or_zero((unsigned)val)){
				if(result_reg)
					out_asm(
							octx,
							"%s %s, %s, #%" NUMERIC_FMT_D,
							opc,
							arm_reg_to_str(result_reg->idx),
							arm_reg_to_str(l->bits.regoff.reg.idx),
							val);
				else
					out_asm(
							octx,
							"%s %s, #%" NUMERIC_FMT_D,
							opc,
							arm_reg_to_str(l->bits.regoff.reg.idx),
							val);
				return;
			}
		}else if(opc_neg && (sintegral_t)val < 0){
			integral_t flipped = ~val;

			if(is_8bit_rotated_or_zero((unsigned)flipped)){
				if(result_reg)
					out_asm(
							octx,
							"%s %s, %s, #%" NUMERIC_FMT_D,
							opc_neg,
							arm_reg_to_str(result_reg->idx),
							arm_reg_to_str(l->bits.regoff.reg.idx),
							val);
				else
					out_asm(
							octx,
							"%s %s, #%" NUMERIC_FMT_D,
							opc_neg,
							arm_reg_to_str(l->bits.regoff.reg.idx),
							val);
				return;
			}
		}
	}

	r = v_to_reg(octx, r);

	if(result_reg)
		out_asm(
				octx,
				"%s %s, %s, %s",
				opc,
				arm_reg_to_str(result_reg->idx),
				arm_reg_to_str(l->bits.regoff.reg.idx),
				arm_reg_to_str(r->bits.regoff.reg.idx));
	else
		out_asm(
				octx,
				"%s %s, %s",
				opc,
				arm_reg_to_str(l->bits.regoff.reg.idx),
				arm_reg_to_str(r->bits.regoff.reg.idx));
}

const out_val *impl_load(
		out_ctx *octx, const out_val *from,
		const struct vreg *reg)
{
	switch(from->type){
		case V_CONST_I:
		{
			/*
			 * if we have an 8-bit value (rotated an even number of bits, 0 <= b <= 30:
			 * 	 mov reg, #constant
			 * if we have armv7:
			 *   movw reg, #constant & 0xffff
			 *   movt reg, #constant >> 16
			 * else:
			 *   ldr reg, =constant
			 */
			const integral_t val = from->bits.val_i;
			const char *regstr = arm_reg_to_str(reg->idx);
			int done = 0;

			if(val < ((integral_t)1 << 32)){
				if(is_8bit_rotated_or_zero((unsigned)val)){
					done = 1;
					out_asm(octx, "mov %s, #%" NUMERIC_FMT_D, regstr, val);

				}else if(is_armv7_or_above()){
					done = 1;

					out_asm(octx, "movw %s, #%d",
							arm_reg_to_str(reg->idx),
							(int)(val & 0x0000ffff));

					if(from->bits.val_i & 0xffff0000){
						out_asm(octx, "movt %s, #%d",
								arm_reg_to_str(reg->idx),
								(int)((val & 0xffff0000) >> 16));
					}
				}
			}else if((sintegral_t)val < 0){
				integral_t flipped = ~val;

				if(is_8bit_rotated_or_zero((unsigned)flipped)){
					done = 1;

					out_asm(octx, "mvn %s, #%d",
							arm_reg_to_str(reg->idx),
							(int)flipped);
				}
			}

			if(!done)
				out_asm(octx, "ldr %s, =%" NUMERIC_FMT_D, regstr, val);
			break;
		}

		case V_SPILT:
			arm_ldr_str(
					octx,
					LDR,
					type_is_ptr(from->t),
					reg->idx,
					&from->bits.regoff);
			break;

		case V_REGOFF:
			ICE("TODO: V_REGOFF");
		case V_REG:
			if(from->bits.regoff.offset > 0){
				const out_val *add = out_new_l(octx, from->t, from->bits.regoff.offset);

				arm_op(
						octx,
						"add",
						NULL,
						reg,
						from,
						add);

				out_val_consume(octx, add);
				break;
			}
			if(from->bits.regoff.offset < 0){
				const out_val *sub = out_new_l(octx, from->t, -from->bits.regoff.offset);

				arm_op(
						octx,
						"sub",
						NULL,
						reg,
						from,
						sub);

				out_val_consume(octx, sub);
				break;
			}

			if(reg->idx == from->bits.regoff.reg.idx)
				break;

			out_asm(octx, "mov %s, %s",
					arm_reg_to_str(reg->idx),
					arm_reg_to_str(from->bits.regoff.reg.idx));
			break;

		case V_LBL:
			out_asm(octx, "ldr %s, =%s",
					arm_reg_to_str(reg->idx),
					from->bits.lbl.str);
			break;

		case V_CONST_F:
			ICE("TODO");
			break;

		case V_FLAG:
		{
			out_asm(
				octx,
				"mov%s %s, #1",
				arm_cmp(&from->bits.flag, 0),
				arm_reg_to_str(reg->idx));

			out_asm(
				octx,
				"mov%s %s, #0",
				arm_cmp(&from->bits.flag, 1),
				arm_reg_to_str(reg->idx));
			break;
		}
	}

	return v_new_reg(octx, from, from->t, reg);
}

void impl_store(out_ctx *octx, const out_val *dest, const out_val *val)
{
	const struct vreg *reg;

	val = v_to(octx, val, TO_REG);
	reg = &val->bits.regoff.reg;

	switch(dest->type){
		case V_CONST_I:
			ICE("store to int - needs moving to reg");
			out_asm(octx, "mov r0, #0 ; str %s, [r0, #%d] @ ... for e.g.",
					arm_reg_to_str(reg->idx),
					(int)val->bits.val_i);
			break;

		case V_SPILT:
			arm_ldr_str(octx, STR, val->t, reg->idx, &dest->bits.regoff);
			break;
		case V_REGOFF:
			ICE("TODO: V_REGOFF");
		case V_REG:
			arm_ldr_str(octx, STR, val->t, reg->idx, &dest->bits.regoff);
			break;

		case V_LBL:
			out_asm(octx, "str %s, =%s",
					arm_reg_to_str(reg->idx),
					dest->bits.lbl.str);
			break;

		case V_CONST_F:
			ICE("TODO");
			break;

		case V_FLAG:
			ICW("TODO: setCOND");
	}

	out_val_consume(octx, dest);
	out_val_consume(octx, val);
}

const out_val *impl_op(out_ctx *octx, enum op_type op, const out_val *l, const out_val *r)
{
	const char *opc, *opc_neg = NULL;

	switch(op){
		case op_multiply: opc = "mul"; goto op;
		case op_minus: opc = "sub"; goto op;
		case op_divide: opc = "div"; goto op;
		case op_modulus: opc = "mod"; goto op;
		case op_plus: opc = "add"; goto op;
		case op_xor: opc = "or"; goto op;
		case op_or: opc = "or"; goto op;
		case op_and: opc = "and"; goto op;

		case op_eq:
		case op_ne:
		case op_le:
		case op_lt:
		case op_ge:
		case op_gt:
			opc = "cmp";
			opc_neg = "cmn";
			goto cond;
op:
		{
			/*
			 * rhs can be a byte-constant or register, either w/shift
			 * if lhs is a 1-retained register, we reuse it, otherwise we pick another
			 */
			struct vreg result_reg;

			if(l->type == V_REG && impl_reg_frame_const(&l->bits.regoff.reg, 0)){
				/* can't reuse fp/sp */
				v_unused_reg(octx, 1, 0, &result_reg, NULL);
			}else{
				l = v_to_reg(octx, l);
				v_unused_reg(octx, 1, 0, &result_reg, l);
			}

			// TODO: swap opc_neg/NULL for:
			// and --> bics
			// add <--> sub
			// etc
			arm_op(octx, opc, opc_neg, &result_reg, l, r);

			/* return 'l' since we use it's register as the result */
			out_val_consume(octx, r);
			return v_new_reg(octx, l, l->t, &result_reg);
		}

cond:
		{
			const int is_signed = type_is_signed(l->t);
			enum flag_cmp cmp = op_to_flag(op);

			/* if we have a const and reg in the wrong order... */
			if(l->type == V_CONST_I && r->type != V_CONST_I){
				const out_val *tmp = l;
				l = r, r = tmp;
				cmp = v_commute_cmp(cmp);
			}

			l = v_to_reg(octx, l);

			arm_op(octx, opc, opc_neg, NULL, l, r);

			return v_new_flag(
					octx, impl_min_retained(octx, l, r),
					cmp, is_signed ? flag_mod_signed : 0);
		}

		case op_orsc:
		case op_andsc:
			ICE("sc op");
			break;

		case op_shiftl:
		case op_shiftr:
			ICW("TODO: shift");
			break;

		case op_not:
		case op_bnot:
			ICW("TODO: not");
			break;

		case op_signbit:
		case op_no_signbit:
			ICE("TODO: op_signbit");
			ICE("TODO: op_no_signbit");

		case op_unknown:
			ICE("unknown op");
	}

	return l;
}

const out_val *impl_op_unary(out_ctx *octx, enum op_type op, const out_val *val)
{
	switch(op){
		default:
			ICE("unary %s", op_to_str(op));

		case op_plus:
			/* noop */
			return val;

		case op_minus:
			/* TODO: factor out */
			return out_op(octx, op_minus, out_new_zero(octx, val->t), val);

		case op_bnot:
			UCC_ASSERT(!type_is_floating(val->t), "~float");

			val = v_to_reg(octx, val);

			out_asm(octx, "mvn %s, %s",
					arm_reg_to_str(val->bits.regoff.reg.idx),
					arm_reg_to_str(val->bits.regoff.reg.idx));

			return v_dup_or_reuse(octx, val, val->t);

		case op_not:
			return out_op(octx, op_eq, out_new_zero(octx, val->t), val);
	}
}

void impl_to_retreg(out_ctx *octx, const out_val *val, type *retty)
{
	struct vreg reg = VREG_INIT(REG_RET_I, 0);

	UCC_ASSERT(!type_is_floating(retty), "TODO: float return");

	out_val_consume(octx, v_to_reg_given(octx, val, &reg));
}

void impl_reg_cp_no_off(
		out_ctx *octx, const out_val *from, const struct vreg *to_reg)
{
	out_asm(octx, "mov %s, %s",
			arm_reg_to_str(to_reg->idx),
			arm_reg_to_str(from->bits.regoff.reg.idx));
}

int impl_reg_is_scratch(type *fnty, const struct vreg *reg)
{
	UNUSED_ARG(fnty);
	UCC_ASSERT(!reg->is_float, "TODO");
	return (/*ARM_REG_R0 <= reg->idx &&*/ reg->idx < ARM_REG_R3)
		|| ARM_REG_R12 == reg->idx;
}

ucc_static_assert(maths, ARM_REG_R0 == 0);
int impl_reg_to_scratch(const struct vreg *reg)
{
	if(/*ARM_REG_R0 <= reg->idx &&*/ reg->idx <= ARM_REG_R8)
		return reg->idx;

	UCC_ASSERT(reg->idx == ARM_REG_R12, "bad scratch reg");
	return ARM_REG_R8 + 1;
}

void impl_scratch_to_reg(int idx, struct vreg *reg)
{
	if(idx == ARM_REG_R8 + 1)
		reg->idx = ARM_REG_R12;
	else if(idx > ARM_REG_R8)
		ICE("bad arm idx reg %d", idx);
	else
		reg->idx = idx;
}

void impl_set_nan(out_ctx *octx, out_val *val)
{
	UNUSED_ARG(octx);
	UNUSED_ARG(val);
	ICE("TODO");
}

const out_val *impl_test_overflow(
		out_ctx *octx, const out_val **val)
{
	UNUSED_ARG(octx);
	UNUSED_ARG(val);
	ICE("TODO");
}

void impl_undefined(out_ctx *octx)
{
	UNUSED_ARG(octx);
	ICE("TODO");
}

void impl_debugtrap(out_ctx *octx)
{
	UNUSED_ARG(octx);
	ICE("TODO");
}

void impl_fp_bits(char *buf, size_t bufsize, enum type_primitive prim, floating_t fp)
{
	UNUSED_ARG(buf);
	UNUSED_ARG(bufsize);
	UNUSED_ARG(prim);
	UNUSED_ARG(fp);
	ICE("TODO");
}

void impl_reserve_retregs(out_ctx *octx)
{
	UNUSED_ARG(octx);
	ICW("TODO");
}

void impl_unreserve_retregs(out_ctx *octx)
{
	UNUSED_ARG(octx);
	ICW("TODO");
}
