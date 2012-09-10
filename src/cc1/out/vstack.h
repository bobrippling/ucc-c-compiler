#ifndef VSTACK_H
#define VSTACK_H

struct vstack
{
	enum vstore
	{
		CONST,          /* vtop is a constant value/memory address */
		REG,            /* vtop is in a register/register pointer */
		STACK,          /* vtop is in the stack/pointer onto stack */
		FLAG,           /* vtop is a cpu flag/invalid */
		LBL,            /* vtop is a label/pointer to label */
	} type;

	int is_addrof;
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

extern struct vstack *vtop, vstack[];

void vpop(void);
void vswap(void);
void vdup(void);

void v_clear(struct vstack *vp, decl *);
void vtop_clear(decl *);

void vtop2_prepare_op(void);
void v_prepare_op(struct vstack *vp);

void v_inv_cmp(struct vstack *vp);

int  v_to_reg(struct vstack *conv);
void v_to_mem(struct vstack *conv);

int  v_unused_reg(int stack_as_backup);
void v_freeup_regp(struct vstack *);
void v_freeup_reg(int r, int allowable_stack);
void v_save_reg(struct vstack *vp);

void v_deref_decl(struct vstack *vp);

#endif
