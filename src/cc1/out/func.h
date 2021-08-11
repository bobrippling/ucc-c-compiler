#ifndef FUNC_H
#define FUNC_H

enum stret
{
	stret_scalar,
	stret_regs,
	stret_memcpy
};

void impl_func_call_regs(type *, unsigned *pn, const struct vreg **);
enum stret impl_func_stret(type *, unsigned *stack_space);
void impl_func_alignstack(out_ctx *);
int impl_func_caller_cleanup(type *);

void impl_func_call(
		out_ctx *octx, type *fnty, int nfloats,
		const out_val *fn, const out_val **args);

void impl_func_overlay_regpair(
		struct vreg regpair[/*2*/], int *const nregs, type *retty);

#endif
