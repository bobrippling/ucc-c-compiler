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
#include "../const.h"
#include "../type_nav.h"
#include "../type_is.h"
#include "../fopt.h" /* fopt */
#include "../cc1.h" /* fopt */

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
#include "bitfield.h"

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

out_ctx *out_ctx_new(void)
{
	out_ctx *ctx = umalloc(sizeof *ctx);

	ctx->dbg.last_file = ctx->dbg.last_line = -1;

	ctx->reserved_regs = v_alloc_reg_reserve(ctx, NULL);

	return ctx;
}

void **out_user_ctx(out_ctx *octx)
{
	return &octx->userctx;
}

int out_dump_retained(out_ctx *octx, const char *desc)
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

		fprintf(stderr, "retained(%d) %s { %d %d } %s%p\n",
				l->val.retains,
				v_store_to_str(l->val.type),
				l->val.bits.regoff.reg.is_float,
				l->val.bits.regoff.reg.idx,
				l->val.phiblock ? "(phi) " : "",
				(void *)&l->val);
	}

	return done_desc;
}

void out_comment(out_ctx *octx, const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	impl_comment(octx, fmt, l);
	va_end(l);
}

const char *out_val_str(const out_val *v, int deref)
{
	return impl_val_str(v, deref);
}

const out_val *out_cast(out_ctx *octx, const out_val *val, type *to, int normalise_bool)
{
	type *from = val->t;
	char fp[2];

	switch(val->type){
		case V_REG:
			if(val->bits.regoff.offset
			&& type_size(val->t, NULL) != type_size(to, NULL))
			{
				/* must apply the offset in the current type */
				val = v_reg_apply_offset(octx, val);
			}
			break;

		case V_SPILT:
		case V_REGOFF:
			/* must load the value for a sensible conversion */
			val = v_to_reg(octx, val);
			from = val->t;
			break;

		default:
			break;
	}

	/* normalise before the cast, otherwise we do things like
	 * 5.3 -> 5, then normalise 5, instead of 5.3 != 0.0
	 */
	if(normalise_bool && type_is_primitive(to, type__Bool)){
		val = out_normalise(octx, val);

		if(type_cmp(val->t, to, 0) & TYPE_EQUAL_ANY){
			out_comment(octx, "out_cast done via normalise");
			return val;
		}

		/* val may have changed type, e.g. float -> _Bool.
		 * update `from' */
		from = val->t;
	}

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
	type *tnext = type_is_ptr(target->t);
	struct vreg reg_store;
	const struct vreg *reg = &reg_store;
	int is_fp;
	struct vbitfield bf = target->bitfield;
	const out_val *dval;
	int done_out_deref;

	/* if the pointed-to object is not an lvalue, don't deref */
	if(type_is(tnext, type_array)
	|| type_is(tnext, type_func))
	{
		/* noop */
		return v_dup_or_reuse(octx, target,
				type_dereference_decay(target->t));
	}

	is_fp = type_is_floating(tnext);

	v_unused_reg(octx, 1, is_fp, &reg_store, target);

	if(bf.nbits){
		/* need to ensure we load using the bitfield's master type */
		target = out_cast(octx, target, type_ptr_to(bf.master_ty), 0);
	}

	if(target->type == V_SPILT){
		if(cc1_fopt.verbose_asm)
			out_comment(octx, "double-indir for spilt value");
		target = impl_deref(octx, target, reg, NULL);
	}

	dval = impl_deref(octx, target, reg, &done_out_deref);

	if(bf.nbits && !done_out_deref)
		dval = out_bitfield_to_scalar(octx, &bf, dval);

	return dval;
}

const out_val *out_normalise(out_ctx *octx, const out_val *unnormal)
{
	out_val *normalised = v_dup_or_reuse(octx, unnormal, unnormal->t);

	switch(normalised->type){
		case V_FLAG:
			/* already normalised */
			break;

		case V_CONST_I:
			normalised->bits.val_i = !!normalised->bits.val_i;
			break;

		case V_CONST_F:
			normalised->bits.val_i = !!normalised->bits.val_f;
			normalised->type = V_CONST_I;
			/* float to int - change .t */
			normalised->t = type_nav_btype(cc1_type_nav, type__Bool);
			break;

		default:
			normalised = (out_val *)v_to_reg(octx, normalised);
			/* fall */

		case V_REG:
		{
			const out_val *z = out_new_zero(octx, normalised->t);

			normalised = (out_val *)out_op(octx, op_ne, z, normalised);
			/* 0 -> `0 != 0` = 0
			 * 1 -> `1 != 0` = 1
			 * 5 -> `5 != 0` = 1
			 */
		}
	}

	return normalised;
}

const out_val *out_set_bitfield(
		out_ctx *octx, const out_val *val,
		unsigned off, unsigned nbits,
		type *master_ty)
{
	out_val *mut = v_dup_or_reuse(octx, val, val->t);

	mut->bitfield.off = off;
	mut->bitfield.nbits = nbits;
	mut->bitfield.master_ty = master_ty;

	return mut;
}

void out_store(out_ctx *octx, const out_val *dest, const out_val *val)
{
	if(dest->bitfield.nbits){
		out_val_retain(octx, dest);
		val = out_bitfield_scalar_merge(octx, &dest->bitfield, val, dest);
	}

	impl_store(octx, dest, val);
}

void out_force_read(out_ctx *octx, type *ty, const out_val *v)
{
	/* must read */
	const out_val *target = out_aalloct(octx, ty);

	out_val_consume(octx,
			out_memcpy(octx, target, v, type_size(ty, NULL)));
}
