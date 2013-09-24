#include <stdio.h>
#include <stdarg.h>

#include "../../../util/where.h"

#include "../../data_structs.h"
#include "../../expr.h"
#include "../../decl.h"

#include "../vstack.h"
#include "defs.h"
#include "flush.h"
#include "io.h"

#include "../impl_flow.h"

static void bb_flush_fork(struct basic_blk_fork *fork, FILE *f)
{
	fprintf(f, "--- fork block ---\n");
	bb_flush(fork->btrue, f);
	bb_flush(fork->bfalse, f);
	bb_flush(PHI_TO_NORMAL(fork->phi), f);
}

void bb_flush(basic_blk *head, FILE *f)
{
	if(!head)
		return;

	switch(head->type){
		case bb_norm:
		{
			char **i;
			fprintf(f, "--- basic block ---\n");
			impl_lbl(f, head->lbl);
			for(i = head->insns; i && *i; i++)
				bb_cmd(f, "%s", *i);
			bb_flush(head->next, f);
			break;
		}

		case bb_fork:
			bb_flush_fork((struct basic_blk_fork *)head, f);
			break;

		case bb_phi:
			fprintf(f, "--- phi block ---\n");
			bb_flush(((struct basic_blk_phi *)head)->next, f);
			break;
	}
}

#if 0
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

