#ifndef OUT_STACK_H
#define OUT_STACK_H

#include "vstack_t.h"

/* emit an actual stack insn */
void v_stack_adj(out_ctx *octx, v_stackt amt, int sub);

/* set stack alignment for current function */
void v_stack_needalign(out_ctx *octx, unsigned align);

ucc_wur v_stackt v_aalloc(out_ctx *, unsigned sz, unsigned align, const char *);
void v_aalloc_noop(out_ctx *, unsigned sz, unsigned align, const char *);

#endif
