#include <stddef.h>
#include <stdarg.h>

#include "out.h"
#include "val.h"
#include "../type_nav.h"

/* v_to_reg_given */
#include "virt.h"
#include "impl.h"

/* octx->var_stack_sz - out_alloca_push_fixed() */
#include "ctx.h"

/* cc1_mstack_align */
#include <stdarg.h>
#include "../cc1.h"

static const out_val *alloca_stack_adj(
		out_ctx *octx, enum op_type op,
		const out_val *sz)
{
	/* %sp op= sz */
	const out_val *adj = out_op(
			octx, op,
			v_new_sp(octx, NULL),
			sz);

	out_val_retain(octx, adj);
	out_flush_volatile(octx, adj);
	return adj;
}

const out_val *out_alloca_push(
		out_ctx *octx,
		const out_val *sz,
		unsigned align)
{
	type *arith_ty = type_nav_btype(cc1_type_nav, type_intptr_t);
	const out_val *adjusted_sp;

	if(align < (unsigned)cc1_mstack_align)
		align = cc1_mstack_align;

	if(align == 1)
		return alloca_stack_adj(octx, op_minus, sz);


	/* add $16, %rsp */
	sz = out_op(octx, op_plus, sz, out_new_l(octx, arith_ty, align));
	adjusted_sp = alloca_stack_adj(octx, op_minus, sz);

	/* and $~16, %rsp */
	return out_op(octx, op_and,
			adjusted_sp,
			out_new_l(octx, arith_ty, ~((long)align - 1)));
}

void out_alloca_pop(out_ctx *octx, const out_val *ptr)
{
	struct vreg sp;

	sp.idx = REG_SP;
	sp.is_float = 0;

	out_flush_volatile(octx, v_to_reg_given(octx, ptr, &sp));
}

unsigned out_alloca_fixed(out_ctx *octx, unsigned sz)
{
	v_alloc_stack(octx, sz, "inline variable");
	return octx->var_stack_sz;
}
