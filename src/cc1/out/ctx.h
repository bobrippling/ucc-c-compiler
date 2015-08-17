#ifndef CTX_H
#define CTX_H

#include "vstack_t.h"

typedef struct out_val_list out_val_list;

struct out_ctx
{
	/* entry handles arg spill, etc.
	 * prologue handles variadic spill, jumps, etc,
	 * post_prologue is where user code goes
	 */
	out_blk *entry_blk, *prologue_prejoin_blk, *prologue_postjoin_blk;
	out_blk *current_blk;
	out_blk *epilogue_blk;

	out_blk *last_used_blk; /* for appending debug labels */
	out_blk **mustgen; /* goto *lbl; where lbl is otherwise unreachable */
	struct out_dbg_lbl **pending_lbls; /* debug labels */

	/* memory management */
	out_blk *mem_blk_head;

	void *userctx;

	struct out_val_list
	{
		out_val val;
		struct out_val_list *next, *prev;
	} *val_head, *val_tail;
	const out_val *current_stret;

	type *current_fnty;

	unsigned long nblks; /* used for unique label gen. */

	/* fixed: */
	v_stackt stack_variadic_offset;
	v_stackt initial_stack_sz;
	/* vary: */
	v_stackt cur_stack_sz;
	v_stackt max_stack_sz;
	v_stackt stack_n_alloc; /* just the alloc_n() part */
	v_stackt stack_callspace; /* space used by extra call arguments */
	unsigned max_align;

	unsigned check_flags : 1; /* decay flags? */
	unsigned in_prologue : 1, used_stack : 1;
	unsigned alloca_count;

	/* we won't reserve it more than 255 times */
	unsigned char *reserved_regs;

	/* mark callee save regs, to preserve at prologue */
	struct vreg *used_callee_saved;

	struct
	{
		struct out_dbg_filelist *file_head;

		where where;
		unsigned last_file, last_line;
	} dbg;
};

#endif
