#ifndef BASIC_BLOCK_H
#define BASIC_BLOCK_H

#define OUT_VPHI_SZ 64 /* sizeof(struct vstack) */

#include "basic_block.tdef.h"

basic_blk *bb_new(void);
basic_blk *bb_new_after(basic_blk *);

basic_blk_phi *bb_new_phi(void);
basic_blk *bb_phi_next(basic_blk_phi *);

void bb_split(basic_blk *exp, basic_blk *b_true, basic_blk *b_false);
#define bb_split_new(exp, pbt, pbf) \
	*pbt = bb_new(), *pbf = bb_new(), bb_split((exp), *pbt, *pbf)

void bb_phi_incoming(basic_blk_phi *to, basic_blk *from);
void bb_link_forward(basic_blk *from, basic_blk *to);

void bb_pop_to(basic_blk *from, basic_blk *to);

void bb_terminates(basic_blk *);

void bb_flush(basic_blk *head, FILE *);

#endif
