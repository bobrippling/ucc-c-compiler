#ifndef VIRT_H
#define VIRT_H

#include "stack.h"

unsigned char *v_alloc_reg_reserve(out_ctx *octx, int *n);

/* register allocation */
int v_unused_reg(
		out_ctx *octx,
		int stack_as_backup, int fp,
		struct vreg *out,
		out_val const *to_replace);

/* v_to_* */
enum vto
{
	TO_REG = 1 << 0,
	TO_MEM = 1 << 1,
	TO_CONST = 1 << 2,
};
const out_val *v_to(out_ctx *octx, const out_val *vp, enum vto loc) ucc_wur;

const out_val *v_to_reg_given(
		out_ctx *octx, const out_val *from,
		const struct vreg *given) ucc_wur;

const out_val *v_to_reg_given_freeup_no_off(
		out_ctx *octx, const out_val *from,
		const struct vreg *given);

const out_val *v_to_reg_out(out_ctx *octx, const out_val *conv, struct vreg *out) ucc_wur;
const out_val *v_to_reg(out_ctx *octx, const out_val *conv) ucc_wur;
const out_val *v_reg_apply_offset(out_ctx *octx, const out_val *vreg) ucc_wur;

/* this functions allows us to either perform the opposite of a dereferencing
 * (see v_reg_to_stack_mem()), or store a value, but record it as spilt */
const out_val *v_to_stack_mem(
		out_ctx *octx, const out_val *val, const out_val *stk, enum out_val_store type);

/* this functions assumes the outcome should be a V_REGOFF, not V_SPILT
 * i.e. the opposite of dereferencing a value - we make a stack slot and save it there,
 * returning the slot */
const out_val *v_reg_to_stack_mem(
		out_ctx *octx, struct vreg const *, const out_val *stk);

/* register saving */
void v_freeup_reg(out_ctx *, const struct vreg *r);

/* func_ty may be null, ignores is null-terminated */
void v_save_regs(
		out_ctx *, type *func_ty,
		const out_val *ignores[], const out_val *fnval);

int v_is_const_reg(const out_val *);
int v_needs_GOT(const out_val *);


void v_reserve_reg(out_ctx *, const struct vreg *);
void v_unreserve_reg(out_ctx *, const struct vreg *);

/* util */
enum flag_cmp v_not_cmp(enum flag_cmp);
enum flag_cmp v_commute_cmp(enum flag_cmp);

#endif
