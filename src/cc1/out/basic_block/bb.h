#ifndef BASIC_BLOCK_H
#define BASIC_BLOCK_H

#define OUT_VPHI_SZ 64 /* sizeof(struct vstack) */

#include "tdefs.h"

basic_blk *bb_new(char *label);

basic_blk *bb_phi_next(basic_blk_phi *);

void bb_split(
		basic_blk *exp,
		basic_blk *b_true, basic_blk *b_false,
		basic_blk_phi **pphi);

#define bb_split_new(exp, pbt, pbf, pphi, lbl) \
	*pbt = bb_new(lbl "_true"),                  \
	*pbf = bb_new(lbl "_false"),                 \
	bb_split((exp), *pbt, *pbf, pphi)

void bb_phi_incoming(basic_blk_phi *to, basic_blk *from);
void bb_link_forward(basic_blk *from, basic_blk *to);

void bb_pop_to(basic_blk *from, basic_blk *to);

void bb_terminates(basic_blk *);

#endif
