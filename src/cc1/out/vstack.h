#ifndef VSTACK_H
#define VSTACK_H

struct vstack
{
	enum vstore
	{
		CONST,          /* vtop is a constant value */
		REG,            /* vtop is in a register */
		STACK,          /* vtop pointer onto stack */
		STACK_SAVE,     /* saved register/flag */
		FLAG,           /* vtop is a cpu flag */
		LBL,            /* vtop is a pointer to label */
	} type;

	type_ref *t;

	/*
	 * TODO: offset to optimise multiple adds
	 * i.e. movq x+63(%rip), %rax
	 * instead of
	 *      leaq x(%rip), %rax
	 *      addq 60, %rax
	 *      addq  3, %rax
	 *      movq (%rax), %rax
	 */

	union
	{
		unsigned long val;
		int reg;
		int off_from_bp;
		struct flag_opts
		{
			enum flag_cmp
			{
				flag_eq, flag_ne,
				flag_le, flag_lt,
				flag_ge, flag_gt,
			} cmp;
			int is_signed;
		} flag;
		struct
		{
			const char *str;
			int pic;
		} lbl;
	} bits;
};
#define VSTACK_INIT(ty) { (ty), NULL, { 0 } }

extern struct vstack *vtop;

void vpop(void);
void vswap(void);

void v_clear(struct vstack *vp, type_ref *);

void v_to_reg_const(struct vstack *vp);

void v_inv_cmp(struct vstack *vp);

int  v_to_reg(struct vstack *conv);
void v_to_reg2(struct vstack *from, int reg);


int  v_unused_reg(int stack_as_backup);
void v_freeup_regp(struct vstack *);
void v_freeup_reg(int r, int allowable_stack);
void v_freeup_regs(int a, int b);
void v_save_reg(struct vstack *vp);
/* if func_ty != NULL, don't save callee-save-regs */
void v_save_regs(int n_ignore, type_ref *func_ty);
void v_reserve_reg(const int r);
void v_unreserve_reg(const int r);

void v_deref_decl(struct vstack *vp);

const char *v_val_str(struct vstack *vp);

#endif
