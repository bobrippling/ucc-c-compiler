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

#define F_DEBUG cc_out[SECTION_TEXT]
#define REG_STR_SZ 4 /* r[0-32] */

#define PIC_CHECK(vp)                 \
			if(vp->bits.lbl.pic){           \
				static int w;                 \
				if(!w)                        \
					ICW("TODO: pic on mips");   \
			}

#define TODO() ICE("TODO")

#pragma GCC diagnostic ignored "-Wunused-parameter"

static char *reg_str_r_i(char buf[REG_STR_SZ], int r)
{
	snprintf(buf, REG_STR_SZ, "r%d", r);
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
	fprintf(F_DEBUG, "MIPS prologue %d %d %d\n", stack_res, nargs, variadic);

#if 0
	out_asm("addiu $sp, $sp,-8"); /* space for saved ret */
	out_asm("sw    $fp, 4($sp)"); /* save ret */
	out_asm("move  $fp, $sp");    /* new frame */

	out_asm("addiu $sp, $sp,-%d", stack_res); /* etc */
#endif
}

void impl_func_epilogue(void)
{
	fprintf(F_DEBUG, "TODO: epilogue\n");
}

void impl_lea(struct vstack *from, int reg)
{
	char rstr[REG_STR_SZ];

	reg_str_r_i(rstr, reg);

	switch(from->type){
		case CONST:
		case REG:
		case FLAG:
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
			TODO();

		case LBL:
			PIC_CHECK(from);
			out_asm("lw $%s, %s", rstr, from->bits.lbl.str);
			break;

		case CONST:
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
	fprintf(F_DEBUG, "TODO: store %d -> %d\n", from->type, to->type);
}

void impl_reg_swp(struct vstack *a, struct vstack *b)
{
	fprintf(F_DEBUG, "TODO: swap %d <-> %d\n", a->bits.reg, b->bits.reg);
}

void impl_reg_cp(struct vstack *from, int r)
{
	if(from->type == REG && from->bits.reg == r) /* TODO: x86_64 merge */
		return;

	fprintf(F_DEBUG, "TODO: copy %d -> reg %d\n", from->type, r);
}

void impl_op(enum op_type op)
{
	fprintf(F_DEBUG, "TODO: %d %s %d\n",
			vtop[-1].type, op_to_str(op), vtop->type);

	vpop();
}

void impl_deref_reg()
{
	char from[REG_STR_SZ], to[REG_STR_SZ];
	int r = v_unused_reg(1);

	UCC_ASSERT(vtop->type == REG, "not reg (%d)", vtop->type);

	reg_str_r_i(from, vtop->bits.reg);
	reg_str_r_i(to,   r);

	out_asm("lw $%s, ($%s)",
			to, from);

	vtop->type = REG;
	vtop->bits.reg = r;
}

void impl_op_unary(enum op_type op)
{
	fprintf(F_DEBUG, "TODO: %s %d\n", op_to_str(op), vtop->type);
}

void impl_normalise(void)
{
	fprintf(F_DEBUG, "TODO: normalise %d\n", vtop->type);
}

void impl_cast(type_ref *from, type_ref *to)
{
	/* TODO FIXME: combine with code in x86_64 */
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
			1, 2, 3, 4
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
	fprintf(F_DEBUG, "TODO: undefined instruction\n");
}

int impl_frame_ptr_to_reg(int nframes)
{
	ICE("TODO: builtin frame address");
	return -1;
}
