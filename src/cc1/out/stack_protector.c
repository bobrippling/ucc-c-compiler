#include <stdlib.h>
#include <stdarg.h>

#include "../type.h"
#include "val.h"
#include "blk.h"
#include "ctx.h"
#include "stack_protector.h"

#include "../mangle.h"
#include "../type_nav.h"
#include "../funcargs.h"
#include "../cc1_target.h"
#include "impl.h"

static const out_val *stack_canary_address_lbl(out_ctx *octx, char **const tofree)
{
	const char *stack_chk_guard = "__stack_chk_guard";
	char *mangled = func_mangle(stack_chk_guard, NULL);
	type *intptr_ty = type_nav_btype(cc1_type_nav, type_intptr_t);

	*tofree = NULL;
	if(mangled != stack_chk_guard)
		*tofree = mangled;

	return out_new_lbl(
			octx,
			type_ptr_to(type_ptr_to(intptr_ty)),
			mangled,
			OUT_LBL_PIC); /* FIXME? */
}

static const out_val *stack_canary_address_tls(out_ctx *octx)
{
	const char *tlsent = "%fs:40"; /* XXX: hack until tls support is available */
	type *intptr_ty = type_nav_btype(cc1_type_nav, type_intptr_t);

	return out_new_lbl(
			octx,
			type_ptr_to(type_ptr_to(intptr_ty)),
			tlsent,
			OUT_LBL_NOPIC);
}

static const out_val *stack_canary_address(out_ctx *octx, char **const tofree)
{
	if(cc1_target_details.as.stack_protector_via_tls){
		*tofree = NULL;
		return stack_canary_address_tls(octx);
	}

	return stack_canary_address_lbl(octx, tofree);
}

static const out_val *stack_check_fail_func(
		out_ctx *octx, type **const pfnty, char **const tofree)
{
	type *fnty = type_ptr_to(
			type_func_of(
			type_nav_btype(cc1_type_nav, type_void),
			funcargs_new_void(),
			NULL));
	const char *chk_fail = "__stack_chk_fail";
	char *mangled = func_mangle(chk_fail, NULL);

	if(mangled == chk_fail)
		*tofree = NULL;
	else
		*tofree = mangled;

	*pfnty = fnty;

	return out_new_lbl(octx, fnty, mangled, /* usually in libc */OUT_LBL_PIC);
}

void out_init_stack_canary(
		out_ctx *octx, const out_val *stack_prot_slot)
{
	char *tofree;
	const out_val *chk_guard = stack_canary_address(octx, &tofree);

	octx->stack_canary_ent = out_val_retain(octx, stack_prot_slot);

	out_store(octx, stack_prot_slot, out_deref(octx, chk_guard));

	free(tofree);
}

void out_check_stack_canary(out_ctx *octx)
{
	const out_val *sp_val;
	const out_val *cond;
	out_blk *bok, *bsmashed;
	char *tofree;

	if(!octx->stack_canary_ent)
		return;

	impl_reserve_retregs(octx);

	bok = out_blk_new(octx, "stack_prot_ok");
	bsmashed = out_blk_new(octx, "stack_prot_fail");

	sp_val = out_deref(octx, /*released:*/octx->stack_canary_ent);
	octx->stack_canary_ent = NULL;

	cond = out_annotate_likely(
			octx,
			out_op(
				octx, op_eq, sp_val,
				out_deref(octx, stack_canary_address(octx, &tofree))),
			0);

	impl_unreserve_retregs(octx);

	free(tofree), tofree = NULL;
	out_ctrl_branch(octx, cond, bok, bsmashed);

	out_current_blk(octx, bsmashed);
	{
		type *fnty;
		char *tofree;
		const out_val *fn = stack_check_fail_func(octx, &fnty, &tofree);

		out_val_consume(octx, out_call(octx, fn, NULL, fnty));
		out_ctrl_end_undefined(octx);

		free(tofree);
	}

	out_current_blk(octx, bok);
}
