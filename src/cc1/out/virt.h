#ifndef VIRT_H
#define VIRT_H

unsigned char *v_alloc_reg_reserve(out_ctx *octx, int *n);

/* stack */
void v_stack_adj(out_ctx *octx, unsigned amt, int sub);

unsigned v_alloc_stack2(out_ctx *octx,
		const unsigned sz_initial, int noop, const char *desc);

unsigned v_alloc_stack_n(out_ctx *octx, unsigned sz, const char *desc);

unsigned v_alloc_stack(out_ctx *octx, unsigned sz, const char *desc);

unsigned v_stack_align(out_ctx *octx, unsigned const align, int force_mask);

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

const out_val *v_to_reg_given_freeup(
		out_ctx *octx, const out_val *from,
		const struct vreg *given);

const out_val *v_to_reg_out(out_ctx *octx, const out_val *conv, struct vreg *out) ucc_wur;
const out_val *v_to_reg(out_ctx *octx, const out_val *conv) ucc_wur;
const out_val *v_reg_apply_offset(out_ctx *octx, const out_val *vreg) ucc_wur;

const out_val *v_to_stack_mem(out_ctx *octx, const out_val *vp, long stack_pos) ucc_wur;

void v_reg_to_stack(
		out_ctx *octx,
		const struct vreg *vr,
		type *ty, long where);


/* register saving */
void v_freeup_reg(out_ctx *, const struct vreg *r);
const out_val *v_find_reg(out_ctx *octx, const struct vreg *reg);

/* func_ty may be null, ignores is null-terminated */
void v_save_regs(
		out_ctx *, type *func_ty,
		const out_val *ignores[], const out_val *fnval);

int v_is_const_reg(const out_val *);


void v_reserve_reg(out_ctx *, const struct vreg *);
void v_unreserve_reg(out_ctx *, const struct vreg *);

/* util */
enum flag_cmp v_inv_cmp(enum flag_cmp cmp, int invert_eq);

#endif
