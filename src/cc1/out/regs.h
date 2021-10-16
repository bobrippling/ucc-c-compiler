#ifndef IMPL_REGS
#define IMPL_REGS

void impl_regs_reserve_rets(out_ctx *);
void impl_regs_unreserve_rets(out_ctx *);

void impl_regs_reserve_args(out_ctx *);
void impl_regs_unreserve_args(out_ctx *);

#endif
