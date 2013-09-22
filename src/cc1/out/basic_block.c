#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "../../util/where.h"
#include "../../util/alloc.h"
#include "../../util/assert.h"
#include "../../util/dynarray.h"

#include "../data_structs.h"
#include "../type_ref.h"

#include "vstack.h"
#include "basic_block.h"
#include "basic_block_int.h"

struct basic_blk
{
	enum bb_type
	{
		bb_norm, bb_fork, bb_phi
	} type;

	struct basic_blk *next;
	char **insns;
};

struct basic_blk_fork
{
	enum bb_type type;
	struct basic_blk *btrue, *bfalse;
};

struct basic_blk_phi
{
	enum bb_type type;
	struct basic_blk *next;
	struct basic_blk **incoming;
};

#define PHI_TO_NORMAL(phi) (basic_blk *)phi


basic_blk *bb_new(void)
{
	basic_blk *bb = umalloc(sizeof *bb);
	bb->type = bb_norm;
	return bb;
}

basic_blk_phi *bb_new_phi(void)
{
	basic_blk_phi *phi = umalloc(sizeof *phi);
	phi->type = bb_phi;
	return phi;
}

basic_blk *bb_phi_next(basic_blk_phi *phi)
{
	if(!phi->next)
		phi->next = bb_new();
	return phi->next;
}

void bb_addv(basic_blk *bb, const char *fmt, va_list l)
{
	char *insn = ustrvprintf(fmt, l);

	UCC_ASSERT(bb->type == bb_norm, "insn on non-normal block");

	dynarray_add(&bb->insns, insn);
}

void bb_commentv(basic_blk *bb, const char *fmt, va_list l)
{
	char **plast;
	size_t len;

	bb_addv(bb, fmt, l);

	plast = &bb->insns[dynarray_count(bb->insns)-1];

	len = strlen(*plast);
	*plast = urealloc1(*plast, len + 3); /* \0, "# " */
	memmove(*plast + 2, *plast, len + 2);

	memcpy(*plast, "# ", 2);
}

void bb_add(basic_blk *bb, const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	bb_addv(bb, fmt, l);
	va_end(l);
}

void bb_pop_to(basic_blk *from, basic_blk *to)
{
	(void)from;
	ICW("TODO");
	bb_add(to, "TODO: pop_to");
}

void bb_terminates(basic_blk *bb)
{
	(void)bb;
}

void bb_split(
		basic_blk *exp,
		basic_blk *b_true,
		basic_blk *b_false)
{
	struct basic_blk_fork *fork = umalloc(sizeof *fork);

	UCC_ASSERT(!exp->next, "can't split - already have a .next");

	exp->next = (basic_blk *)fork;

	fork->type = bb_fork;
	fork->btrue = b_true, fork->bfalse = b_false;
}

void bb_link_forward(basic_blk *from, basic_blk *to)
{
	UCC_ASSERT(!from->next, "forward link present");
	from->next = to;
}

void bb_phi_incoming(basic_blk_phi *to, basic_blk *from)
{
	bb_link_forward(from, PHI_TO_NORMAL(to));

	dynarray_add(&to->incoming, from);
}

static void bb_flush_fork(struct basic_blk_fork *head, FILE *f)
{
	fprintf(f, "/* TODO: fork on ^ */\n");
	fprintf(f, "# true case:\n");
	bb_flush(head->btrue, f);
	fprintf(f, "# false case:\n");
	bb_flush(head->bfalse, f);
	fprintf(f, "# fork end\n");
}

void bb_flush(basic_blk *head, FILE *f)
{
	if(!head)
		return;

	switch(head->type){
		case bb_norm:
		{
			char **i;
			for(i = head->insns; i && *i; i++)
				fprintf(f, "%s\n", *i);
			bb_flush(head->next, f);
			break;
		}

		case bb_fork:
			bb_flush_fork((struct basic_blk_fork *)head, f);
			break;

		case bb_phi:
			fprintf(f, "/* TODO: phi node */\n", head->type);
			bb_flush(((struct basic_blk_phi *)head)->next, f);
			break;
	}
}

#if 0
void out_jmp(basic_blk *b_from)
{
	if(vtop > vstack){
		/* flush the stack-val we need to generate before the jump */
		v_flush_volatile(vtop - 1);
	}

	impl_jmp();
	vpop();
}

void out_phi_pop_to(basic_blk *b_from, void *vvphi)
{
	struct vstack *const vphi = vvphi;

	/* put the current value into the phi-save area */
	memcpy_safe(vphi, vtop);

	if(vphi->type == V_REG)
		v_reserve_reg(&vphi->bits.regoff.reg); /* XXX: watch me */

	out_pop(b_from);
}

void out_phi_join(basic_blk *b_from, void *vvphi)
{
	struct vstack *const vphi = vvphi;

	if(vphi->type == V_REG)
		v_unreserve_reg(&vphi->bits.regoff.reg); /* XXX: voila */

	/* join vtop and the current phi-save area */
	v_to_reg(vtop);
	v_to_reg(vphi);

	if(!vreg_eq(&vtop->bits.regoff.reg, &vphi->bits.regoff.reg)){
		/* _must_ match vphi, since it's already been generated */
		impl_reg_cp(vtop, &vphi->bits.regoff.reg);
		memcpy_safe(vtop, vphi);
	}
}
#endif
