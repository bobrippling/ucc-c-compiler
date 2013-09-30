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

#ifdef BLOCK_DBG
#  define BLOCK_SHOW(blk, s, ...) \
            fprintf(f, "# --- %p " s "\n", (void *)blk, __VA_ARGS__)

#else
#  define BLOCK_SHOW(blk, s, ...)
#endif

static void bb_flush_phi(struct basic_blk_phi *phi, FILE *f)
{
	BLOCK_SHOW(phi, "phi block, next=%p, inc:", (void *)phi->next);

#ifdef BLOCK_DBG
	struct basic_blk **i;
	for(i = phi->incoming; i && *i; i++)
		fprintf(f, "#  -> %p\n", (void *)*i);
#endif

	bb_flush(phi->next, f);
}

static void bb_flush_fork(struct basic_blk_fork *fork, FILE *f)
{
	BLOCK_SHOW(fork, "fork block, true=%p, false=%p, phi=%p",
			(void *)fork->btrue, (void *)fork->bfalse, (void *)fork->phi);

	if(fork->on_const){
		/*impl_jmp(f, (fork->bits.const_t ? fork->btrue : fork->bfalse)->lbl);*/
		bb_flush(fork->bits.const_t ? fork->btrue : fork->bfalse, f);
	}else{
		impl_jflag(f, &fork->bits.flag, fork->btrue->lbl, fork->bfalse->lbl);

		bb_flush(fork->btrue, f);
		bb_flush(fork->bfalse, f);
	}
	bb_flush_phi(fork->phi, f);
}

void bb_flush(basic_blk *head, FILE *f)
{
	if(!head)
		return;

	switch(head->type){
		case bb_norm:
		{
			char **i;

			BLOCK_SHOW(head, "basic block, next=%p", (void *)head->next);

			bb_lbl(f, head->lbl);

			/*impl_lbl(f, head->lbl);*/
			for(i = head->insns; i && *i; i++)
				bb_cmd(f, "%s", *i);
			bb_flush(head->next, f);
			break;
		}

		case bb_fork:
			bb_flush_fork((struct basic_blk_fork *)head, f);
			break;

		case bb_phi:
			/* reached a phi from a normal basic block,
			 * don't flush here, the split code logic takes care
			 */
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

