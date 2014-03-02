#include <stdio.h>
#include <stdarg.h>

#include "../../util/util.h"
#include "../../util/platform.h"

#include "../decl.h"
#include "../op.h"
#include "../type_nav.h"

#include "vstack.h"
#include "asm.h"
#include "impl.h"
#include "write.h"
#include "out.h"

void impl_comment(enum section_type sec, const char *fmt, va_list l)
{
	out_asm2(sec, P_NO_NL, "/* ");
	out_asmv(sec, P_NO_INDENT | P_NO_NL, fmt, l);
	out_asm2(sec, P_NO_INDENT, " */");
}

void impl_lbl(const char *lbl)
{
	out_asm2(SECTION_TEXT, P_NO_INDENT, "%s:", lbl);
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

int vreg_eq(const struct vreg *a, const struct vreg *b)
{
	return a->idx == b->idx && a->is_float == b->is_float;
}

static void impl_overlay_mem_reg(
		unsigned memsz, unsigned nregs,
		struct vreg regs[], int mem2reg)
{
	const unsigned pws = platform_word_size();
	struct vreg *cur_reg = regs;
	unsigned reg_i = 0;

	UCC_ASSERT(
			nregs * pws >= memsz,
			"not enough registers for memory overlay");

	out_comment("overlay, %s2%s(%u)",
			mem2reg ? "mem" : "reg",
			mem2reg ? "reg" : "mem",
			memsz);

	if(!mem2reg){
		/* reserve all registers so we don't accidentally wipe before the spill */
		for(reg_i = 0; reg_i < nregs; reg_i++)
			v_reserve_reg(&regs[reg_i]);
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
			this_ty = type_nav_MAX_FOR(cc1_type_nav, memsz);
		}
		this_sz = type_size(this_ty, NULL);

		UCC_ASSERT(this_sz <= memsz, "reading/writing too much memory");

		out_dup(); /* vv */
		out_change_type(type_ptr_to(this_ty));

		if(mem2reg){
			out_deref(); /* vA */

			/* move to register */
			UCC_ASSERT(reg_i < nregs, "reg oob");
			v_freeup_reg(cur_reg, 0);
			v_to_reg_given(vtop, cur_reg);
			v_reserve_reg(cur_reg); /* prevent changes */

			/* forget about the register, as far as the vstack is concerned */
			out_pop(); /* v */
		}else{
			vpush(this_ty); /* vvR */
			v_set_reg(vtop, cur_reg);
			out_store(); /* vv */

			out_pop(); /* v */
		}

		memsz -= this_sz;

		/* early termination */
		if(memsz == 0)
			break;

		/* increment our memory pointer */
		out_change_type(type_ptr_to(type_nav_btype(cc1_type_nav, type_uchar)));

		out_push_l(type_nav_btype(cc1_type_nav, type_intptr_t), pws);
		/* v8 */

		out_op(op_plus);
		/* {v+8} */
	}

	/* done, unreserve all registers */
	for(reg_i = 0; reg_i < nregs; reg_i++)
		v_unreserve_reg(&regs[reg_i]);
}

void impl_overlay_mem2regs(
		unsigned memsz, unsigned nregs,
		struct vreg regs[])
{
	impl_overlay_mem_reg(memsz, nregs, regs, 1);
}

void impl_overlay_regs2mem(
		unsigned memsz, unsigned nregs,
		struct vreg regs[])
{
	impl_overlay_mem_reg(memsz, nregs, regs, 0);
}
