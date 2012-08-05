#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "../../util/util.h"
#include "../../util/alloc.h"
#include "../data_structs.h"
#include "out.h"
#include "vstack.h"
#include "x86_64.h"

#define TODO() printf("// TODO: %s\n", __func__)

struct vstack vstack[1024];
struct vstack *vtop = vstack;

void vpush(void)
{
	UCC_ASSERT(vtop < ((vstack + sizeof(vstack)/sizeof(*vstack)) - 1),
			"vstack overflow");

	vtop++;
	memset(vtop, 0, sizeof *vtop);
}

void vpop(void)
{
	UCC_ASSERT(vtop != ((vstack + sizeof(vstack)/sizeof(*vstack)) - 1),
			"vstack underflow");

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

void out_store()
{
	struct vstack *store, *val;
	int reg;

	store = &vtop[0];
	val   = &vtop[-1];

	reg = impl_load(val);
	impl_store(reg, store);

	/* pop the store, but not the value */
	vpop();
}

void out_normalise(void)
{
	TODO();
}

void out_push_sym_addr(sym *s)
{
	vpush();

	switch(s->type){
		case sym_local:
			vtop->type = STACK;
			vtop->bits.off_from_bp = s->offset;
			break;

		case sym_arg:
			vtop->type = STACK;
			vtop->bits.off_from_bp = -s->offset;
			break;

		case sym_global:
			vtop->type = LBL;
			vtop->bits.lbl = s->decl->spel;
			break;
	}

	vtop->d = s->decl;
}

void out_push_sym(sym *s)
{
	out_push_sym_addr(s);
	out_op(op_deref, s->decl);
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
	impl_comment(fmt, l);
	va_end(l);
}
