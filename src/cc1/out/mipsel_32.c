#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "../../util/util.h"
#include "../../util/alloc.h"
#include "../../util/platform.h"
#include "../data_structs.h"
#include "vstack.h"
#include "impl.h"
#include "../cc1.h"
#include "asm.h"
#include "common.h"
#include "out.h"
#include "lbl.h"

#define SYMBOLIC_REGS

#define F_DEBUG cc_out[SECTION_TEXT]
#define REG_STR_SZ 4 /* r[0-31] / r[a-z][0-9a-z] */

#define PIC_CHECK(vp)                 \
			if(vp->bits.lbl.pic){           \
				static int w;                 \
				if(!w)                        \
					ICW("TODO: pic on mips");   \
			}

#define TODO() ICE("TODO")

#pragma GCC diagnostic ignored "-Wunused-parameter"

static const char *const sym_regs[] = {
	"0", "at",
	"v0", "v1",
	"a0", "a1", "a2", "a3",
	"t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
	"s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
	"t8", "t9",
	"k0", "k1",
	"gp",
	"sp", "fp", "ra"
};

static char *reg_str_r_i(char buf[REG_STR_SZ], int r)
{
#ifdef SYMBOLIC_REGS
	strcpy(buf, sym_regs[r]);
#else
	snprintf(buf, REG_STR_SZ, "r%d", r);
#endif
	return buf;
}

static char *reg_str_r(char buf[REG_STR_SZ], struct vstack *vp)
{
	UCC_ASSERT(vp->type == REG, "not reg");
	return reg_str_r_i(buf, vp->bits.reg);
}

static char *reg_str_i(int r)
{
	static char buf[REG_STR_SZ];
	return reg_str_r_i(buf, r);
}

void impl_func_prologue(int stack_res, int nargs, int variadic)
{
	/* FIXME: very similar to x86_64::func_prologue - merge */
	if(variadic)
		ICW("variadic function prologue on MIPS");

	out_asm("addi  $sp, $sp, -8"); /* space for saved ret and fp */
	out_asm("sw    $ra, 4($sp)");  /* save ret */
	out_asm("sw    $fp,  ($sp)");  /* save fp */
	out_asm("move  $fp, $sp");     /* new frame */

	if(nargs)
		ICW("TODO: push args");

	out_asm("addi $sp, $sp, -%d", stack_res);
}

void impl_func_epilogue(void)
{
	out_asm("move $sp, $fp");
	out_asm("lw $fp,  ($sp)");
	out_asm("lw $ra, 4($sp)");
	out_asm("addi $sp, $sp, 8");
	out_asm("j $ra");
}

void impl_lea(struct vstack *from, int reg)
{
	char rstr[REG_STR_SZ];

	reg_str_r_i(rstr, reg);

	switch(from->type){
		case FLAG:
			ICE("FLAG in MIPS");
		case CONST:
		case REG:
		case STACK_SAVE:
			ICE("%s of %d", __func__, from->type);

		case STACK:
			out_asm("la $%s, %d($fp)", rstr, from->bits.off_from_bp);
			break;
		case LBL:
			PIC_CHECK(from);
			out_asm("la $%s, %s", rstr, from->bits.lbl.str);
			break;
	}
}

void impl_load(struct vstack *from, int reg)
{
	char rstr[REG_STR_SZ];

	reg_str_r_i(rstr, reg);

	switch(from->type){
		case FLAG:
			ICE("FLAG in MIPS");

		case LBL:
			PIC_CHECK(from);
			out_asm("lw $%s, %s", rstr, from->bits.lbl.str);
			break;

		case CONST:
			if(from->bits.val == 0)
				out_asm("move $%s, $%s", rstr, sym_regs[MIPS_REG_ZERO]);
			else
				out_asm("li $%s, %d", rstr, from->bits.val);
			break;

		case REG:
			impl_reg_cp(from, reg);
			break;

		case STACK:
		case STACK_SAVE:
			out_asm("lw $%s, %d($fp)",
					rstr,
					from->bits.off_from_bp);
	}
}

void impl_store(struct vstack *from, struct vstack *to)
{
	char rstr[REG_STR_SZ];

	v_to_reg(from); /* room for optimisation, e.g. const */
	reg_str_r(rstr, from);

	switch(to->type){
		case FLAG:
			ICE("FLAG in MIPS");

		case STACK_SAVE:
			ICE("store to %d", to->type);

		case LBL:
			PIC_CHECK(from);
			out_asm("sw $%s, %s", rstr, to->bits.lbl.str);
			break;

		case CONST:
			out_asm("sw $%s, %d", rstr, to->bits.val);
			break;

		case REG:
			out_asm("sw $%s, ($%s)",
					rstr,
					reg_str_i(to->bits.reg));
			break;

		case STACK:
			out_asm("sw $%s, %d($fp)",
					rstr, to->bits.off_from_bp);
	}
}

static void impl_reg_cp_reg(int from, int to)
{
	char rstr[REG_STR_SZ];

	out_asm("move $%s, $%s",
			reg_str_i(to),
			reg_str_r_i(rstr, from));
}

void impl_reg_swp(struct vstack *va, struct vstack *vb)
{
	const int r = v_unused_reg(1),
	          a = va->bits.reg,
	          b = vb->bits.reg;

	impl_reg_cp_reg(a, r);
	impl_reg_cp_reg(b, a);
	impl_reg_cp_reg(r, b);
}

void impl_reg_cp(struct vstack *from, int r)
{
	if(from->type == REG && from->bits.reg == r) /* FIXME: x86_64 merge */
		return;

	impl_reg_cp_reg(from->bits.reg, r);
}

void impl_op(enum op_type op)
{
	const char *cmp;
	char r_vtop[REG_STR_SZ], r_vtop1[REG_STR_SZ];

	/* TODO: use immediate instructions */
	v_to_reg(vtop - 1);
	reg_str_r(r_vtop1, vtop-1);

	v_to_reg(vtop);
	reg_str_r(r_vtop, vtop);

	/* TODO: signed */
	switch(op){
#define OP(ty) case op_ ## ty: cmp = #ty; goto cmp
		OP(eq);
		OP(ne);
		OP(le);
		OP(lt);
		OP(ge);
		OP(gt);
#undef OP
cmp:
		out_asm("s%s $%s, $%s, $%s",
				cmp, r_vtop1, r_vtop1, r_vtop);
		break;

		case op_orsc:
		case op_andsc:
		case op_not:
		case op_bnot:
		case op_unknown:
			ICE("%s", op_to_str(op));

#define OP(ty, n) case op_ ## ty: cmp = n; goto op
		OP(multiply, "mul");
		OP(plus,     "add");
		OP(minus,    "sub");
		OP(divide,   "div");
		OP(modulus,  "div"); /* special */
		OP(xor,      "xor");
		OP(or,       "or");
		OP(and,      "and");
		OP(shiftl,   "sll");
		OP(shiftr,   "srl");
#undef OP
op:
		out_asm("%s $%s, $%s, $%s",
				cmp, r_vtop1, r_vtop1, r_vtop);

		switch(op){
			case op_divide:
				out_asm("mflo $%s", r_vtop1);
				break;
			case op_modulus:
				out_asm("mfhi $%s", r_vtop1);
			default:
				break;
		}
		break;
	}

	vpop();
}

void impl_deref_reg()
{
	char rstr[REG_STR_SZ];

	UCC_ASSERT(vtop->type == REG, "not reg (%d)", vtop->type);

	reg_str_r_i(rstr, vtop->bits.reg);

	out_asm("lw $%s, ($%s)", rstr, rstr);

	v_deref_decl(vtop);
}

void impl_op_unary(enum op_type op)
{
	char r_vtop[REG_STR_SZ];

	v_to_reg(vtop);
	reg_str_r(r_vtop, vtop);

	switch(op){
		default:
			ICE("unary %s", op_to_str(op));

		case op_plus:
			/* noop */
			return;

		case op_minus:
			out_asm("sub $%s, $%s, $%s",
					r_vtop, sym_regs[MIPS_REG_ZERO], r_vtop);
			break;

		case op_bnot:
			out_asm("not $%s, $%s", r_vtop, r_vtop);
			break;

		case op_not:
			out_asm("seq $%s, $%s, $%s",
					r_vtop, r_vtop, sym_regs[MIPS_REG_ZERO]);
	}
}

void impl_normalise(void)
{
	fprintf(F_DEBUG, "TODO: normalise %d\n", vtop->type);
}

void impl_cast(type_ref *from, type_ref *to)
{
	/* TODO FIXME: combine with code in x86_64 */
	ICW("MIPS cast");
}

void impl_jmp_reg(int r)
{
	out_asm("jr $%s", reg_str_i(r));
}

void impl_jmp_lbl(const char *lbl)
{
	out_asm("j %s", lbl);
}

static char *mips_cmp(struct flag_opts *flag)
{
	/* FIXME: factor + check */
	switch(flag->cmp){
#define OP(e, s, u) case flag_ ## e: return flag->is_signed ? s : u
		OP(eq, "e" , "e");
		OP(ne, "ne", "ne");
		OP(le, "le", "be");
		OP(lt, "l",  "b");
		OP(ge, "ge", "ae");
		OP(gt, "g",  "a");
#undef OP

		/*case flag_z:  return "z";
		case flag_nz: return "nz";*/
	}
	return NULL;
}


void impl_jcond(int true, const char *lbl)
{
	switch(vtop->type){
		case FLAG:
			UCC_ASSERT(true, "jcond(false) for flag - should've been inverted");

			out_asm("b%s %s", mips_cmp(&vtop->bits.flag), lbl);
			break;

		case CONST:
			if(true == !!vtop->bits.val){ /* FIXME: factor */
				out_asm("b %s", lbl);
				out_comment("// constant jmp condition %d", vtop->bits.val);
			}
			break;

		case LBL:
		case STACK:
		case STACK_SAVE:
			v_to_reg(vtop); /* FIXME: factor */

		case REG:
		{
			char buf[REG_STR_SZ];

			reg_str_r(buf, vtop);

			out_asm("b%s %s, %s, %s", "?", lbl, "FIXME", "FIXME");
		}
	}
}

void impl_call(int nargs, type_ref *r_ret, type_ref *r_func)
{
	int i;

	UCC_ASSERT(nargs <= N_CALL_REGS, "TODO: more than 4 args"); /* FIXME */

	for(i = 0; i < nargs; i++){
		static const int call_regs[] = {
			MIPS_REG_A0,
			MIPS_REG_A1,
			MIPS_REG_A2,
			MIPS_REG_A3,
		};
		int ri = call_regs[i];

		v_freeup_reg(ri, 1);
		out_load(vtop, ri);
		vpop();
	}

	switch(vtop->type){
		default:
			v_to_reg(vtop);
		case REG:
			out_asm("jalr $%s", reg_str_i(vtop->bits.reg));
			break;
		case LBL:
			out_asm("jal %s", vtop->bits.lbl.str);
	}
}

void impl_pop_func_ret(type_ref *r)
{
	(void)r;
	out_load(vtop, REG_RET);
	vpop();
}

void impl_undefined(void)
{
	ICE("TODO: mips undefined instruction");
}

int impl_frame_ptr_to_reg(int nframes)
{
	ICE("TODO: builtin frame address");
	return -1;
}
