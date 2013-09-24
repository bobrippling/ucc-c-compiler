#ifndef BBLOCK_DEFS_H
#define BBLOCK_DEFS_H

struct basic_blk
{
	enum bb_type
	{
		bb_norm, bb_fork, bb_phi
	} type;

	struct basic_blk *next;

	char *lbl;
	char **insns;
	char *leave_cmd;
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
	enum bb_type type;
	struct basic_blk *next;
	struct basic_blk **incoming;
};

#define PHI_TO_NORMAL(phi) (basic_blk *)phi

#endif
