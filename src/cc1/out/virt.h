#ifndef VIRT_H
#define VIRT_H

void v_stack_adj(out_ctx *octx, unsigned amt, int sub);

unsigned v_alloc_stack2(out_ctx *octx,
		const unsigned sz_initial, int noop, const char *desc);

unsigned v_alloc_stack_n(out_ctx *octx, unsigned sz, const char *desc);

unsigned v_alloc_stack(out_ctx *octx, unsigned sz, const char *desc);

unsigned v_stack_align(out_ctx *octx, unsigned const align, int force_mask);

void v_dealloc_stack(out_ctx *octx, unsigned sz);

out_val *v_to_reg_given(out_val *from, const struct vreg *given);
out_val *v_to_reg_out(out_ctx *octx, out_val *conv, struct vreg *out);
out_val *v_to_reg(out_ctx *octx, out_val *conv);

#endif
