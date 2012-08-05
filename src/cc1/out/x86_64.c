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

#define VSTACK_STR_SZ 128

const char regs[] = "abcd";

static const char *
reg_str(int reg)
{
	const char *regpre, *regpost;
	static char regstr[REG_STR_SZ];

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
			ICE("TODO: %s flag", __func__);
			break;

		case LBL:
			SNPRINTF(buf, VSTACK_STR_SZ, "%s(%%rip)", vs->bits.lbl);
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

void
impl_load(struct vstack *from, int reg)
{
	if(from->type == FLAG){
		out_asm("setnz %%%s", reg_str(reg));
	}else if(from->type == REG && from->bits.reg == reg){
		/* noop */
	}else{
		char buf[REG_STR_SZ];

		strcpy(buf, reg_str(reg));

		out_asm("mov_ %s, %%%s", vstack_str(from), buf);
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

int
impl_op(enum op_type op)
{
	const char *opc;

	switch(op){
#define OP(e, s) case op_ ## e: opc = s; break
		OP(multiply, "imul");
		OP(plus,     "add");
		OP(minus,    "sub");
		OP(modulus,  "mod");
#undef OP

		case op_divide:

		case op_eq:
		case op_ne:
		case op_le:
		case op_lt:
		case op_ge:
		case op_gt:

		case op_xor:
		case op_or:
		case op_and:
		case op_orsc:
		case op_andsc:
		case op_not:
		case op_bnot:

		case op_shiftl:
		case op_shiftr:
			ICE("TODO: %s", op_to_str(op));

		default:
			ICW("invalid op %s", op_to_str(op));
	}

	{
		char buf[VSTACK_STR_SZ];

		out_asm("%s %s, %s", opc,
				vstack_str_r(buf, &vtop[ 0]),
				vstack_str(       &vtop[-1]));

		return vtop[-1].bits.reg;
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
