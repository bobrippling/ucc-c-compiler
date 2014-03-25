#ifndef BLK_H
#define BLK_H

#include "forwards.h"

struct out_blk
{
	out_ctx *octx;
	const char *desc;
	out_val *phi_val;

	struct
	{
		enum { BLK_NEXT_BLOCK, BLK_NEXT_EXPR } type;
		union
		{
			out_blk *blk;
			out_val *exp;
		} bits;
	} next;
};

#endif
