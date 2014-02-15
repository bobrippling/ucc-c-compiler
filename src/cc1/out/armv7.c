#include <stdio.h>
#include <stdarg.h>

#include "../../util/util.h"

#include "armv7.h"

#include "../data_structs.h"
#include "asm.h"
#include "vstack.h"

#include "impl.h"
#include "out.h"

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

static void arm_jmp(const char *pre)
{
	switch(vtop->type){
		case V_CONST_F:
			ICE("call float");

		case V_FLAG:
		case V_REG_SAVE:
		case V_CONST_I:
			v_to_reg(vtop);
		case V_REG:
			out_asm("%sx %s", pre, arm_reg_to_str(vtop->bits.regoff.reg.idx));
			break;

		case V_LBL:
			out_asm("%s %s", pre, vtop->bits.lbl.str);
	}
}

void impl_call(const int nargs, type_ref *r_ret, type_ref *r_func)
{
	(void)r_ret;
	(void)r_func;

	if(nargs)
		ICW("TODO: nargs");

	arm_jmp("bl");
}

void impl_jmp(void)
{
	arm_jmp("b");
}

void impl_jcond(int true, const char *lbl)
{
	(void)true;
	(void)lbl;
	ICW("TODO: impl_jcond");
}

void impl_cast_load(
		struct vstack *vp,
		type_ref *small, type_ref *big,
		int is_signed)
{
	out_asm("TODO: cast load");
}

void impl_change_type(type_ref *t)
{
	vtop->t = t;
}

void impl_deref(
		struct vstack *vp,
		const struct vreg *to,
		type_ref *tpointed_to)
{
	out_asm("mov %s, [%s]", arm_reg_to_str(to->idx), "?");
}

void impl_i2f(struct vstack *vp, type_ref *t_i, type_ref *t_f)
{ ICW("TODO"); }

void impl_f2i(struct vstack *vp, type_ref *t_f, type_ref *t_i)
{ ICW("TODO"); }

void impl_f2f(struct vstack *vp, type_ref *from, type_ref *to)
{ ICW("TODO"); }

void impl_func_prologue_save_fp(void)
{
	out_asm("push { r4, lr }");
	out_asm("mov r4, sp");
}

void impl_func_prologue_save_call_regs(
		type_ref *rf, unsigned nargs,
		int arg_offsets[/*nargs*/])
{
	unsigned i;

	for(i = 0; i < nargs; i++){
		out_asm("push { %s }", arm_reg_to_str(i));

		arg_offsets[i] = i * 4;
	}
}

void impl_func_prologue_save_variadic(type_ref *rf)
{ ICW("TODO"); }

void impl_func_epilogue(type_ref *ty)
{
	out_asm("mov sp, r4");
	out_asm("pop { r4, pc }");
}

void impl_load(struct vstack *from, const struct vreg *reg)
{
	switch(vtop->type){
		case V_CONST_I:
			/* TODO: >16 bit numbers? */
			out_asm("mov %s, #%d",
					arm_reg_to_str(reg->idx),
					(int)from->bits.val_i);
			break;

		case V_REG_SAVE:
			out_asm("mov %s, [%s]",
					arm_reg_to_str(reg->idx),
					arm_reg_to_str(reg->idx));
			break;
		case V_REG:
			if(reg->idx == from->bits.regoff.reg.idx)
				break;
			out_asm("mov %s, %s",
					arm_reg_to_str(reg->idx),
					arm_reg_to_str(from->bits.regoff.reg.idx));
			break;

		case V_LBL:
			out_asm("ldr %s, =%s",
					arm_reg_to_str(reg->idx),
					from->bits.lbl.str);
			break;

		case V_CONST_F:
			ICE("TODO");
			break;

		case V_FLAG:
			ICW("TODO: setCOND");
	}
}

void impl_store(struct vstack *from, struct vstack *to)
{
	struct vreg *reg;
	v_to(from, TO_REG);
	reg = &from->bits.regoff.reg;

	switch(to->type){
		case V_CONST_I:
			out_asm("str %s, #%d",
					arm_reg_to_str(reg->idx),
					(int)to->bits.val_i);
			break;

		case V_REG_SAVE:
			out_asm("str %s, [%s]",
					arm_reg_to_str(reg->idx),
					arm_reg_to_str(to->bits.regoff.reg.idx));
			break;
		case V_REG:
			out_asm("str %s, %s",
					arm_reg_to_str(reg->idx),
					arm_reg_to_str(to->bits.regoff.reg.idx));
			break;

		case V_LBL:
			out_asm("str %s, =%s",
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

void impl_load_fp(struct vstack *from)
{ ICW("TODO"); }

void impl_load_iv(struct vstack *from)
{
	v_to_reg(from);
	impl_load(from, &vtop->bits.regoff.reg);
}

void impl_op(enum op_type op)
{
	switch(op){
		case op_multiply:
		case op_divide:
		case op_modulus:
		case op_plus:
		case op_minus:
		case op_xor:
		case op_or:
		case op_and:
			ICW("TODO: %s", op_to_str(op));

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
}

void impl_op_unary(enum op_type op)
{
	switch(op){
		default:
			ICE("unary %s", op_to_str(op));

		case op_plus:
			/* noop */
			return;

		case op_minus:
			/* TODO: factor out */
			out_push_zero(vtop->t);
			out_op(op_minus);
			return;

		case op_bnot:
			UCC_ASSERT(!type_ref_is_floating(vtop->t), "~float");

			v_to_reg(vtop);

			out_asm("mvn %s, %s",
					arm_reg_to_str(vtop->bits.regoff.reg.idx),
					arm_reg_to_str(vtop->bits.regoff.reg.idx));
			return;

		case op_not:
			out_push_zero(vtop->t);
			out_op(op_eq);
			return;
	}
}

void impl_pop_func_ret(type_ref *ty)
{
	struct vreg reg = VREG_INIT(REG_RET_I, 0);

	UCC_ASSERT(!type_ref_is_floating(ty), "TODO: float return");

	v_to_reg(vtop);
	impl_reg_cp(vtop, &reg);
	vpop();
}

void impl_reg_cp(struct vstack *from, const struct vreg *r)
{
	UCC_ASSERT(from->type == V_REG, "reg cp non reg?");

	out_asm("mov %s, %s",
			arm_reg_to_str(r->idx),
			arm_reg_to_str(from->bits.regoff.reg.idx));
}

int impl_reg_is_callee_save(const struct vreg *r, type_ref *fr)
{
	(void)r;
	(void)fr;
	ICW("TODO");
	return 0;
}

int impl_reg_to_scratch(const struct vreg *reg)
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

void impl_set_nan(type_ref *t)
{ ICE("TODO"); }

void impl_set_overflow(void)
{ ICE("TODO"); }

void impl_undefined(void)
{ ICE("TODO"); }
