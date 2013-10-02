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

	union
	{
		intval_t val;
		int reg;
		long off_from_bp;
		struct flag_opts
		{
			enum flag_cmp
			{
				flag_eq, flag_ne,
				flag_le, flag_lt,
				flag_ge, flag_gt,
				flag_overflow, flag_no_overflow
			} cmp;
			int is_signed;
		} flag;
		struct
		{
			const char *str;
			long offset;
			int pic;
		} lbl;
	} bits;

	struct vbitfield
	{
		unsigned off, nbits;
	} bitfield; /* !!width iif bitfield */
};
#define VSTACK_INIT(ty) { (ty), NULL, { 0 } }

extern struct vstack *vtop;

void vpop(void);
void vswap(void);

void v_clear(struct vstack *vp, type_ref *) ucc_nonnull();

void v_cast(struct vstack *vp, type_ref *to);

void v_to_reg_const(struct vstack *vp);

void v_inv_cmp(struct vstack *vp);

int  v_to_reg(struct vstack *conv);
void v_to_reg2(struct vstack *from, int reg);


int  v_unused_reg(int stack_as_backup);
void v_freeup_regp(struct vstack *);
void v_freeup_reg(int r, int allowable_stack);
void v_freeup_regs(int a, int b);
void v_save_reg(struct vstack *vp);
void v_save_regs(void);
void v_reserve_reg(const int r);
void v_unreserve_reg(const int r);

void v_deref_decl(struct vstack *vp);

int impl_n_scratch_regs(void);
int impl_n_call_regs(void);
int impl_ret_reg(void);

#endif
