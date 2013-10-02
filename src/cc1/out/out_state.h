#ifndef OUT_STATE_H
#define OUT_STATE_H

/* MAX() */
#include "../defs.h"

#define N_RESERVED_REGS \
	MAX(N_SCRATCH_REGS_I, N_SCRATCH_REGS_F)

struct out
{
	int stack_sz, stack_local_offset, stack_variadic_offset;

	/* we won't reserve it more than 255 times */
	unsigned char reserved_regs[N_RESERVED_REGS];
};

struct out *out_state_new(void);

#endif
