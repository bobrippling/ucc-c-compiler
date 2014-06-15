#include <stdio.h>
#include <stdarg.h>

#include "../../util/util.h"
#include "../../util/dynarray.h"

#include "../type.h"
#include "../type_nav.h"
#include "../type_is.h"
#include "../op.h"
#include "../num.h"

#include "forwards.h"
#include "val.h"
#include "asm.h"
#include "impl.h"
#include "impl_jmp.h"
#include "virt.h"
#include "write.h"
#include "out.h"

#include "armv7.h"

#define UNUSED_ARG(a) (void)a

const struct asm_type_table asm_type_table[ASM_TABLE_LEN] = {
	{ 1, "byte" },
	{ 2, "word" },
	{ 4, "long" },
};

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
		"r11",
		"r12",
		"sp",
		"lr",
		"pc",
	};

	UCC_ASSERT(0 <= i && i < (int)(sizeof(rnames)/sizeof(rnames[0])),
			"reg idx oob %d", i);

	return rnames[i];
}

int impl_reg_frame_const(const struct vreg *r)
{
	return !r->is_float && r->idx == REG_BP;
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
		case V_REG_SPILT:
		case V_CONST_I:
			target = v_to_reg(octx, target);
		case V_REG:
			out_asm(octx, "%sx %s", pre, arm_reg_to_str(target->bits.regoff.reg.idx));
			break;

		case V_LBL:
			out_asm(octx, "%s %s", pre, target->bits.lbl.str);
	}
}

void impl_branch(out_ctx *octx, const out_val *cond, out_blk *bt, out_blk *bf)
{
	UNUSED_ARG(octx);
	UNUSED_ARG(cond);
	UNUSED_ARG(bt);
	UNUSED_ARG(bf);
	ICW("TODO");
}

void impl_jmp_expr(out_ctx *octx, const out_val *v)
{
	v = v_reg_apply_offset(octx, v_to_reg(octx, v));

	out_asm(octx, "bx %s", arm_reg_to_str(v->bits.regoff.reg.idx));
	out_val_consume(octx, v);
}

void impl_jmp(FILE *f, const char *lbl)
{
	fprintf(f, "\tb %s\n", lbl);
}

const out_val *impl_call(
		out_ctx *octx,
		const out_val *fn, const out_val **args,
		type *fnty)
{
	const int nargs = dynarray_count(args);
	int i;

	(void)fnty;

	for(i = 0; i < nargs; i++){
		struct vreg call_reg = VREG_INIT(i, 0);

		v_freeup_reg(octx, &call_reg);

		out_flush_volatile(octx,
				v_to_reg_given(octx, args[i], &call_reg));

		v_reserve_reg(octx, &call_reg);
	}

	arm_jmp(octx, fn, "bl");

	for(i = 0; i < nargs; i++){
		struct vreg call_reg = VREG_INIT(i, 0);
		v_unreserve_reg(octx, &call_reg);
	}

	{
		struct vreg retreg = VREG_INIT(REG_RET_I, 0);

		return v_new_reg(octx, fn, type_called(fn->t, NULL), &retreg);
	}
}

const out_val *impl_cast_load(
		out_ctx *octx, const out_val *vp,
		type *small, type *big,
		int is_signed)
{
	UNUSED_ARG(octx);
	UNUSED_ARG(vp);
	UNUSED_ARG(small);
	UNUSED_ARG(big);
	UNUSED_ARG(is_signed);
	out_asm(octx, "TODO: cast load");
	return vp;
}

const out_val *impl_deref(
		out_ctx *octx, const out_val *vp, const struct vreg *reg)
{
	// TODO: merge with x86
	type *tpointed_to = vp->type == V_REG_SPILT
		? vp->t
		: type_dereference_decay(vp->t);

	vp = v_to_reg(octx, vp);
	out_asm(octx, "mov %s, [%s]",
			arm_reg_to_str(reg->idx),
			arm_reg_to_str(vp->bits.regoff.reg.idx));

	return v_new_reg(octx, vp, tpointed_to, reg);
}

const out_val *impl_i2f(out_ctx *octx, const out_val *vp, type *t_i, type *t_f)
{
	UNUSED_ARG(octx);
	UNUSED_ARG(vp);
	UNUSED_ARG(t_i);
	UNUSED_ARG(t_f);
	ICW("TODO");
	return NULL;
}

const out_val *impl_f2i(out_ctx *octx, const out_val *vp, type *t_f, type *t_i)
{
	UNUSED_ARG(octx);
	UNUSED_ARG(vp);
	UNUSED_ARG(t_i);
	UNUSED_ARG(t_f);
	ICW("TODO");
	return NULL;
}

const out_val *impl_f2f(out_ctx *octx, const out_val *vp, type *from, type *to)
{
	UNUSED_ARG(octx);
	UNUSED_ARG(vp);
	UNUSED_ARG(from);
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
		type *rf, unsigned nargs,
		int arg_offsets[/*nargs*/])
{
	unsigned i;

	UNUSED_ARG(rf);

	for(i = 0; i < nargs; i++){
		out_asm(octx, "push { %s }", arm_reg_to_str(i));

		arg_offsets[i] = i * 4;
	}
}

void impl_func_prologue_save_variadic(out_ctx *octx, type *rf)
{
	UNUSED_ARG(octx);
	UNUSED_ARG(rf);
	ICW("TODO");
}

void impl_func_epilogue(out_ctx *octx, type *ty)
{
	UNUSED_ARG(ty);
	out_asm(octx, "mov sp, fp");
	out_asm(octx, "pop { fp, pc }");
}

const out_val *impl_load(
		out_ctx *octx, const out_val *from,
		const struct vreg *reg)
{
	switch(from->type){
		case V_CONST_I:
			/* TODO: >16 bit numbers? */
			out_asm(octx, "mov %s, #%d",
					arm_reg_to_str(reg->idx),
					(int)from->bits.val_i);
			break;

		case V_REG_SPILT:
			out_asm(octx, "mov %s, [%s]",
					arm_reg_to_str(reg->idx),
					arm_reg_to_str(reg->idx));
			break;
		case V_REG:
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
			ICW("TODO: setCOND");
	}

	return v_new_reg(octx, from, from->t, reg);
}

void impl_store(out_ctx *octx, const out_val *from, const out_val *to)
{
	const struct vreg *reg;

	from = v_to(octx, from, TO_REG);
	reg = &from->bits.regoff.reg;

	switch(to->type){
		case V_CONST_I:
			out_asm(octx, "str %s, #%d",
					arm_reg_to_str(reg->idx),
					(int)to->bits.val_i);
			break;

		case V_REG_SPILT:
			out_asm(octx, "str %s, [%s]",
					arm_reg_to_str(reg->idx),
					arm_reg_to_str(to->bits.regoff.reg.idx));
			break;
		case V_REG:
			out_asm(octx, "str %s, %s",
					arm_reg_to_str(reg->idx),
					arm_reg_to_str(to->bits.regoff.reg.idx));
			break;

		case V_LBL:
			out_asm(octx, "str %s, =%s",
					arm_reg_to_str(reg->idx),
					to->bits.lbl.str);
			break;

		case V_CONST_F:
			ICE("TODO");
			break;

		case V_FLAG:
			ICW("TODO: setCOND");
	}
}

const out_val *impl_op(out_ctx *octx, enum op_type op, const out_val *l, const out_val *r)
{
	const char *opc;

	switch(op){
		case op_multiply: opc = "mul"; goto op;
		case op_minus: opc = "sub"; goto op; //
		case op_divide: opc = "div"; goto op; //
		case op_modulus: opc = "mod"; goto op; //
		case op_plus: opc = "add"; goto op; //
		case op_xor: opc = "or"; goto op; //
		case op_or: opc = "or"; goto op; //
		case op_and: opc = "and"; goto op; //
op:
		{
			// TODO: similar to x86
			const char *rA, *rB;

			l = v_to_reg(octx, l);
			r = v_to_reg(octx, r);

			rA = arm_reg_to_str(l->bits.regoff.reg.idx);
			rB = arm_reg_to_str(r->bits.regoff.reg.idx);

			/* put the result in vtop-1's reg */
			out_asm(octx, "%s %s, %s, %s", opc, rB, rA, rB);

			out_val_consume(octx, r);
			return v_dup_or_reuse(octx, l, l->t);
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

		case op_eq:
		case op_ne:
		case op_le:
		case op_lt:
		case op_ge:
		case op_gt:
			ICW("TODO: setCOND");
			break;

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

	val = v_to_reg(octx, val);
	impl_reg_cp_no_off(octx, val, &reg);
	out_val_consume(octx, val);
}

void impl_reg_cp_no_off(
		out_ctx *octx, const out_val *from, const struct vreg *to_reg)
{
	UCC_ASSERT(from->type == V_REG, "reg cp non reg?");

	out_asm(octx, "mov %s, %s",
			arm_reg_to_str(to_reg->idx),
			arm_reg_to_str(from->bits.regoff.reg.idx));
}

int impl_reg_is_callee_save(const struct vreg *r, type *fr)
{
	(void)r;
	(void)fr;
	ICW("TODO");
	return 0;
}

int impl_reg_is_scratch(const struct vreg *reg)
{
	return (/*ARM_REG_R0 <= reg->idx &&*/ reg->idx < ARM_REG_R3)
		|| ARM_REG_R12 == reg->idx;
}

int impl_reg_to_idx(const struct vreg *reg)
{
	if(/*ARM_REG_R0 <= reg->idx &&*/ reg->idx < ARM_REG_R3)
		return reg->idx;

	UCC_ASSERT(reg->idx == ARM_REG_R12, "bad scratch reg");
	return ARM_REG_R3 + 1;
}

void impl_scratch_to_reg(int scratch, struct vreg *reg)
{
	if(scratch == ARM_REG_R3 + 1)
		reg->idx = ARM_REG_R12;
	else if(scratch > ARM_REG_R3 + 1)
		ICE("bad arm scratch reg %d", scratch);
	else
		reg->idx = scratch;
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
