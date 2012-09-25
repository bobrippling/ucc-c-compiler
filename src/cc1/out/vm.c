#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "../../util/util.h"
#include "../data_structs.h"
#include "vstack.h"
#include "x86_64.h"
#include "../cc1.h"

static void vm_show_v(struct vstack *vs)
{
	switch(vs->type){
		case CONST:
			fprintf(cc1_out, "%d\n", vs->bits.val);
			break;
		case REG:
			fprintf(cc1_out, "reg %d\n", vs->bits.reg);
			break;
		case STACK:
			fprintf(cc1_out, "on stack, at 0x%x\n", vs->bits.off_from_bp);
			break;
		case STACK_ADDR:
			fprintf(cc1_out, "&stack 0x%x\n", vs->bits.off_from_bp);
			break;
		case FLAG:
			fprintf(cc1_out, "flag %d\n", vs->bits.flag);
			break;
		case LBL:
			fprintf(cc1_out, "lbl %s\n", vs->bits.lbl.str);
			break;
		case LBL_ADDR:
			fprintf(cc1_out, "&lbl %s\n", vs->bits.lbl.str);
			break;
	}
}

void impl_comment(const char *fmt, va_list l)
{
	fprintf(cc1_out, "; ");
	vfprintf(cc1_out, fmt, l);
	fputc('\n', cc1_out);
}

void impl_store(struct vstack *from, struct vstack *to)
{
	fprintf(cc1_out, "store:\n");
	vm_show_v(to);
	fprintf(cc1_out, "in:\n");
	vm_show_v(from);
}

void impl_load(struct vstack *from, int reg)
{
	fprintf(cc1_out, "store:\n");
	vm_show_v(from);
	fprintf(cc1_out, "in reg %d\n", reg);
}

void impl_reg_cp(struct vstack *from, int r)
{
	fprintf(cc1_out, "reg %d -> %d\n",
			from->bits.reg, r);
}

void impl_op(enum op_type o)
{
	vm_show_v(vtop);
	vm_show_v(&vtop[-1]);
	fprintf(cc1_out, "binary %s\n", op_to_str(o));
	vpop();
}

void impl_op_unary(enum op_type o)
{
	vm_show_v(vtop);
	fprintf(cc1_out, "unary %s\n", op_to_str(o));
}

void impl_deref(void)
{
	vm_show_v(vtop);
	fprintf(cc1_out, "deref\n");
}

void impl_normalise(void)
{
	vm_show_v(vtop);
	fprintf(cc1_out, "normalise\n");
}

static const char *jmp_target(void)
{
	static char buf[16];

	if(vtop->type == LBL_ADDR)
		return vtop->bits.lbl.str;

	v_to_reg(vtop);
	snprintf(buf, sizeof buf, "*%d", vtop->bits.reg);

	return buf;
}

void impl_jmp(void)
{
	fprintf(cc1_out, "jmp %s\n", jmp_target());
}

void impl_jcond(int true, const char *lbl)
{
	fprintf(cc1_out, "jmp if %s%s\n", true ? "" : "! ", lbl);
}

void impl_cast(decl *from, decl *to)
{
	char buf[DECL_STATIC_BUFSIZ];

	vm_show_v(vtop);
	fprintf(cc1_out, "%s <-- %s\n",
			decl_to_str_r(buf, to), decl_to_str(from));
}

void impl_call(int nargs, int variadic, decl *d)
{
	(void)variadic;
	(void)d;

	fprintf(cc1_out, "%d args\n", nargs);
	while(nargs --> 0)
		vpop();
	fprintf(cc1_out, "call %s\n", jmp_target());
	vpop();
}

void impl_call_fin(int nargs)
{
	(void)nargs;
}

void impl_lbl(const char *lbl)
{
	fprintf(cc1_out, "%s:\n", lbl);
}

int impl_alloc_stack(int sz)
{
	fprintf(cc1_out, "alloc stack %d\n", sz);
	return -1;
}

void impl_func_prologue(int stack_res, int nargs, int variadic)
{
	(void)stack_res;
	(void)nargs;
	(void)variadic;

	fprintf(cc1_out, "func begin\n");
}

void impl_func_epilogue(void)
{
	fprintf(cc1_out, "func end\n");
}

void impl_pop_func_ret(decl *d)
{
	(void)d;
	fprintf(cc1_out, "func ret\n");
	vpop();
}
