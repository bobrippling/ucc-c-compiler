#ifndef OUT_H
#define OUT_H

#include "basic_block/tdefs.h"

void out_pop(basic_blk *);
void out_pop_func_ret(basic_blk *, type_ref *) ucc_nonnull((1));

void out_push_num(basic_blk *, type_ref *t, const numeric *n) ucc_nonnull((1));
void out_push_l(basic_blk *, type_ref *, long) ucc_nonnull((1));
void out_push_zero(basic_blk *, type_ref *) ucc_nonnull((1));
void out_push_lbl(basic_blk *, const char *s, int pic);
void out_push_noop(basic_blk *);

void out_dup(basic_blk *); /* duplicate top of stack */
void out_pulltop(basic_blk *, int i); /* pull the nth stack entry to the top */
void out_normalise(basic_blk *); /* change to 0 or 1 */

void out_push_sym(basic_blk *, sym *);
void out_push_sym_val(basic_blk *, sym *);
void out_store(basic_blk *); /* store stack[1] into *stack[0] */

void out_set_bitfield(basic_blk *, unsigned off, unsigned nbits);

void out_op(basic_blk *, enum op_type); /* binary ops and comparisons */
void out_op_unary(basic_blk *, enum op_type); /* unary ops */
void out_deref(basic_blk *);
void out_swap(basic_blk *);
void out_flush_volatile(basic_blk *);

void out_cast(basic_blk *, type_ref *to) ucc_nonnull((1));
void out_change_type(basic_blk *, type_ref *) ucc_nonnull((1));

void out_call(basic_blk *,
		int nargs, type_ref *rt, type_ref *f) ucc_nonnull((3, 4));

basic_blk *out_func_prologue(
		char *spel,
		type_ref *rf,
		int stack_res, int nargs, int variadic,
		int arg_offsets[]);

void out_func_epilogue(basic_blk *, type_ref *);

void out_comment(basic_blk *, const char *, ...) ucc_printflike(2, 3);
#ifdef ASM_H
void out_comment_sec(enum section_type, const char *, ...)
	ucc_printflike(2, 3);
#endif

void out_assert_vtop_null(basic_blk *);
void out_dump(basic_blk *);
void out_undefined(basic_blk *);
void out_push_overflow(basic_blk *);

void out_push_frame_ptr(basic_blk *, int nframes);
void out_push_reg_save_ptr(basic_blk *);
void out_push_nan(basic_blk *, type_ref *ty);

int out_vcount(basic_blk *);

#endif
