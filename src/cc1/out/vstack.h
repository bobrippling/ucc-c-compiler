#ifndef VSTACK_H
#define VSTACK_H

struct vstack
{
	enum vstore
	{
		/* current store */
		CONST,
		REG,
		STACK,
		FLAG,
		LBL,
	} type;

	decl *d;
	union
	{
		int val;
		int reg;
		int off_from_bp;
		/* nothing for flag */
		char *lbl;
	} bits;
};

extern struct vstack *vtop;

void vpop(void);

#endif
