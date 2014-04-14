#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "../../util/util.h"
#include "../../util/alloc.h"

#include "../../util/platform.h"
#include "../cc1.h"
#include "../pack.h"
#include "../defs.h"
#include "../opt.h"
#include "../const.h"
#include "../type_nav.h"
#include "../type_is.h"

#include "asm.h"
#include "out.h"
#include "val.h"
#include "impl.h"
#include "lbl.h"
#include "virt.h"
#include "ctx.h"
#include "blk.h"
#include "dbg.h"

/*
 * stack layout is:
 *   2nd variadic-extra arg...
 *   1st variadic-extra arg,
 *   3rd arg...
 *   2nd arg
 *   1st arg
 *   --bp/ra--,
 *   call_regs[0],
 *   call_regs[1],
 *   call_regs[2],
 *   variadic_reg_args[2],
 *   variadic_reg_args[1],
 *   variadic_reg_args[0], <-- stack_variadic_offset
 *   local_var[0],         <-- stack_local_offset
 *   local_var[1],
 *   local_var[2],         <-- stack_sz (note local vars may be packed)
 *
 * all call/variadic regs must be at increasing memory addresses for
 * variadic logic is in impl_func_prologue_save_variadic
 */

void out_dbg_label(out_ctx *octx, const char *lbl)
{
	impl_lbl(octx, lbl);
}

out_ctx *out_ctx_new(void)
{
	out_ctx *ctx = umalloc(sizeof *ctx);
	return ctx;
}

size_t out_expr_stack(out_ctx *octx)
{
	out_val_list *l;
	size_t retains = 0;

	for(l = octx->val_head; l; l = l->next)
		retains += l->val.retains;

	return retains;
}

void out_expr_stack_assert(out_ctx *octx, size_t prev)
{
	size_t now = out_expr_stack(octx);

	if(now != prev){
		ICW("values still retained (%ld)", (long)(now - prev));

		for(out_val_list *l = octx->val_head; l; l = l->next){
			if(l->val.retains)
				fprintf(stderr, "retained %s { %d %d } %p\n",
						v_store_to_str(l->val.type),
						l->val.bits.regoff.reg.is_float,
						l->val.bits.regoff.reg.idx,
						&l->val);
		}
	}
}

void out_ctx_end(out_ctx *octx)
{
	/* TODO */
	(void)octx;
}

out_blk *out_blk_new(out_ctx *octx, const char *desc)
{
	out_blk *blk = umalloc(sizeof *blk);
	blk->desc = desc;
	blk->lbl = out_label_bblock(octx->nblks++);
	return blk;
}

void out_comment(const char *fmt, ...)
{
	/* TODO */
	(void)fmt;
}

out_val *out_cast(out_ctx *octx, out_val *val, type *to, int normalise_bool)
{
	type *const from = val->t;
	char fp[2];

	/* normalise before the cast, otherwise we do things like
	 * 5.3 -> 5, then normalise 5, instead of 5.3 != 0.0
	 */
	if(normalise_bool && type_is_primitive(to, type__Bool))
		val = out_normalise(octx, val);

	fp[0] = type_is_floating(from);
	fp[1] = type_is_floating(to);

	if(fp[0] || fp[1]){
		if(fp[0] && fp[1]){

			if(type_primitive(from) != type_primitive(to))
				val = impl_f2f(octx, val, from, to);
			/* else no-op cast */

		}else if(fp[0]){
			/* float -> int */
			UCC_ASSERT(type_is_integral(to),
					"fp to %s?", type_to_str(to));

			val = impl_f2i(octx, val, from, to);

		}else{
			/* int -> float */
			UCC_ASSERT(type_is_integral(from),
					"%s to fp?", type_to_str(from));

			val = impl_i2f(octx, val, from, to);
		}

	}else{
		/* casting integral vtop
		 * don't bother if it's a constant,
		 * just change the size */
		if(val->type != V_CONST_I){
			int szfrom = asm_type_size(from),
					szto   = asm_type_size(to);

			if(szfrom != szto){
				if(szto > szfrom){
					/* we take from's signedness for our sign-extension,
					 * e.g. uint64_t x = (int)0x8000_0000;
					 * sign extends the int to an int64_t, then changes
					 * the type
					 */
					val = impl_cast_load(octx, val, from, to,
							type_is_signed(from));
				}else{
					char buf[TYPE_STATIC_BUFSIZ];

					out_comment("truncate cast from %s to %s, size %d -> %d",
							from ? type_to_str_r(buf, from) : "",
							to   ? type_to_str(to) : "",
							szfrom, szto);
				}
			}
		}
	}

	return val;
}

out_val *out_change_type(out_ctx *octx, out_val *val, type *ty)
{
	return v_dup_or_reuse(octx, val, ty);
}

out_val *out_deref(out_ctx *octx, out_val *target)
{
	return impl_deref(octx, target, NULL);
}

out_val *out_normalise(out_ctx *octx, out_val *unnormal)
{
	out_val *normalised = v_dup_or_reuse(octx, unnormal, unnormal->t);

	switch(unnormal->type){
		case V_FLAG:
			/* already normalised */
			break;

		case V_CONST_I:
			normalised->bits.val_i = !!normalised->bits.val_i;
			break;

		case V_CONST_F:
			normalised->bits.val_i = !!normalised->bits.val_f;
			normalised->type = V_CONST_I;
			break;

		default:
			normalised = v_to_reg(octx, normalised);
			/* fall */

		case V_REG:
		{
			out_val *z = out_new_zero(octx, normalised->t);

			normalised = out_op(octx, op_ne, z, unnormal);
			/* 0 -> `0 != 0` = 0
			 * 1 -> `1 != 0` = 1
			 * 5 -> `5 != 0` = 1
			 */
		}
	}

	return normalised;
}

out_val *out_op(out_ctx *octx, enum op_type binop, out_val *lhs, out_val *rhs)
{
	return impl_op(octx, binop, lhs, rhs);
}

out_val *out_op_unary(out_ctx *octx, enum op_type uop, out_val *exp)
{
	return impl_op_unary(octx, uop, exp);
}

void out_set_bitfield(
		out_ctx *octx, out_val *val,
		unsigned off, unsigned nbits)
{
	(void)octx;
	val->bitfield.off = off;
	val->bitfield.nbits = nbits;
}

void out_store(out_ctx *octx, out_val *dest, out_val *val)
{
	impl_store(octx, dest, val);
}
