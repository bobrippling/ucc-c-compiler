#ifndef VSTACK_H
#define VSTACK_H

struct vstack
{
	enum vstore
	{
		V_CONST_I, /* constant integer */
		V_REG, /* value in a register */
		V_LBL, /* value at a memory address */

		V_CONST_F, /* constant float */
		V_FLAG, /* cpu flag */

		V_REG_SAVE, /* value stored in memory,
		             * referenced by reg+offset */
	} type;
	int is_lval;

	type_ref *t;

	union
	{
		integral_t val_i;
		floating_t val_f;

		struct
		{
			struct vreg
			{
				unsigned short idx, is_float;
			} reg;
#define VREG_INIT(idx, fp) { idx, fp }
			long offset;
		} regoff;

		struct flag_opts
		{
			enum flag_cmp
			{
				flag_eq, flag_ne,
				flag_le, flag_lt,
				flag_ge, flag_gt,
				flag_overflow, flag_no_overflow
			} cmp;
			enum flag_mod
			{
				flag_mod_signed = 1 << 0,
				flag_mod_float  = 1 << 1 /* e.g. unordered/nan */
			} mods;
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
#define VSTACK_INIT(ty) { (ty), 0, NULL, { 0 } }

extern struct vstack *vtop;

void vpush(type_ref *) ucc_nonnull();
void vpop(void);
void vswap(void);

void v_clear(struct vstack *vp, type_ref *);
void v_clear(struct vstack *vp, type_ref *) ucc_nonnull();
void v_set_reg(struct vstack *vp, const struct vreg *r);
void v_set_reg_i(struct vstack *vp, int idx);

void v_set_flag(
		struct vstack *vp,
		enum flag_cmp c,
		enum flag_mod mods);

void v_set_stack(
		struct vstack *vp, type_ref *ty,
		long off, int lval);

void v_cast(struct vstack *vp, type_ref *to);

void v_inv_cmp(struct flag_opts *);

void v_to_reg(struct vstack *conv);
void v_to_reg_out(struct vstack *conv, struct vreg *);
void v_to_reg_given(struct vstack *from, const struct vreg *);

void v_to_mem_given(struct vstack *, int stack_pos);
void v_to_mem(struct vstack *);
int  v_stack_sz(void);

void v_to_rvalue(struct vstack *);

enum vto
{
	TO_REG = 1 << 0,
	TO_MEM = 1 << 1, /* TODO: allow offset(%reg) */
	TO_CONST = 1 << 2,
};
void v_to(struct vstack *, enum vto);

int vreg_eq(const struct vreg *, const struct vreg *);

/* returns 0 on success, -1 if no regs free */
int  v_unused_reg(int stack_as_backup, int fp, struct vreg *);

void v_freeup_regp(struct vstack *);
void v_freeup_reg(const struct vreg *, int allowable_stack);
void v_freeup_regs(const struct vreg *, const struct vreg *);
void v_save_reg(struct vstack *vp);
/* if func_ty != NULL, don't save callee-save-regs */
void v_save_regs(int n_ignore, type_ref *func_ty);
void v_reserve_reg(const struct vreg *);
void v_unreserve_reg(const struct vreg *);

void v_store(struct vstack *val, struct vstack *store);

/* outputs stack-ptr instruction(s) */
unsigned v_alloc_stack(unsigned sz, const char *);
/* Will output instructions to align the stack to cc1_mstack_align
 * e.g. if a push is done manually */
unsigned v_alloc_stack_n(unsigned sz, const char *);
/* v_alloc_stack* returns the padded sz that was alloced */
void v_dealloc_stack(unsigned sz);
/* returns the amount added to the stack */
unsigned v_stack_align(unsigned const align, int do_mask);

void v_deref_decl(struct vstack *vp);

int impl_n_scratch_regs(void);
unsigned impl_n_call_regs(type_ref *);
int impl_ret_reg(void);

#endif
