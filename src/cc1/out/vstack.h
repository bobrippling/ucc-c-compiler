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
		struct
		{
			char *str;
			int pic;
		} lbl;
	} bits;

	int is_addr; /* lea vs mov */
};

extern struct vstack *vtop;

void vpop(void);
void vswap(void);
int  vfreereg(void);
int  v_to_reg(struct vstack *conv);

#endif
