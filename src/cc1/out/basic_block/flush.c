#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "../../../util/where.h"
#include "../../../util/dynarray.h"
#include "../../../util/alloc.h"

#include "../../data_structs.h"
#include "../../expr.h"
#include "../../decl.h"

#include "../vstack.h"
#include "defs.h"
#include "flush.h"
#include "io.h"

#include "../impl_flow.h"

/* TODO:
 *	UCC_ASSERT(out_vcount(bb_end) == 0, "non empty vstack after func gen");
 */

#ifdef BLOCK_DBG
#  define BLOCK_SHOW(blk, s, ...) \
            fprintf(f, "# --- %p " s "\n", (void *)blk, __VA_ARGS__)

#else
#  define BLOCK_SHOW(blk, s, ...)
#endif


static void phi_join(basic_blk **ents)
{
	const unsigned nents = dynarray_count(ents);
	struct vreg *regs = umalloc(sizeof *regs * (nents + 1));
	unsigned i;

	for(i = 0; ents[i]; i++){
		basic_blk *this = ents[i];

		/* haven't flushed the blocks yet - v_to_reg() them */
		v_to_reg_out(this, this->vtop, &regs[i]); /* FIXME: waiting for vtop branch */
	}

	free(regs);
}

static void bb_finalise_phi(struct basic_blk_phi *phi, FILE *f)
{
	BLOCK_SHOW(phi, "phi block, next=%p, inc:", (void *)phi->next);

#ifdef BLOCK_DBG
	struct basic_blk **i;
	for(i = phi->incoming; i && *i; i++)
		fprintf(f, "#  -> %p\n", (void *)*i);
#endif

	/* do the phi joining logic */
	if(phi->incoming)
		phi_join(phi->incoming);
}

static void bb_flush_fork(struct basic_blk_fork *fork, FILE *f)
{
	BLOCK_SHOW(fork, "fork block, true=%p, false=%p, phi=%p",
			(void *)fork->btrue, (void *)fork->bfalse, (void *)fork->phi);

	bb_finalise_phi(fork->phi, f);

	if(fork->on_const){
		/*impl_jmp(f, (fork->bits.const_t ? fork->btrue : fork->bfalse)->lbl);*/
		bb_flush(fork->bits.const_t ? fork->btrue : fork->bfalse, f);
	}else{
		impl_jflag(f, &fork->bits.flag, fork->btrue->lbl, fork->bfalse->lbl);

		bb_flush(fork->btrue, f);
		impl_jmp(f, fork->phi->next->lbl);
		bb_flush(fork->bfalse, f);
	}

	bb_flush(fork->phi->next, f);
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
			head->flushed = 1;

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
