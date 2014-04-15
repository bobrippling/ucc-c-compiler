#ifndef BLK_H
#define BLK_H

#include "forwards.h"

struct out_blk
{
	out_ctx *octx;
	const char *desc;

	char **insns;

	char *lbl;
	out_val *phi_val;

	int flushed;

	struct
	{
		enum
		{
			BLK_NEXT_NONE,
			BLK_NEXT_BLOCK,
			BLK_NEXT_EXPR,
			BLK_NEXT_COND
		} type;
		union
		{
			/* nothing */

			/* ucond jump */
			out_blk *blk;

			/* expr jump */
			out_val *exp;

			/* cond */
			struct
			{
				char *insn;
				out_blk *if_0_blk, *if_1_blk;
			} cond;
		} bits;
	} next;
};

void blk_flushall(out_ctx *octx);

void blk_terminate_condjmp(
		out_ctx *octx, char *condinsn,
		out_blk *bpass, out_blk *bfail);

void blk_terminate_jmp(out_blk *, char *jmpinsn);

#endif
