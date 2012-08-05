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
		out_asm("subq %d, %%rsp", offset);
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
	(void)d;
	impl_load(vtop);
	vpop();
	out_asm("popq %%rax");
}

static const char *
reg_str(int reg)
{
	const char regs[] = "abc";
	const char *regpre, *regpost;
	static char regstr[8];

	asm_reg_name(NULL, &regpre, &regpost);

	snprintf(regstr, sizeof regstr, "%s%c%s", regpre, regs[reg], regpost);

	return regstr;
}

int
impl_load(struct vstack *from)
{
	int reg = 0;

	switch(from->type){
		case CONST:
			out_asm("mov_ %d, %%%s", from->bits.val, reg_str(reg));
			break;
		default:
			abort();
	}

	return reg;
}

void
impl_store(int reg, struct vstack *where)
{
	const char *regstr = reg_str(reg);

	switch(where->type){
		case STACK:
			out_asm("mov_ %%%s, -0x%x(%%rbp)", regstr, where->bits.off_from_bp);
			break;

		case LBL:
			out_asm("mov_ %%%s, %s(%%rip)", regstr, where->bits.lbl);
			break;

		default:
			abort();
	}
}
