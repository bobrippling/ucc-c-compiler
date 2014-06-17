#ifndef CTX_H
#define CTX_H

typedef struct out_val_list out_val_list;

struct out_ctx
{
	out_blk *first_blk, *current_blk, *epilogue_blk;
	out_blk *last_used_blk; /* for appending debug labels */
	out_blk **mustgen; /* goto *lbl; where lbl is otherwise unreachable */

	struct out_val_list
	{
		out_val val;
		struct out_val_list *next, *prev;
	} *val_head, *val_tail;

	type *current_fnty;

	unsigned long nblks; /* used for unique label gen. */

	/* fixed: */
	int stack_sz_initial;
	int stack_local_offset, stack_variadic_offset;
	/* vary: */
	int var_stack_sz, max_stack_sz;
	int stack_n_alloc; /* just the alloc_n() part */

	int in_prologue;

	/* we won't reserve it more than 255 times */
	unsigned char *reserved_regs;

	struct
	{
		struct out_dbg_filelist *file_head;

		where where;
		int last_file, last_line;
	} dbg;
};

#endif
