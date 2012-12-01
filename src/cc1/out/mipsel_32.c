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

static int alloc_stack(int sz)
{
	TODO();
}

static void func_prologue(int stack_res, int nargs, int variadic)
{
	/* FIXME: very similar to x86_64::func_prologue - merge */

	out_asm("addiu $sp, $sp,-8"); /* space for saved ret */
	out_asm("sw    $fp, 4($sp)"); /* save ret */
	out_asm("move  $fp, $sp");    /* new frame */

	out_asm("addiu $sp, $sp,-%d", stack_res); /* etc */
}

static void func_epilogue(void)
{
	TODO();
}

static void load(struct vstack *from, int reg)
{
	TODO();
}

static void store(struct vstack *from, struct vstack *to)
{
	TODO();
}

static void reg_swp(struct vstack *a, struct vstack *b)
{
	TODO();
}

static void reg_cp(struct vstack *from, int r)
{
	if(from->type == REG && from->bits.reg == r) /* TODO: x86_64 merge */
		return;

	TODO();
}

static void op(enum op_type op)
{
	TODO();
}

static void deref()
{
	TODO();
}

static void op_unary(enum op_type op)
{
	TODO();
}

static void normalise(void)
{
	TODO();
}

static void cast(decl *from, decl *to)
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

static void jmp()
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

static void jcond(int true, const char *lbl)
{
	TODO();
}

static void call(const int nargs, decl *d_ret, decl *d_func)
{
	TODO();
}

static void undefined(void)
{
	TODO();
}

static int frame_ptr_to_reg(int nframes)
{
	TODO();
}

void impl_mipsel_32()
{
	N_REGS = 10;
	N_CALL_REGS = 4; /* FIXME ??? */
	REG_RET = 0; /* FIXME ??? */

	reserved_regs = umalloc(N_REGS * sizeof *reserved_regs);

#define INIT(fn) impl.fn = fn
	INIT(store);
	INIT(load);
	INIT(reg_cp);
	INIT(reg_swp);
	INIT(op);
	INIT(op_unary);
	INIT(deref);
	INIT(normalise);
	INIT(jmp);
	INIT(jcond);
	INIT(cast);
	INIT(call);
	INIT(alloc_stack);
	INIT(func_prologue);
	INIT(func_epilogue);
	INIT(undefined);
	INIT(frame_ptr_to_reg);
}
