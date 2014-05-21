#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "../../util/util.h"
#include "../../util/alloc.h"
#include "../../util/dynarray.h"

#include "../../util/platform.h"
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
#include "write.h"

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
	out_blk *blk = octx->current_blk;
	if(!blk){
		assert(octx->last_used_blk);
		blk = octx->last_used_blk;
	}
	out_dbg_flush(octx);
	dynarray_add(&blk->insns, ustrprintf("%s:\n", lbl));
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

void out_dump_retained(out_ctx *octx, const char *desc)
{
	out_val_list *l;
	int done_desc = 0;

	for(l = octx->val_head; l; l = l->next){
		if(l->val.retains == 0)
			continue;

		if(!done_desc){
			fprintf(stderr, "leaks after %s:\n", desc);
			done_desc = 1;
		}

		fprintf(stderr, "retained(%d) %s { %d %d } %p\n",
				l->val.retains,
				v_store_to_str(l->val.type),
				l->val.bits.regoff.reg.is_float,
				l->val.bits.regoff.reg.idx,
				(void *)&l->val);
	}
}

void out_ctx_end(out_ctx *octx)
{
	/* TODO */
	(void)octx;
}

void out_comment(out_ctx *octx, const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	impl_comment(octx, fmt, l);
	va_end(l);
}

const out_val *out_cast(out_ctx *octx, const out_val *val, type *to, int normalise_bool)
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
		/* even if vtop is V_CONST_I, we still
		 * want runtime code for sign extension */
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

				out_comment(octx, "truncate cast from %s to %s, size %d -> %d",
						from ? type_to_str_r(buf, from) : "",
						to   ? type_to_str(to) : "",
						szfrom, szto);
			}
		}
	}

	return v_dup_or_reuse(octx, val, to);
}

const out_val *out_change_type(out_ctx *octx, const out_val *val, type *ty)
{
	return v_dup_or_reuse(octx, val, ty);
}

const out_val *out_deref(out_ctx *octx, const out_val *target)
{
	type *tnext = type_pointed_to(target->t);

	/* if the pointed-to object is not an lvalue, don't deref */
	if(type_is(tnext, type_array)
	|| type_is(type_is_ptr(tnext), type_func))
	{
		/* noop */
		return v_dup_or_reuse(octx, target, tnext);
	}

	return impl_deref(octx, target, NULL);
}

const out_val *out_normalise(out_ctx *octx, const out_val *unnormal)
{
	out_val *const normalised = v_dup_or_reuse(octx, unnormal, unnormal->t);
	const out_val *ret = normalised;

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
			ret = v_to_reg(octx, normalised);
			/* fall */

		case V_REG:
		{
			const out_val *z = out_new_zero(octx, normalised->t);

			ret = out_op(octx, op_ne, z, unnormal);
			/* 0 -> `0 != 0` = 0
			 * 1 -> `1 != 0` = 1
			 * 5 -> `5 != 0` = 1
			 */
		}
	}

	return ret;
}

const out_val *out_set_bitfield(
		out_ctx *octx, const out_val *val,
		unsigned off, unsigned nbits)
{
	out_val *mut = v_dup_or_reuse(octx, val, val->t);

	mut->bitfield.off = off;
	mut->bitfield.nbits = nbits;

	return mut;
}

void out_store(out_ctx *octx, const out_val *dest, const out_val *val)
{
	impl_store(octx, dest, val);
}
