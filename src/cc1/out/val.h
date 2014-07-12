#ifndef VSTACK_H
#define VSTACK_H

#include "forwards.h"

struct out_val
{
	enum out_val_store
	{
		V_CONST_I, /* constant integer */

		V_REG, /* value in a register, possibly offset */
		V_REG_SPILT, /* value in memory pointed to by register */
		V_LBL, /* value at a memory address */

		V_CONST_F, /* constant float */
		V_FLAG, /* cpu flag */
#define V_IS_MEM(ty) ((ty) == V_REG_SPILT || (ty) == V_LBL)
	} type;
	unsigned retains;

	type *t;

	union
	{
		integral_t val_i;
		floating_t val_f;

		struct vreg_off
		{
			struct vreg
			{
				unsigned short idx;
				unsigned char is_float;
			} reg;
			long offset;
		} regoff;
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
		unsigned short off, nbits;
	} bitfield; /* !!width iif bitfield */
	unsigned char flags;
};

enum
{
	VAL_FLAG_LIKELY = 1 << 0,
	VAL_FLAG_UNLIKELY = 1 << 1,
};

const char *v_store_to_str(enum out_val_store);

int vreg_eq(const struct vreg *, const struct vreg *);

out_val *v_new(out_ctx *octx, type *);

out_val *v_dup_or_reuse(out_ctx *octx, const out_val *from, type *ty);

out_val *v_new_flag(
		out_ctx *octx, const out_val *from,
		enum flag_cmp, enum flag_mod);

out_val *v_new_sp(out_ctx *octx, const out_val *from /* void* */);

out_val *v_new_sp3(out_ctx *octx, const out_val *from, type *ty,
		long stack_pos);
out_val *v_new_bp3_above(out_ctx *octx, const out_val *from, type *ty,
		long stack_pos);
out_val *v_new_bp3_below(out_ctx *octx, const out_val *from, type *ty,
		long stack_pos);

out_val *v_new_reg(out_ctx *octx, const out_val *from, type *ty,
		const struct vreg *reg);

void out_val_overwrite(out_val *d, const out_val *s);

#endif
