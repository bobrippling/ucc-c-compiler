#include <stddef.h>

#include "out.h"
#include "val.h"

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

#include <stdio.h>

const out_val *out_alloca_push(
		out_ctx *octx,
		const out_val *sz,
		unsigned align)
{
	/* TODO: align */
	static int warn;
	if(!warn){
		fprintf(stderr, "TODO: align vla (%u)\n", align);
		warn = 1;
	}
	return alloca_stack_adj(octx, op_minus, sz);
}

void out_alloca_pop(out_ctx *octx, const out_val *sz)
{
	out_flush_volatile(
			octx,
			alloca_stack_adj(octx, op_plus, sz));
}
