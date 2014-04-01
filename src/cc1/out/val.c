#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../../util/alloc.h"

#include "../type.h"

#include "val.h"

#include "../op.h"
#include "asm.h"
#include "impl.h"
#include "out.h" /* retain/release prototypes */

out_val *v_new_from(out_ctx *octx, out_val *from)
{
	out_val *v;

	(void)octx;

	if(from && from->retains == 0)
		return from;

	v = umalloc(sizeof *v);
	if(from)
		memcpy_safe(v, from);
	return v;
}

out_val *v_new_flag(
		out_ctx *octx, out_val *from,
		enum flag_cmp cmp, enum flag_mod mod)
{
	out_val *v = v_new_from(octx, from);
	v->type = V_FLAG;
	v->bits.flag.cmp = cmp;
	v->bits.flag.mods = mod;
	return v;
}

out_val *v_new_reg(out_ctx *octx, out_val *from, const struct vreg *reg)
{
	out_val *v = v_new_from(octx, from);
	v->type = V_REG;
	memcpy_safe(&v->bits.regoff.reg, reg);
	return v;
}

out_val *v_new_sp(out_ctx *octx, out_val *from)
{
	struct vreg r;

	r.is_float = 0;
	r.idx = REG_SP;

	return v_new_reg(octx, from, &r);
}

out_val *v_new_sp3(out_ctx *octx, out_val *from, type *ty, long stack_pos)
{
	out_val *v = v_new_sp(octx, from);
	v->t = ty;
	v->bits.regoff.offset = stack_pos;
	return v;
}

out_val *out_val_release(out_ctx *octx, out_val *v)
{
	(void)octx;
	assert(v->retains > 0 && "double release");
	v->retains--;
	return v;
}

out_val *out_val_retain(out_ctx *octx, out_val *v)
{
	(void)octx;
	v->retains++;
	return v;
}

int vreg_eq(const struct vreg *a, const struct vreg *b)
{
	return a->idx == b->idx && a->is_float == b->is_float;
}
