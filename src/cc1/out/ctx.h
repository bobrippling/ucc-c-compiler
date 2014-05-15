#ifndef CTX_H
#define CTX_H

typedef struct out_val_list out_val_list;

struct out_ctx
{
	out_blk *first_blk, *current_blk, *epilogue_blk;
	out_blk *last_used_blk; /* for appending debug labels */

	struct out_val_list
	{
		out_val val;
		struct out_val_list *next, *prev;
	} *val_head, *val_tail;

	unsigned long nblks; /* used for unique label gen. */

	int stack_sz, stack_local_offset, stack_variadic_offset;

	/* we won't reserve it more than 255 times
	 * XXX: FIXME: hardcoded 20
	 */
	unsigned char reserved_regs[20];
};

#endif
