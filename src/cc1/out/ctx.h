#ifndef CTX_H
#define CTX_H

struct out_ctx
{
	out_blk *current_blk;

	out_val *val_head, *val_tail;

	int stack_sz, stack_local_offset, stack_variadic_offset;

	/* we won't reserve it more than 255 times
	 * XXX: FIXME: hardcoded 20
	 */
	unsigned char reserved_regs[20];
};

#endif
