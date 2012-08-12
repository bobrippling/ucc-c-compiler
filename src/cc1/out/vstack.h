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
		enum op_type cmp;
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
void vtop_clear(void);
void vswap(void);
int  v_unused_reg(int stack_as_backup);

struct vstack *v_find_reg(int reg);
int  v_to_reg(struct vstack *conv);
void v_save_reg(struct vstack *);
void v_freeup_reg(int r, int allowable_stack);

#endif
