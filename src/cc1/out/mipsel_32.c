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

#define TODO() ICE("TODO")

void impl_func_prologue(int stack_res, int nargs, int variadic)
{
	/* FIXME: very similar to x86_64::func_prologue - merge */
	fprintf(F_DEBUG, "MIPS prologue %d %d %d\n", stack_res, nargs, variadic);

	out_asm("addiu $sp, $sp,-8"); /* space for saved ret */
	out_asm("sw    $fp, 4($sp)"); /* save ret */
	out_asm("move  $fp, $sp");    /* new frame */

	out_asm("addiu $sp, $sp,-%d", stack_res); /* etc */
}

void impl_func_epilogue(void)
{
	fprintf(F_DEBUG, "TODO: epilogue\n");
}

void impl_load(struct vstack *from, int reg)
{
	fprintf(F_DEBUG, "TODO: load into reg %d\n", reg);
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

	TODO();
}

void impl_op(enum op_type op)
{
	fprintf(F_DEBUG, "TODO: %d %s %d\n",
			vtop[-1].type, op_to_str(op), vtop->type);

	vpop();
}

void impl_deref()
{
	fprintf(F_DEBUG, "TODO: deref %d\n", vtop->type);
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

void impl_jmp()
{
	switch(vtop->type){
		case LBL:
		case REG:
			out_asm("j%s %s",
					vtop->type == REG ? "r" : "",
					vtop->bits.lbl.str);
			break;

		default:
			ICE("invalid jmp target %d", vtop->type);
	}
}

static char *reg_str_r(char buf[REG_STR_SZ], struct vstack *vp)
{
	UCC_ASSERT(vp->type == REG, "not reg");
	snprintf(buf, REG_STR_SZ, "r%d", vp->bits.reg);
	return buf;
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

		case STACK:
		case STACK_SAVE:
		case LBL:
			v_to_reg(vtop); /* FIXME: factor */

		case REG:
		{
			char buf[REG_STR_SZ];

			reg_str_r(buf, vtop);

			out_asm("b%s %s, %s, %s", lbl, "FIXME", "FIXME");
		}
	}
}

void impl_call(int nargs, type_ref *r_ret, type_ref *r_func)
{
	out_asm("// funcargs...");
	out_asm("jal %s", "???");

	while(nargs --> 0)
		vpop();
}

void impl_pop_func_ret(type_ref *r)
{
	vpop();
	fprintf(F_DEBUG, "TODO: pop func ret\n");
}

void impl_undefined(void)
{
	fprintf(F_DEBUG, "TODO: undefined instruction\n");
}

int impl_frame_ptr_to_reg(int nframes)
{
	fprintf(F_DEBUG, "TODO: builtin frame address\n");
}
