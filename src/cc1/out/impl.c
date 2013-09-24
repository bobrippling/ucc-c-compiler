#include <stdio.h>
#include <stdarg.h>

#include "../../util/util.h"
#include "../data_structs.h"
#include "../decl.h"
#include "vstack.h"
#include "asm.h"
#include "basic_block_int.h"
#include "impl.h"
#include "out.h"

#include "../cc1.h"

void out_asm(basic_blk *bb, const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	bb_addv(bb, fmt, l);
	va_end(l);
}

void impl_comment_sec(enum section_type sec, const char *fmt, va_list l)
{
	FILE *f = cc_out[sec];

	fprintf(f, "\t/* ");
	vfprintf(f, fmt, l);
	fprintf(f, " */\n");
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

void impl_jcond_make(
		struct basic_blk_fork *b_fork,
		struct basic_blk *bb,
		const char *tlbl, const char *flbl)
{
	switch(vtop->type){
		case V_CONST_I:
			//impl_jmp(bb, vp->bits.val_i ? ltrue : lfalse);
			ICE("TODO: const jmp");
			break;

		case V_LBL:
		case V_REG_SAVE:
			v_to_reg(bb, vtop);

		case V_CONST_F:
		case V_REG:
			out_normalise(bb);
			UCC_ASSERT(vtop->type != V_REG,
					"normalise remained as a register");
		case V_FLAG:
			impl_jflag_make(b_fork, &vtop->bits.flag, tlbl, flbl);
			break;
	}
}
