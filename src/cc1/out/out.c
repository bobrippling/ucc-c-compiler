#include <stdio.h>
#include <stdarg.h>

#include "../../util/util.h"
#include "../data_structs.h"

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
	(void)d;
	tmp_asm("pushq %ld", iv->val);
}

void out_push_i(decl *d, int i)
{
	(void)d;
	tmp_asm("pushq %d", i);
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
	tmp_asm("// TODO: normalise (%rsp)");
}


void out_push_sym_addr(sym *s)
{
	(void)s;
	tmp_asm("// TODO: sym addr");
}

void out_push_sym(sym *s)
{
	(void)s;
}

void out_store(decl *d)
{
	(void)d;
}


void out_op(enum op_type op, decl *d)
{
	(void)op;
	(void)d;
}

void out_op_unary(enum op_type op, decl *d)
{
	(void)op;
	(void)d;
}


void out_cast(decl *from, decl *to)
{
	(void)from;
	(void)to;
}


void out_call(void)
{
}

void out_call_fin(int i)
{
	(void)i;
}

void out_jmp(void)
{
}

void out_jz(const char *lbl)
{
	(void)lbl;
}

void out_jnz(const char *lbl)
{
	(void)lbl;
}


void out_func_prologue(int offset)
{
	(void)offset;
}

void out_func_epilogue(void)
{
}

void out_label(const char *lbl)
{
	(void)lbl;
}


void out_comment(const char *fmt, ...)
{
	(void)fmt;
}


void out_flush(void)
{
}
