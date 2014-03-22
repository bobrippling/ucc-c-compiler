#ifndef VSTACK_H
#define VSTACK_H

#include "forwards.h"

struct out_val
{
	enum out_val_store
	{
		V_CONST_I, /* constant integer */
		V_REG, /* value in a register */
		V_LBL, /* value at a memory address */

		V_CONST_F, /* constant float */
		V_FLAG, /* cpu flag */
	} type;
	unsigned retains;

	type *t;

	union
	{
		integral_t val_i;
		floating_t val_f;

		struct vreg
		{
			unsigned short idx, is_float;
		} reg;
#define VREG_INIT(idx, fp) { idx, fp }

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

out_val *outval_new(out_ctx *, out_val *from);

#if 0
void v_clear(out_val *vp, type *);
void v_clear(out_val *vp, type *) ucc_nonnull();
void v_set_reg(out_val *vp, const struct vreg *r);
void v_set_reg_i(out_val *vp, int idx);

void v_set_flag(
		out_val *vp,
		enum flag_cmp c,
		enum flag_mod mods);

void v_cast(out_val *vp, type *to);

void v_inv_cmp(struct flag_opts *, int invert_eq);

void v_to_reg(out_val *conv);
void v_to_reg_out(out_val *conv, struct vreg *);
void v_to_reg_given(out_val *from, const struct vreg *);

void v_to_mem_given(out_val *, int stack_pos);
void v_to_mem(out_val *);
int  v_stack_sz(void);

void v_to_rvalue(out_val *);

enum vto
{
	TO_REG = 1 << 0,
	TO_MEM = 1 << 1, /* TODO: allow offset(%reg) */
	TO_CONST = 1 << 2,
};
void v_to(out_val *, enum vto);

int vreg_eq(const struct vreg *, const struct vreg *);

/* returns 0 on success, -1 if no regs free */
int  v_unused_reg(int stack_as_backup, int fp, struct vreg *);

void v_freeup_regp(out_val *);
void v_freeup_reg(const struct vreg *, int allowable_stack);
void v_freeup_regs(const struct vreg *, const struct vreg *);
void v_save_reg(out_val *vp);
/* if func_ty != NULL, don't save callee-save-regs */
void v_save_regs(int n_ignore, type *func_ty);
void v_reserve_reg(const struct vreg *);
void v_unreserve_reg(const struct vreg *);

void v_store(out_val *val, out_val *store);

/* outputs stack-ptr instruction(s) */
unsigned v_alloc_stack(unsigned sz, const char *);
/* Will output instructions to align the stack to cc1_mstack_align
 * e.g. if a push is done manually */
unsigned v_alloc_stack_n(unsigned sz, const char *);
/* v_alloc_stack* returns the padded sz that was alloced */
void v_dealloc_stack(unsigned sz);
/* returns the amount added to the stack */
unsigned v_stack_align(unsigned const align, int do_mask);

void v_deref_decl(out_val *vp);

int impl_n_scratch_regs(void);
unsigned impl_n_call_regs(type *);
int impl_ret_reg(void);
#endif

#endif
