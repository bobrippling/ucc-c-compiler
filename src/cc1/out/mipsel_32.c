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

#define TODO() ICE("TODO")

int impl_alloc_stack(int sz)
{
	TODO();
}

void impl_func_prologue(int stack_res, int nargs, int variadic)
{
	/* FIXME: very similar to x86_64::func_prologue - merge */

	out_asm("addiu $sp, $sp,-8"); /* space for saved ret */
	out_asm("sw    $fp, 4($sp)"); /* save ret */
	out_asm("move  $fp, $sp");    /* new frame */

	out_asm("addiu $sp, $sp,-%d", stack_res); /* etc */
}

void impl_func_epilogue(void)
{
	TODO();
}

void impl_load(struct vstack *from, int reg)
{
	TODO();
}

void impl_store(struct vstack *from, struct vstack *to)
{
	TODO();
}

void impl_reg_swp(struct vstack *a, struct vstack *b)
{
	TODO();
}

void impl_reg_cp(struct vstack *from, int r)
{
	if(from->type == REG && from->bits.reg == r) /* TODO: x86_64 merge */
		return;

	TODO();
}

void impl_op(enum op_type op)
{
	TODO();
}

void impl_deref()
{
	TODO();
}

void impl_op_unary(enum op_type op)
{
	TODO();
}

void impl_normalise(void)
{
	TODO();
}

void impl_cast(type_ref *from, type_ref *to)
{
	/* TODO FIXME: combine with code in x86_64 */
	int szfrom, szto;

	szfrom = asm_type_size(from);
	szto   = asm_type_size(to);

	if(szfrom != szto){
		if(szfrom < szto){
			TODO();
		}else{
			char buf[DECL_STATIC_BUFSIZ];

			out_comment("truncate cast from %s to %s, size %d -> %d",
					from ? decl_to_str_r(buf, from) : "",
					to ? decl_to_str(to) : "",
					szfrom, szto);
		}
	}
}

void impl_jmp()
{
	switch(vtop->type){
		case LBL:
			out_asm("j %s", vtop->bits.lbl.str);
			break;

		default:
			TODO();
	}

	ICE("invalid jmp target");
}

void impl_jcond(int true, const char *lbl)
{
	TODO();
}

void impl_call(const int nargs, type_ref *r_ret, type_ref *r_func)
{
	TODO();
}

void impl_pop_func_ret(type_ref *r)
{
	TODO();
}

void impl_undefined(void)
{
	TODO();
}

int impl_frame_ptr_to_reg(int nframes)
{
	TODO();
}
