#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "../../util/util.h"
#include "vstack.h"
#include "impl.h"
#include "../cc1.h"

static void vm_show_v(struct vstack *vs)
{
#define BP_TO_STR(bp) bp < 0 ? "-" : "", abs(bp)
	switch(vs->type){
		case CONST:
			fprintf(cc1_out, "val %d\n", vs->bits.val);
			break;
		case REG:
			fprintf(cc1_out, "reg %d\n", vs->bits.reg);
			break;
		case STACK_SAVE:
			fprintf(cc1_out, "stack save %s0x%x\n", BP_TO_STR(vs->bits.off_from_bp));
			break;
		case STACK:
			fprintf(cc1_out, "&stack %s0x%x\n", BP_TO_STR(vs->bits.off_from_bp));
			break;
		case FLAG:
			fprintf(cc1_out, "flag %d\n", vs->bits.flag);
			break;
		case LBL:
			fprintf(cc1_out, "&lbl %s + %ld\n",
					vs->bits.lbl.str, vs->bits.lbl.offset);
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
	fprintf(cc1_out, "store:\n\t");
	vm_show_v(from);
	fprintf(cc1_out, "to:\n\t");
	vm_show_v(to);
}

void impl_load(struct vstack *from, int reg)
{
	fprintf(cc1_out, "store:\n\t");
	vm_show_v(from);
	fprintf(cc1_out, "... in reg %d\n", reg);
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

	if(vtop->type == LBL){
		if(vtop->bits.lbl.offset){
			snprintf(buf, sizeof buf, "%s + %ld",
					vtop->bits.lbl.str, vtop->bits.lbl.offset);
		}
		return vtop->bits.lbl.str;
	}

	v_to_reg(vtop);
	snprintf(buf, sizeof buf, "*reg_%d", vtop->bits.reg);

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

void impl_call(int nargs, decl *d_ret, decl *d_func)
{
	(void)d_ret;
	(void)d_func;
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
	fprintf(cc1_out, "label %s\n", lbl);
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

void impl_undefined(void)
{
	fprintf(cc1_out, "<undefined trap>\n");
}

int impl_frame_ptr_to_reg(int nframes)
{
	int r = v_unused_reg(1);
	fprintf(cc1_out, "frame ptr %d to reg %d\n", nframes, r);
	return r;
}
