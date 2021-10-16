#include "../util/macros.h"

#include "ctx.h"
#include "regs.h"

/* ret regs we can hardcode here for now
 * arg regs we need to make dependant on the call type
 * (for all of x86's calling conventions) */

static const struct vreg retregs[] = {
	{ REG_RET_I_1, 0 },
#ifdef REG_RET_I_2
	{ REG_RET_I_2, 0 },
#endif
#ifdef REG_RET_F_1
	{ REG_RET_F_1, 1 },
#endif
#ifdef REG_RET_F_2
	{ REG_RET_F_2, 1 },
#endif
};

static void reserve_unreserve_regs(
		out_ctx *octx,
		int reserve,
		const struct vreg *regs,
		size_t n)
{
	unsigned i;

	for(i = 0; i < n; i++){
		const struct vreg *r = &regs[i];
		if(reserve)
			v_reserve_reg(octx, r);
		else
			v_unreserve_reg(octx, r);
	}
}

void impl_regs_reserve_rets(out_ctx *octx)
{
	reserve_unreserve_regs(octx, 1, retregs, countof(retregs));
}

void impl_regs_unreserve_rets(out_ctx *octx)
{
	reserve_unreserve_regs(octx, 0, retregs, countof(retregs));
}

static const struct vreg *lookup_argregs(type *fnty, size_t *const n)
{
	funcargs *fa = type_funcargs(fnty);

	return &calling_convs[fa->conv];
}

void impl_regs_reserve_args(out_ctx *octx)
{
	size_t n;
	const struct vreg *regs = lookup_argregs(fnty, &n);

	reserve_unreserve_regs(octx, 1, regs, n);
}

void impl_regs_unreserve_args(out_ctx *octx)
{
	size_t n;
	const struct vreg *regs = lookup_argregs(fnty, &n);

	reserve_unreserve_regs(octx, 0, regs, n);
}
