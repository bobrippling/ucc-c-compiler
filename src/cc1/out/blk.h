#ifndef BLK_H
#define BLK_H

#include "forwards.h"

struct out_blk
{
	/* all blocks: */
	out_ctx *octx;
	const char *desc;
	char *lbl;
	char **insns;

	unsigned pred_count;
	out_blk *merge_preds[2];

#define BLK_IS_MERGE(b) (b->preds[0] && b->preds[1])

	unsigned flushed;
	enum
	{
		BLK_UNINIT,
		BLK_TERMINAL, /* a ->nothing */
		BLK_NEXT_BLOCK, /* a ->b */
		BLK_NEXT_EXPR, /* a ->`expr` */
		BLK_COND, /* a ? ->b : ->c */
	} type;

	/* phi terminators: */
	const out_val *phi_val;

	union
	{
		/* nothing */

		/* ucond jump */
		out_blk *next;

		/* expr jump */
		const out_val *exp;

		/* cond */
		struct blk_cond
		{
			char *insn;
			out_blk *if_0_blk, *if_1_blk;
		} cond;
	} bits;
};

out_blk *out_blk_new_lbl(out_ctx *, const char *lbl);

void blk_flushall(out_ctx *octx);

void blk_terminate_condjmp(
		out_ctx *octx, char *condinsn,
		out_blk *bpass, out_blk *bfail);

void blk_terminate_jmp(out_blk *, char *jmpinsn);
void blk_terminate_undef(out_blk *);

#endif
