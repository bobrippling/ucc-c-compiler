#ifndef VSTACK_H
#define VSTACK_H

struct vstack
{
	enum vstore
	{
		CONST,          /* vtop is a constant value */
		REG,            /* vtop is in a register */
		STACK,          /* vtop is in the stack */
		STACK_ADDR,     /* vtop is a pointer to the stack */
		FLAG,           /* vtop is a cpu flag */
		LBL,            /* vtop is a label */
		LBL_ADDR,       /* vtop is a label address */
	} type;

	decl *d;

	union
	{
		int val;
		int reg;
		int off_from_bp;
		enum flag_cmp
		{
			flag_eq, flag_ne,
			flag_le, flag_lt,
			flag_ge, flag_gt,
		} flag;
		struct
		{
			char *str;
			int pic;
		} lbl;
	} bits;
};

extern struct vstack *vtop;

void vpop(void);
void vswap(void);

void v_clear(struct vstack *vp, decl *);
void vtop_clear(decl *);

void vtop2_prepare_op(void);
void v_prepare_op(struct vstack *vp);

int  v_to_reg(struct vstack *conv);

int  v_unused_reg(int stack_as_backup);
void v_freeup_regp(struct vstack *);
void v_freeup_reg(int r, int allowable_stack);

enum vstore v_deref_type(enum vstore store);

#endif
