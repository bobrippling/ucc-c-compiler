#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <assert.h>
#include <math.h>

#include "../type.h"
#include "../type_is.h"
#include "../type_nav.h"

#include "out.h"
#include "val.h"
#include "asm.h"
#include "impl.h"
#include "virt.h"

#include "../const.h"
#include "../cc1.h" /* fopt_mode */
#include "../vla.h"

static int calc_ptr_step(type *t)
{
	type *tnext;
	/* we are calculating the sizeof *t */

	if(type_is_primitive(type_is_ptr(t), type_void))
		return type_primitive_size(type_void);

	if(type_is_primitive(t, type_unknown))
		return 1;

	tnext = type_next(t);
	if(type_is_vla(tnext, VLA_ANY_DIMENSION))
		return -1;
	return type_size(tnext, NULL);
}

static void fill_if_type(
		const out_val *v,
		const out_val **vconst,
		const out_val **vregp_or_lbl)
{
	switch(v->type){
		case V_CONST_I:
			*vconst = v;
			break;

		case V_LBL:
		case V_REG:
			*vregp_or_lbl = v;
			break;

		default:
			break;
	}
}

static out_val *try_mem_offset(
		out_ctx *octx,
		enum op_type binop,
		const out_val *vconst, const out_val *vregp_or_lbl,
		const out_val *rhs)
{
	/* if it's a minus, we enforce an order */
	if((binop == op_plus || (binop == op_minus && vconst == rhs))
	&& (vregp_or_lbl->type != V_LBL || (fopt_mode & FOPT_SYMBOL_ARITH)))
	{
		out_val *mut_vregp_or_lbl = v_dup_or_reuse(
				octx, vregp_or_lbl, vregp_or_lbl->t);
		long *p;
		int step;

		switch(mut_vregp_or_lbl->type){
			case V_LBL:
				p = &mut_vregp_or_lbl->bits.lbl.offset;
				break;

			case V_REG:
				p = &mut_vregp_or_lbl->bits.regoff.offset;
				break;

			default:
				assert(0);
		}

		step = calc_ptr_step(mut_vregp_or_lbl->t);
		if(step == -1){
			/* restore retains */
			out_val_retain(octx, vregp_or_lbl);
			out_val_release(octx, mut_vregp_or_lbl);
			return NULL;
		}

		*p += (binop == op_minus ? -1 : 1) *
			vconst->bits.val_i * step;

		out_val_consume(octx, vconst);

		return mut_vregp_or_lbl;
	}

	return NULL;
}

static int const_is_noop(enum op_type binop, const out_val *vconst, int is_lhs)
{
	switch(binop){
		case op_plus:
		case op_or:
		case op_xor:
		case op_shiftl: /* if we're shifting 0, or shifting _by_ zero, noop */
		case op_shiftr:
			return vconst->bits.val_i == 0;

		case op_multiply:
			return vconst->bits.val_i == 1;

		case op_and:
			/* TODO: check all 1s just for the size of vconst->t */
			return (sintegral_t)vconst->bits.val_i == -1;

		/* non-commutative: */
		case op_minus:
			return !is_lhs && vconst->bits.val_i == 0;

		case op_divide:
			return !is_lhs && vconst->bits.val_i == 1;

		default:
			return 0;
	}
}

static out_val *try_const_fold(
		out_ctx *octx,
		enum op_type binop,
		const out_val *lhs, const out_val *rhs)
{
	const char *err = NULL;
	const integral_t eval = const_op_exec(
			lhs->bits.val_i, &rhs->bits.val_i,
			binop, lhs->t,
			&err);

	if(!err){
		type *t = lhs->t;
		out_val_consume(octx, lhs);
		out_val_consume(octx, rhs);
		return out_new_l(octx, t, eval);
	}

	return NULL;
}

static void apply_ptr_step(
		out_ctx *octx,
		const out_val **lhs, const out_val **rhs,
		const out_val **div_out)
{
	int l_ptr = !!type_is((*lhs)->t, type_ptr);
	int r_ptr = !!type_is((*rhs)->t, type_ptr);
	int ptr_step;

	if(!l_ptr && !r_ptr)
		return;

	ptr_step = calc_ptr_step((l_ptr ? *lhs : *rhs)->t);

	if(l_ptr ^ r_ptr){
		/* ptr +/- int, adjust the non-ptr by sizeof *ptr */
		const out_val **incdec = (l_ptr ? rhs : lhs);
		out_val *mut_incdec;

		*incdec = mut_incdec = v_dup_or_reuse(octx, *incdec, (*incdec)->t);

		switch(mut_incdec->type){
			case V_CONST_I:
				if(ptr_step == -1){
					*incdec = out_op(octx, op_multiply,
							*incdec,
							vla_size(
								type_next((l_ptr ? *lhs : *rhs)->t),
								octx));

					mut_incdec = NULL; /* safety */
				}else{
					mut_incdec->bits.val_i *= ptr_step;
				}
				break;

			case V_CONST_F:
				assert(0 && "float pointer inc?");

			case V_LBL:
			case V_FLAG:
			case V_REG_SPILT:
				assert(mut_incdec->retains == 1);
				*incdec = (out_val *)v_to_reg(octx, *incdec);

			case V_REG:
			{
				const out_val *n;
				if(ptr_step == -1){
					n = vla_size(
							type_next((l_ptr ? *lhs : *rhs)->t),
							octx);
				}else{
					n = out_new_l(
						octx,
						type_nav_btype(cc1_type_nav, type_intptr_t),
						ptr_step);
				}

				*incdec = (out_val *)out_op(octx, op_multiply, *incdec, n);
				break;
			}
		}

	}else if(l_ptr && r_ptr){
		/* difference - divide afterwards */
		if(ptr_step == -1){
			*div_out = vla_size(type_next((*lhs)->t), octx);
		}else{
			*div_out = out_new_l(octx,
					type_ptr_to(type_nav_btype(cc1_type_nav, type_void)),
					ptr_step);
		}
	}
}

static void try_shift_conv(
		out_ctx *octx,
		enum op_type *binop,
		const out_val **lhs, const out_val **rhs)
{
	if(*binop == op_divide && (*rhs)->type == V_CONST_I){
		integral_t k = (*rhs)->bits.val_i;
		if((k & (k - 1)) == 0){
			/* power of two, can shift */
			out_val *mut;

			*binop = op_shiftr;

			*rhs = mut = v_dup_or_reuse(octx, *rhs, (*rhs)->t);
			mut->bits.val_i = log2(k);
		}
	}else if(*binop == op_multiply){
		const out_val **vconst = (*lhs)->type == V_CONST_I ? lhs : rhs;
		integral_t k = (*vconst)->bits.val_i;

		if((k & (k - 1)) == 0){
			out_val *mut;

			*binop = op_shiftl;

			*vconst = mut = v_dup_or_reuse(octx, *vconst, (*vconst)->t);
			mut->bits.val_i = log2(k);

			if(vconst == lhs){
				/* need to swap as shift expects the constant to be rhs */
				const out_val *tmp = *lhs;
				*lhs = *rhs;
				*rhs = tmp;
			}
		}
	}
}

static const out_val *consume_one(
		out_ctx *octx,
		const out_val *const ret,
		const out_val *const a,
		const out_val *const b)
{
	out_val_consume(octx, ret == a ? b : a);
	return ret;
}

const out_val *out_op(
		out_ctx *octx, enum op_type binop,
		const out_val *lhs, const out_val *rhs)
{
	const out_val *div = NULL;
	const out_val *vconst = NULL, *vregp_or_lbl = NULL;
	const out_val *result;
	const out_val *decay_except[] = { lhs, rhs, NULL };

	v_decay_flags_except(octx, decay_except); /* an op instruction may change cpu flags */

	fill_if_type(lhs, &vconst, &vregp_or_lbl);
	fill_if_type(rhs, &vconst, &vregp_or_lbl);

	/* check for adding or subtracting to stack */
	if(vconst && vregp_or_lbl){
		result = try_mem_offset(octx, binop, vconst, vregp_or_lbl, rhs);
		if(result)
			return result;
	}

	if(vconst && const_is_noop(binop, vconst, vconst == lhs))
		return consume_one(octx, vconst == lhs ? rhs : lhs, lhs, rhs);

	/* constant folding */
	if(vconst && (fopt_mode & FOPT_CONST_FOLD)){
		const out_val *oconst = (vconst == lhs ? rhs : lhs);

		if(oconst->type == V_CONST_I){
			int step_l = calc_ptr_step(lhs->t);
			int step_r = calc_ptr_step(rhs->t);
			out_val *consted;

			/* currently we bail if something like (short *)0 + 2
			 * is attempted */
			if(step_l == 1 && step_r == 1){
				consted = try_const_fold(octx, binop, lhs, rhs);
				if(consted)
					return consted;
			}
		}
	}

	switch(binop){
		case op_plus:
		case op_minus:
			apply_ptr_step(octx, &lhs, &rhs, &div);
			break;
		case op_multiply:
		case op_divide:
			if(vconst && (fopt_mode & FOPT_TRAPV) == 0)
				try_shift_conv(octx, &binop, &lhs, &rhs);
			break;
		default:
			break;
	}

	result = impl_op(octx, binop, lhs, rhs);

	if(div)
		result = out_op(octx, op_divide, result, div);

	return result;
}

const out_val *out_op_unary(out_ctx *octx, enum op_type uop, const out_val *val)
{
	switch(uop){
		case op_plus:
			return val;

		default:
			break;
	}

	/* special case - reverse the flag if possible */
	switch(val->type){
		case V_FLAG:
			if(uop == op_not){
				out_val *reversed = v_dup_or_reuse(octx, val, val->t);

				reversed->bits.flag.cmp = v_inv_cmp(reversed->bits.flag.cmp, 1);

				return reversed;
			}
			break;

		case V_CONST_I:
			switch(uop){
#define OP(op, tok) \
				case op_ ## op: {                        \
					out_val *dup = v_dup_or_reuse(         \
							octx, val, val->t);                \
					dup->bits.val_i = tok dup->bits.val_i; \
					return dup;                            \
				}

				OP(not, !);
				OP(minus, -);
				OP(bnot, ~);

#undef OP

				default:
				assert(0 && "invalid unary op");
			}
			break;

		default:
			break;
	}

	return impl_op_unary(octx, uop, val);
}
