#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "../../util/util.h"
#include "../../util/alloc.h"
#include "../data_structs.h"
#include "out.h"

#define SNPRINTF(s, n, ...) \
		UCC_ASSERT(snprintf(s, n, __VA_ARGS__) != n, "snprintf buffer too small")

#define TODO() tmp_asm("// TODO: %s", __func__)

#include "out_lbl.c"
#include "x86_64.c"

struct vstack
{
	enum vstore
	{
		/* current store */
		CONST,
		REG,
		STACK,
		FLAG,
		LBL,
	} type;

	decl *d;
	union
	{
		int val;
		int reg;
		int off_from_bp;
		/* nothing for flag */
		char *lbl;
	} bits;
};

static struct vstack vstack[1024];
static struct vstack *vtop = vstack;

static void tmp_asmv(const char *fmt, va_list l, const char *pre)
{
	printf("\t%s", pre ? pre : "");
	vprintf(fmt, l);
	printf("\n");
}

static void tmp_asm(const char *fmt, ...)
{
	va_list l;

	va_start(l, fmt);
	tmp_asmv(fmt, l, NULL);
	va_end(l);
}

static void reg_free(int r)
{
	(void)r;
}

static int reg_req(void)
{
	return 0;
}

static void reg_save(void)
{
}

static void vpush(void)
{
	if(vtop >= ((vstack + sizeof(vstack)/sizeof(*vstack)) - 1))
		abort();
	vtop++;
	memset(vtop, 0, sizeof *vtop);
}

static void vpop(void)
{
	if(vtop == vstack)
		abort();
	vtop--;
}

void out_pop(void)
{
	vpop();
}

void out_push_iv(decl *d, intval *iv)
{
	vpush();
	vtop->type = CONST;
	vtop->bits.val = iv->val; /* TODO: unsigned */
	vtop->d = d;
}

void out_push_i(decl *d, int i)
{
	intval iv = {
		.val = i,
		.suffix = 0
	};

	out_push_iv(d, &iv);
}

void out_push_lbl(char *s)
{
	vpush();
	vtop->type = LBL;
	vtop->bits.lbl = s;
}

void out_dup(void)
{
	vpush();
	memcpy(&vtop[0], &vtop[-1], sizeof *vtop);
}

void out_normalise(void)
{
	TODO();
}

void out_push_sym_addr(sym *s)
{
	vpush();
	vtop->type = STACK;
	vtop->d = s->decl;
	vtop->bits.off_from_bp = s->offset;
}

void out_push_sym(sym *s)
{
	(void)s;
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
	(void)lbl;
	TODO();
}

void out_label(const char *lbl)
{
	printf("%s:\n", lbl);
}

void out_comment(const char *fmt, ...)
{
	va_list l;

	va_start(l, fmt);
	tmp_asmv(fmt, l, "// ");
	va_end(l);
}
