#ifndef CTX_H
#define CTX_H

#include "vstack_t.h"

typedef struct out_val_list out_val_list;

struct out_ctx
{
	out_blk *first_blk, *second_blk, *current_blk, *epilogue_blk;
	out_blk *last_used_blk; /* for appending debug labels */
	out_blk **mustgen; /* goto *lbl; where lbl is otherwise unreachable */

	void *userctx;

	struct out_val_list
	{
		out_val val;
		struct out_val_list *next, *prev;
	} *val_head, *val_tail;

	type *current_fnty;

	unsigned long nblks; /* used for unique label gen. */

	/* fixed: */
	v_stackt stack_sz_initial;
	v_stackt stack_local_offset, stack_variadic_offset;
	/* vary: */
	v_stackt cur_stack_sz;
	v_stackt stack_n_alloc; /* just the alloc_n() part */
	unsigned max_align;

	unsigned check_flags : 1; /* decay flags? */
	unsigned in_prologue : 1, used_stack : 1;

	/* we won't reserve it more than 255 times */
	unsigned char *reserved_regs;

	/* mark callee save regs, to preserve at prologue */
	struct vreg *used_callee_saved;

	struct
	{
		struct out_dbg_filelist *file_head;

		where where;
		int last_file, last_line;
	} dbg;
};

#endif
