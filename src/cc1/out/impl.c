#include <stdio.h>
#include <stdarg.h>

#include "../../util/util.h"
#include "../../util/platform.h"

#include "../decl.h"
#include "../op.h"
#include "../type_nav.h"
#include "../macros.h"

#include "val.h"
#include "asm.h"
#include "impl.h"
#include "write.h"
#include "out.h"
#include "virt.h"

void impl_comment(out_ctx *octx, const char *fmt, va_list l)
{
	out_asm2(octx, SECTION_TEXT, P_NO_LIVEDUMP | P_NO_NL, "/* ");
	out_asmv(octx, SECTION_TEXT, P_NO_LIVEDUMP | P_NO_INDENT | P_NO_NL, fmt, l);
	out_asm2(octx, SECTION_TEXT, P_NO_LIVEDUMP | P_NO_INDENT, " */");
}

enum flag_cmp op_to_flag(enum op_type op)
{
	switch(op){
#define OP(x) case op_ ## x: return flag_ ## x
		OP(eq);
		OP(ne);
		OP(le);
		OP(lt);
		OP(ge);
		OP(gt);
#undef OP

		default:
			break;
	}

	ICE("invalid op");
	return -1;
}

const char *flag_cmp_to_str(enum flag_cmp cmp)
{
	switch(cmp){
		CASE_STR_PREFIX(flag, eq);
		CASE_STR_PREFIX(flag, ne);
		CASE_STR_PREFIX(flag, le);
		CASE_STR_PREFIX(flag, lt);
		CASE_STR_PREFIX(flag, ge);
		CASE_STR_PREFIX(flag, gt);
		CASE_STR_PREFIX(flag, overflow);
		CASE_STR_PREFIX(flag, no_overflow);
	}
	return NULL;
}

int impl_reg_is_callee_save(type *fnty, const struct vreg *r)
{
	unsigned i, n;
	const int *csaves;

	if(r->is_float)
		return 0;

	csaves = impl_callee_save_regs(fnty, &n);

	for(i = 0; i < n; i++)
		if(r->idx == csaves[i])
			return 1;
	return 0;
}

const char *impl_val_str(const out_val *vs, int deref)
{
	static char buf[VAL_STR_SZ];
	return impl_val_str_r(buf, vs, deref);
}

static void impl_overlay_mem_reg(
		out_ctx *octx,
		unsigned memsz, unsigned nregs,
		struct vreg regs[], int mem2reg,
		const out_val *ptr)
{
	const unsigned pws = platform_word_size();
	struct vreg *cur_reg = regs;
	unsigned reg_i = 0;

	if(memsz == 0){
		out_val_release(octx, ptr);
		return;
	}

	UCC_ASSERT(
			nregs * pws >= memsz,
			"not enough registers for memory overlay");

	out_comment(octx,
			"overlay, %s2%s(%u)",
			mem2reg ? "mem" : "reg",
			mem2reg ? "reg" : "mem",
			memsz);

	if(!mem2reg){
		/* reserve all registers so we don't accidentally wipe before the spill */
		for(reg_i = 0; reg_i < nregs; reg_i++)
			v_reserve_reg(octx, &regs[reg_i]);
	}

	for(;; cur_reg++, reg_i++){
		/* read/write whatever size is required */
		type *this_ty;
		unsigned this_sz;

		if(cur_reg->is_float){
			UCC_ASSERT(memsz >= 4, "float for memsz %u?", memsz);

			this_ty = type_nav_btype(
					cc1_type_nav,
					memsz > 4 ? type_double : type_float);

		}else{
			this_ty = type_nav_MAX_FOR(cc1_type_nav, memsz, 0);
		}
		this_sz = type_size(this_ty, NULL);

		UCC_ASSERT(this_sz <= memsz, "reading/writing too much memory");

		ptr = out_change_type(octx, ptr, type_ptr_to(this_ty));

		out_val_retain(octx, ptr);

		if(mem2reg){
			const out_val *fetched;

			/* can use impl_deref, as we have a register already,
			 * and know that the memory is an lvalue and not a bitfield
			 *
			 * this means we can load straight into the desired register
			 */
			fetched = impl_deref(octx, ptr, cur_reg, NULL);

			UCC_ASSERT(reg_i < nregs, "reg oob");

			if(fetched->type != V_REG || !vreg_eq(&fetched->bits.regoff.reg, cur_reg)){
				/* move to register */
				v_freeup_reg(octx, cur_reg);
				fetched = v_to_reg_given(octx, fetched, cur_reg);
			}
			out_flush_volatile(octx, fetched);
			v_reserve_reg(octx, cur_reg); /* prevent changes */

		}else{
			const out_val *vreg = v_new_reg(octx, NULL, this_ty, cur_reg);

			out_store(octx, ptr, vreg);
		}

		memsz -= this_sz;

		/* early termination */
		if(memsz == 0)
			break;

		/* increment our memory pointer */
		ptr = out_change_type(
				octx,
				ptr,
				type_ptr_to(type_nav_btype(cc1_type_nav, type_uchar)));

		ptr = out_op(octx, op_plus,
				ptr,
				out_new_l(
					octx,
					type_nav_btype(cc1_type_nav, type_intptr_t),
					pws));
	}

	out_val_release(octx, ptr);

	/* done, unreserve all registers */
	for(reg_i = 0; reg_i < nregs; reg_i++)
		v_unreserve_reg(octx, &regs[reg_i]);
}

void impl_overlay_mem2regs(
		out_ctx *octx,
		unsigned memsz, unsigned nregs,
		struct vreg regs[],
		const out_val *ptr)
{
	impl_overlay_mem_reg(octx, memsz, nregs, regs, 1, ptr);
}

void impl_overlay_regs2mem(
		out_ctx *octx,
		unsigned memsz, unsigned nregs,
		struct vreg regs[],
		const out_val *ptr)
{
	impl_overlay_mem_reg(octx, memsz, nregs, regs, 0, ptr);
}
