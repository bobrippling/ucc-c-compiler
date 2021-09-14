#ifndef IMPL_REGS
#define IMPL_REGS

void impl_regs_reserve_rets(out_ctx *, type *fnty);
void impl_regs_unreserve_rets(out_ctx *, type *fnty);

void impl_regs_reserve_args(out_ctx *, type *fnty);
void impl_regs_unreserve_args(out_ctx *, type *fnty);

/* implemented by backend */
const struct vreg *impl_regs_for_args(type *fnty, size_t *const n);

#endif
