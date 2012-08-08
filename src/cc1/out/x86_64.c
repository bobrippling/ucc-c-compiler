#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "../../util/util.h"
#include "../../util/alloc.h"
#include "../data_structs.h"
#include "vstack.h"
#include "x86_64.h"
#include "../cc1.h"
#include "asm.h"
#include "common.h"
#include "out.h"

#define RET_REG 0
#define REG_STR_SZ 8

#define REG_A 0
#define REG_D 3

#define VSTACK_STR_SZ 128

const char regs[] = "abcd";

static const char *
reg_str(int reg)
{
	const char *regpre, *regpost;
	static char regstr[REG_STR_SZ];

	UCC_ASSERT((unsigned)reg < N_REGS, "invalid x86 reg %d", reg);

	asm_reg_name(NULL, &regpre, &regpost);

	snprintf(regstr, sizeof regstr, "%s%c%s", regpre, regs[reg], regpost);

	return regstr;
}

static const char *
vstack_str_r(char buf[VSTACK_STR_SZ], struct vstack *vs)
{
	switch(vs->type){
		case CONST:
			SNPRINTF(buf, VSTACK_STR_SZ, "$%d", vs->bits.val);
			break;

		case FLAG:
			ICE("%s shouldn't be called with cmp-flag data", __func__);

		case LBL:
			SNPRINTF(buf, VSTACK_STR_SZ, "%s%s",
					vs->bits.lbl.str, vs->bits.lbl.pic ? "(%%rip)" : "");
			break;

		case REG:
			SNPRINTF(buf, VSTACK_STR_SZ, "%%%s", reg_str(vs->bits.reg));
			break;

		case STACK:
		{
			int n = vs->bits.off_from_bp;
			SNPRINTF(buf, VSTACK_STR_SZ, "%s0x%x(%%rbp)", n < 0 ? "-" : "", abs(n));
			break;
		}
	}

	return buf;
}

static const char *
vstack_str(struct vstack *vs)
{
	static char buf[VSTACK_STR_SZ];
	return vstack_str_r(buf, vs);
}

static void
out_asm(const char *fmt, ...)
{
	va_list l;
	putchar('\t');
	va_start(l, fmt);
	vprintf(fmt, l);
	va_end(l);
	putchar('\n');
}

void
impl_comment(const char *fmt, va_list l)
{
	printf("\t// ");
	vprintf(fmt, l);
	printf("\n");
}

void
out_func_prologue(int offset)
{
	out_asm("push %%rbp");
	out_asm("movq %%rsp, %%rbp");
	if(offset)
		out_asm("subq $%d, %%rsp", offset);
}

void
out_func_epilogue(void)
{
	out_asm("leaveq");
	out_asm("retq");
}

void
out_pop_func_ret(decl *d)
{
	const int r = RET_REG;

	(void)d;

	impl_load(vtop, r);
	vpop();
}

static const char *x86_cmp(enum op_type cmp, int is_signed)
{
	switch(cmp){
#define OP(e, s, u) case op_ ## e: return is_signed ? s : u
		OP(eq, "e" , "e");
		OP(ne, "ne", "ne");
		OP(le, "le", "be");
		OP(lt, "lt", "b");
		OP(ge, "ge", "ae");
		OP(gt, "gt", "a");
#undef OP

		default:
			ICE("invalid x86 comparison");
	}
	return NULL;
}

void
impl_load(struct vstack *from, int reg)
{
	if(from->type == FLAG){
		int is_signed = from->d ? from->d->type->is_signed : 1;

		if(!from->d)
			ICW("TODO: decl for signed-cmp");

		out_asm("set%s %%%s",
				x86_cmp(from->bits.cmp, is_signed),
				reg_str(reg));
	}else{
		char buf[REG_STR_SZ];

		strcpy(buf, reg_str(reg));

		out_asm("%s_ %s, %%%s",
				from->is_addr ? "lea" : "mov",
				vstack_str(from),
				buf);
	}
}

void
impl_store(int reg, struct vstack *where)
{
	char regstr[REG_STR_SZ];

	if(where->type == REG && reg == where->bits.reg)
		return;

	UCC_ASSERT(where->type != CONST && where->type != FLAG, "invalid store");

	strcpy(regstr, reg_str(reg));
	out_asm("mov_ %%%s, %s", regstr, vstack_str(where));
}

void
impl_op(enum op_type op)
{
	const char *opc;
	int normalise = 0;

	switch(op){
#define OP(e, s) case op_ ## e: opc = s; break
		OP(multiply, "imul");
		OP(plus,     "add");
		OP(minus,    "sub");
		OP(xor,      "xor");
		OP(or,       "or");
		OP(and,      "and");

		case op_not:
			normalise = 1;
		OP(bnot,     "not");
#undef OP


		case op_modulus:
		case op_divide:
		{
			/*
			 * divides the 64 bit integer EDX:EAX
			 * by the operand
			 * quotient  -> eax
			 * remainder -> edx
			 */
			int r_div;

			/*
			 * if we are using reg_[ad] elsewhere
			 * and they aren't queued for this idiv
			 * then save them, so we can use them
			 * for idiv
			 */
			v_freeup_reg(REG_A, 2);
			v_freeup_reg(REG_D, 2);

			r_div = v_to_reg(&vtop[-1]);
			UCC_ASSERT(r_div == REG_A, "register A not chosen for idiv");

			out_asm("cqto");

			/* idiv takes either a reg or memory address */
			switch(vtop->type){
				default:
					v_to_reg(vtop);
					/* fall */

				case REG:
				case STACK:
					out_asm("idiv %s", vstack_str(vtop));
			}

			vpop();

			vtop->type = REG;
			vtop->bits.reg = op == op_modulus ? REG_D : REG_A;
			return;
		}

		case op_eq:
		case op_ne:
		case op_le:
		case op_lt:
		case op_ge:
		case op_gt:
		{
			char buf[VSTACK_STR_SZ];

			out_asm("cmp_ %s, %s",
					vstack_str(&vtop[-1]),
					vstack_str_r(buf, vtop));

			vpop();
			vtop->type = FLAG;
			vtop->bits.cmp = op;

			return;
		}

		case op_orsc:
		case op_andsc:

		case op_shiftl:
		case op_shiftr:
			ICE("TODO: %s", op_to_str(op));

		default:
			ICW("invalid op %s", op_to_str(op));
	}

	{
		char buf[VSTACK_STR_SZ];

		/* vtop[-1] must be a reg - try to swap first */
		if(vtop[-1].type != REG){
			if(vtop->type == REG)
				vswap();
			else
				v_to_reg(&vtop[-1]);
		}

		out_asm("%s %s, %s", opc,
				vstack_str_r(buf, &vtop[ 0]),
				vstack_str(       &vtop[-1]));

		/* remove first operand - result is then in vtop (already in a reg) */
		vpop();

		if(normalise)
			out_normalise();
	}
}

int impl_op_unary(enum op_type op)
{
	const char *opc;
	const int reg = v_to_reg(vtop);

	switch(op){
		default:
			ICE("invalid unary op %s", op_to_str(op));

		case op_plus:
			/* noop */
			return reg;

#define OP(t, s) case op_ ## t: opc = s; break
		OP(minus, "neg");
		OP(bnot,  "not");
		OP(not,   "not");
#undef OP

		case op_deref:
		{
			const char *rs = reg_str(reg);

			out_asm("mov_ (%%%s), %%%s", rs, rs);

			return reg;
		}
	}

	out_asm("%s %s", opc, reg_str(reg));

	if(op == op_not)
		out_normalise();

	return reg;
}

static const char *x86_call_jmp_target(struct vstack *vp)
{
	static char buf[VSTACK_STR_SZ + 2];

	if(vp->type == LBL)
		return vp->bits.lbl.str;

	SNPRINTF(buf, sizeof buf, "*%%%s", reg_str(v_to_reg(vp)));

	return buf;
}

void impl_jmp()
{
	out_asm("jmp %s", x86_call_jmp_target(vtop));
}

void impl_call(const int nargs)
{
	const char *call_regs[] = {
		"rdi",
		"rsi",
		"rdx",
		"rcx",
		"r8",
		"r9",
	};
	int i;

	for(i = 0; i < nargs; i++){
		/*out_store(vtop, call_regs[i]); * something like this... */
	}

	out_asm("call %s", x86_call_jmp_target(vtop));

	out_asm("// TODO: stack clean (%d)", nargs);
}
