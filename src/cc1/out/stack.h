#ifndef OUT_STACK_H
#define OUT_STACK_H

#include "vstack_t.h"

/* emit an actual stack insn */
void v_stack_adj(out_ctx *octx, v_stackt amt, int sub);

/* set stack alignment for current function */
void v_stack_needalign(out_ctx *octx, unsigned align);

void v_stack_realign(out_ctx *octx, unsigned align, int force_mask);

ucc_wur v_stackt v_aalloc(out_ctx *, unsigned sz, unsigned align, const char *);
ucc_wur v_stackt v_aalloc_noop(out_ctx *, unsigned sz, unsigned align, const char *);


#if 0
unsigned v_alloc_stack2(out_ctx *octx,
		const unsigned sz_initial, int noop, const char *desc);

unsigned v_alloc_stack_n(out_ctx *octx, unsigned sz, const char *desc);

unsigned v_alloc_stack(out_ctx *octx, unsigned sz, const char *desc);
#endif


#endif
