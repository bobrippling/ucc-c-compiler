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
#include "../../util/platform.h"

#define TODO() printf("// TODO: %s\n", __func__)

/*
 * This entire stack-output idea was inspired by tinycc, and improved slightly,
 *
 * e.g.
 *
 * f(a){return a + 1;}
 * is slightly better optimised, etc
 */

struct vstack vstack[1024];
struct vstack *vtop = NULL;

void vpush(void)
{
	UCC_ASSERT(vtop < ((vstack + sizeof(vstack)/sizeof(*vstack)) - 1),
			"vstack overflow");

	if(!vtop){
		vtop = vstack;
	}else{
		vtop++;
		memset(vtop, 0, sizeof *vtop);
	}
}

void vpop(void)
{
	UCC_ASSERT(vtop != ((vstack + sizeof(vstack)/sizeof(*vstack)) - 1),
			"vstack underflow");

	if(vtop == vstack)
		vtop = NULL;
	else
		vtop--;
}

void vswap(void)
{
	struct vstack tmp;

	memcpy(&tmp, vtop, sizeof tmp);
	memcpy(vtop, &vtop[-1], sizeof tmp);
	memcpy(&vtop[-1], &tmp, sizeof tmp);
}

int vfreereg(void)
{
	struct vstack *it, *first;
	int used[N_REGS];
	int i;

	memset(used, 0, sizeof used);
	first = NULL;

	for(it = vstack; it <= vtop; it++){
		if(it->type == REG){
			first = it;
			used[it->bits.reg] = 1;
		}
	}

	for(i = 0; i < N_REGS; i++)
		if(!used[i])
			return i;

	/* no free regs, move `first` to the stack and claim its reg */
	{
		struct vstack store = {
			.type = STACK,
			.d = first->d,
			.bits.off_from_bp = 23 /* TODO */
		};

		impl_store(first->bits.reg, &store);

		memcpy(first, &store, sizeof store);

		return first->bits.reg;
	}
}

void v_to_reg(struct vstack *conv)
{
	int reg;

	if(conv->type == REG)
		return;

	reg = vfreereg();

	impl_load(conv, reg);

	conv->type = REG;
	conv->bits.reg = reg;
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

	reg = vfreereg();
	impl_load(val, reg);
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
			vtop->bits.off_from_bp = -s->offset;
			break;

		case sym_arg:
			vtop->type = STACK;
			vtop->bits.off_from_bp = s->offset + 2 * platform_word_size();
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
	out_op_unary(op_deref, s->decl);
}

void out_op(enum op_type op, decl *d)
{
	int r;

	/* vtop[-1] must be a reg - try to swap first */
	if(vtop[-1].type != REG){
		if(vtop->type == REG)
			vswap();
		else
			v_to_reg(&vtop[-1]);
	}

	r = impl_op(op);

	vpop();
	vtop->type = REG;
	vtop->bits.reg = r;
	vtop->d = d;
}

void out_op_unary(enum op_type op, decl *d)
{
	(void)op;
	(void)d;
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
