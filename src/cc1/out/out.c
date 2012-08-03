#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "../../util/util.h"
#include "../../util/alloc.h"
#include "../data_structs.h"
#include "out.h"

#define SNPRINTF(s, n, ...) \
		UCC_ASSERT(snprintf(s, n, __VA_ARGS__) != n, "snprintf buffer too small")

#include "out_lbl.c"
#include "x86_64.c"

#define TODO() tmp_asm("// TODO: %s", __func__)

struct vstack
{
	enum
	{
		/* current store */
		CONST,
		REG,
		FLAG,
		LBL
	} type;

	char *comment;

	decl *d;
	union
	{
		int reg;
		char *lbl;
	} bits;
};

struct vstack vstack[1024];
struct vstack *vtop = vstack;

void tmp_asm(const char *fmt, ...)
{
	va_list l;

	printf("\t");
	va_start(l, fmt);
	vprintf(fmt, l);
	va_end(l);
	printf("\n");
}

void out_pop(void)
{
	tmp_asm("popq %rax");
}

void out_pop_func_ret(decl *d)
{
	(void)d;
	tmp_asm("popq %rax");
}


void out_push_iv(decl *d, intval *iv)
{
	vpush();
	vtop->
	tmp_asm("pushq %ld", iv->val);
}

void out_push_i(decl *d, int i)
{
	intval iv = {
		.val = i,
		.suffix = 0
	};

	out_push_iv(d, &iv);
}

void out_push_lbl(const char *s)
{
	tmp_asm("leaq %rax, %s", s);
	tmp_asm("pushq %rax", s);
}


void out_dup(void)
{
	tmp_asm("popq %rax");
	tmp_asm("pushq %rax");
	tmp_asm("pushq %rax");
}

void out_normalise(void)
{
	TODO();
}


void out_push_sym_addr(sym *s)
{
	(void)s;
	TODO();
}

void out_push_sym(sym *s)
{
	(void)s;
	TODO();
}

void out_store(decl *d)
{
	(void)d;
	TODO();
}

void out_op(enum op_type op, decl *d)
{
	(void)d;
	(void)op;
	TODO();
}

void out_op_unary(enum op_type op, decl *d)
{
	(void)d;
	(void)op;
	TODO();
}

void out_cast(decl *from, decl *to)
{
	(void)to;
	(void)from;
	TODO();
}

void out_call_start(void)
{
	TODO();
}

void out_call(void)
{
	TODO();
}

void out_call_fin(int i)
{
	(void)i;
	TODO();
}

void out_jmp(void)
{
	TODO();
}

void out_jz(const char *lbl)
{
	(void)lbl;
	TODO();
}

void out_jnz(const char *lbl)
{

}

void out_label(const char *lbl)
{
}

void out_comment(const char *fmt, ...)
{
	va_list l;
	char *cmt;

	va_start(l, fmt);
  cmt = ustrvprintf(fmt, l);
	va_end(l);

	free(vtop->comment);
	vtop->comment = cmt;
}
