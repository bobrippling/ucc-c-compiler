#ifndef BBLOCK_DEFS_H
#define BBLOCK_DEFS_H

enum bb_type
{
	bb_norm, bb_fork, bb_phi
};

/* ABI between basic_blk{,_phi} */
#define BB_DEFS                         \
	enum bb_type type;                    \
                                        \
	struct basic_blk *next;               \
                                        \
	/* reserved regs, stack size, etc */  \
	struct out *ostate


struct basic_blk
{
	BB_DEFS;

	char *lbl;
	char **insns;

	/* vstack for this block */
	struct vstack *vbuf;
	struct vstack *vtop;

	int flushed;
};

struct basic_blk_fork
{
	enum bb_type type;

	struct basic_blk *exp, *btrue, *bfalse;
	struct basic_blk_phi *phi;

	int on_const;
	union
	{
		struct vstack_flag flag;
		int const_t;
	} bits;
};
/*
    fork on exp
    /   \
   /     \
btrue   bfalse
   \     /
    \   /
     phi
*/

struct basic_blk_phi
{
	BB_DEFS;

	struct basic_blk **incoming;
};

#undef BB_DEFS
#define PHI_TO_NORMAL(phi) (basic_blk *)phi

#endif
