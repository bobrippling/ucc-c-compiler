#ifndef OUT_STACK_PROTECTOR_H
#define OUT_STACK_PROTECTOR_H

void out_init_stack_canary(
		out_ctx *octx,
		const out_val *stack_prot_slot);

void out_check_stack_canary(out_ctx *octx);

#endif
