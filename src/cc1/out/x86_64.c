#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "../../util/util.h"
#include "../../util/alloc.h"
#include "../../util/platform.h"
#include "../data_structs.h"
#include "vstack.h"
#include "x86_64.h"
#include "../cc1.h"
#include "asm.h"
#include "common.h"
#include "out.h"

#ifndef MIN
#  define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#define RET_REG 0
#define REG_STR_SZ 8

#define REG_A 0
#define REG_B 1
#define REG_C 2
#define REG_D 3

#define VSTACK_STR_SZ 128

#define MOV_OR_LEA(vp) ((vp)->is_addr ? "lea" : "mov")


static const char regs[] = "abcd";
static const struct
{
	const char *name;
	int idx;
} call_regs[] = {
	{ "rdi", -1 },
	{ "rsi", -1 },
	{ "rdx", REG_D },
	{ "rcx", REG_C },
	{ "r8",  -1 },
	{ "r9",  -1 },
};

static const char *reg_str(int reg)
{
	const char *regpre, *regpost;
	static char regstr[REG_STR_SZ];

	UCC_ASSERT((unsigned)reg < N_REGS, "invalid x86 reg %d", reg);

	asm_reg_name(NULL, &regpre, &regpost);

	snprintf(regstr, sizeof regstr, "%s%c%s", regpre, regs[reg], regpost);

	return regstr;
}

static const char *vstack_str_r(char buf[VSTACK_STR_SZ], struct vstack *vs)
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

static const char *vstack_str(struct vstack *vs)
{
	static char buf[VSTACK_STR_SZ];
	return vstack_str_r(buf, vs);
}

static void out_asm(const char *fmt, ...) __printflike(1, 2);

static void out_asm(const char *fmt, ...)
{
	va_list l;
	putchar('\t');
	va_start(l, fmt);
	vprintf(fmt, l);
	va_end(l);
	putchar('\n');
}

void impl_comment(const char *fmt, va_list l)
{
	printf("\t// ");
	vprintf(fmt, l);
	printf("\n");
}

void out_func_prologue(int stack_res, int nargs)
{
	(void)nargs;

	out_asm("push %%rbp");
	out_asm("movq %%rsp, %%rbp");

	if(stack_res){
		int i, n_reg_args;

		out_asm("subq $0x%x, %%rsp", stack_res);

		n_reg_args = MIN(nargs, N_CALL_REGS);

		for(i = 0; i < n_reg_args; i++){
			out_asm("mov_ %%%s, -0x%x(%%rbp)",
					call_regs[i].name,
					platform_word_size() * (i + 1));
		}
	}
}

void out_func_epilogue(void)
{
	out_asm("leaveq");
	out_asm("retq");
}

void out_pop_func_ret(decl *d)
{
	(void)d;

	impl_load(vtop, RET_REG);
	vpop();
}

static const char *x86_cmp(enum flag_cmp cmp, decl *d)
{
	int is_signed = d ? d->type->is_signed : 1; /* TODO */

	switch(cmp){
#define OP(e, s, u) case flag_ ## e: return is_signed ? s : u
		OP(eq, "e" , "e");
		OP(ne, "ne", "ne");
		OP(le, "le", "be");
		OP(lt, "lt", "b");
		OP(ge, "ge", "ae");
		OP(gt, "gt", "a");
#undef OP

		case flag_z:  return "z";
		case flag_nz: return "nz";
	}
	return NULL;
}

static enum flag_cmp op_to_flag(enum op_type op)
{
	switch(op){
#define OP(x) case op_ ## x: return flag_ ## x
		OP(eq);
		OP(ne);
		OP(le);
		OP(lt);
		OP(ge);
		OP(gt);
#undef OP

		default:
			break;
	}

	ICE("invalid op");
	return -1;
}

static void x86_load(struct vstack *from, const char *regstr)
{
	if(from->type == FLAG){
		out_asm("set%s %%%s",
				x86_cmp(from->bits.flag, from->d),
				regstr);
	}else{
		out_asm("%s_ %s, %%%s",
				MOV_OR_LEA(from),
				vstack_str(from),
				regstr);
	}
}

void impl_load(struct vstack *from, int reg)
{
	if(from->type == REG && reg == from->bits.reg)
		return;

	x86_load(from, reg_str(reg));
}

void impl_store(int reg, struct vstack *where)
{
	char regstr[REG_STR_SZ];

	if(where->type == REG && reg == where->bits.reg)
		return;

	UCC_ASSERT(where->type != CONST && where->type != FLAG, "invalid store");

	strcpy(regstr, reg_str(reg));
	out_asm("mov_ %%%s, %s",
			regstr,
			vstack_str(where));
}

void impl_op(enum op_type op)
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

			vtop_clear();
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

			vtop2_prepare_op();

			out_asm("cmp_ %s, %s",
					vstack_str(&vtop[-1]),
					vstack_str_r(buf, vtop));

			vpop();
			vtop_clear();
			vtop->type = FLAG;
			vtop->bits.flag = op_to_flag(op);

			return;
		}

		case op_orsc:
		case op_andsc:

		case op_shiftl:
		case op_shiftr:
			ICE("TODO: %s", op_to_str(op));

		default:
			ICE("invalid op %s", op_to_str(op));
	}

	{
		char buf[VSTACK_STR_SZ];

		vtop2_prepare_op();

		out_asm("%s %s, %s", opc,
				vstack_str_r(buf, &vtop[ 0]),
				vstack_str(       &vtop[-1]));

		/* remove first operand - result is then in vtop (already in a reg) */
		vpop();

		if(normalise)
			out_normalise();
	}
}

void impl_op_unary(enum op_type op)
{
	const char *opc;

	switch(op){
		default:
			ICE("invalid unary op %s", op_to_str(op));

		case op_plus:
			/* noop */
			return;

		case op_deref:
		{
			const int reg = v_to_reg(vtop);
			const char *rs = reg_str(reg);

			out_asm("mov_ (%%%s), %%%s", rs, rs);

			return;
		}

#define OP(o, s) case op_ ## o: opc = #s; break
		OP(not, not);
		OP(minus, neg);
		OP(bnot, not);
#undef OP
	}

	out_asm("%s %s", opc, vstack_str(vtop));

	if(op == op_not)
		out_normalise();
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

void impl_jtrue(const char *lbl)
{
	switch(vtop->type){
		case FLAG:
			out_asm("j%s %s",
					x86_cmp(vtop->bits.flag, vtop->d),
					lbl);
			break;

		case CONST:
			if(vtop->bits.val)
				out_asm("jmp %s // constant jmp condition %d", lbl, vtop->bits.val);
			break;

		case REG:
		case STACK:
		case LBL:
			out_asm("jnz %s", vstack_str(vtop));
	}

	vpop();
}

void impl_call(const int nargs)
{
	int i, ncleanup;

	for(i = 0; i < MIN(nargs, N_CALL_REGS); i++){
		int ri;

		ri = call_regs[i].idx;
		if(ri != -1)
			v_freeup_reg(ri, 1);

		x86_load(vtop, call_regs[i].name);
		vpop();
	}
	/* push remaining args onto the stack */
	ncleanup = nargs - i;
	for(; i < nargs; i++){
		out_asm("pushq %s", vstack_str(vtop));
		vpop();
	}

	out_asm("callq %s", x86_call_jmp_target(vtop));

	if(ncleanup)
		out_asm("addq $0x%x, %%rsp", ncleanup * platform_word_size());

	/* return type */
	vtop_clear();
	vtop->type = REG;
	vtop->bits.reg = REG_A;
	vtop->d = NULL; /* TODO */
}
