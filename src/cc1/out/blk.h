#ifndef BLK_H
#define BLK_H

#include "forwards.h"

struct out_blk
{
	out_ctx *octx;
	const char *desc;

	char *lbl;
	out_val *phi_val;

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
				struct
				{
					char *insn;
					out_blk *blk;
				} if_1, if_0;
			} cond;
		} bits;
	} next;
};

void blk_terminate_condjmp(
		out_ctx *, out_blk *,
		char *condinsn, out_blk *condto,
		char *uncondjmp, out_blk *uncondto);

void blk_terminate_jmp(out_blk *, char *jmpinsn);

#endif
