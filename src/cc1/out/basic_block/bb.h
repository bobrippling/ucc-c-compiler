#ifndef BASIC_BLOCK_H
#define BASIC_BLOCK_H

#include "tdefs.h"

#define N_VSTACK 32

basic_blk *bb_new(struct out *, char *label);
basic_blk *bb_new_from(basic_blk *, char *label);

basic_blk *bb_phi_next(basic_blk_phi *);

void bb_split(
		basic_blk *exp,
		basic_blk *b_true, basic_blk *b_false,
		basic_blk_phi **pphi);

#define bb_split_new(exp, pbt, pbf, pphi, lbl) \
	*pbt = bb_new_from(exp, lbl "_true"),        \
	*pbf = bb_new_from(exp, lbl "_false"),       \
	bb_split((exp), *pbt, *pbf, pphi)

void bb_phi_incoming(basic_blk_phi *to, basic_blk *from);
void bb_link_forward(basic_blk *from, basic_blk *to);

void bb_pop_to(basic_blk *from, basic_blk *to);

void bb_terminates(basic_blk *);

unsigned bb_vcount(basic_blk *);

#endif
