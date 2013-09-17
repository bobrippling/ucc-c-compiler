#ifndef BASIC_BLOCK_H
#define BASIC_BLOCK_H

#define OUT_VPHI_SZ 64 /* sizeof(struct vstack) */

typedef struct basic_blk basic_blk;
#define BASIC_BLOCK_DEFINED

basic_blk *bb_new(void);

basic_blk *bb_split(
		basic_blk *exp,
		basic_blk *pb_true,
		basic_blk *pb_false);

basic_blk *bb_merge(basic_blk *, basic_blk *);

basic_blk *bb_current(void);

#  if 0
void out_phi_pop_to(void *); /* put the current value into a phi-save area */
void out_phi_join(void *);   /* join vtop and the phi-save area */

void out_jmp(void);
void out_jtrue( const char *);
void out_label(const char *);
#  endif

#endif
