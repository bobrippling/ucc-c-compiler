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

static out_val *v_new(out_ctx *octx)
{
	out_val *v = umalloc(sizeof *v);
	(void)octx;
	return v;
}

out_val *v_new_from(out_ctx *octx, out_val *from)
{
	if(from->retains == 0){
		memset(from, 0, sizeof *from);
		return from;
	}
	return v_new(octx);
}

out_val *v_new_reg(out_ctx *octx, const struct vreg *reg)
{
	out_val *v = v_new(octx);
	v->type = V_REG;
	memcpy_safe(&v->bits.regoff.reg, reg);
	return v;
}

out_val *v_new_sp(out_ctx *octx)
{
	struct vreg r;

	r.is_float = 0;
	r.idx = REG_SP;

	return v_new_reg(octx, &r);
}

out_val *v_new_sp3(out_ctx *octx, type *ty, long stack_pos)
{
	out_val *v = v_new_sp(octx);
	v->t = ty;
	v->bits.regoff.offset = stack_pos;
	return v;
}

void out_val_release(out_ctx *octx, out_val *v)
{
	(void)octx;
	assert(v->retains > 0 && "double release");
	v->retains--;
}

void out_val_retain(out_ctx *octx, out_val *v)
{
	(void)octx;
	v->retains++;
}
