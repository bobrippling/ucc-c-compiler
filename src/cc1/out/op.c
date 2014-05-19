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

static int calc_ptr_step(type *t)
{
	/* we are calculating the sizeof *t */

	if(type_is_primitive(type_is_ptr(t), type_void))
		return type_primitive_size(type_void);

	if(type_is_primitive(t, type_unknown))
		return 1;

	return type_size(type_next(t), NULL);
}

static void fill_if_type(
		out_val *v,
		out_val **vconst,
		out_val **vregp_or_lbl)
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

static int try_mem_offset(
		enum op_type binop,
		out_val *vconst, out_val *vregp_or_lbl,
		out_val *rhs)
{
	/* if it's a minus, we enforce an order */
	if((binop == op_plus || (binop == op_minus && vconst == rhs))
	&& (vregp_or_lbl->type != V_LBL || (fopt_mode & FOPT_SYMBOL_ARITH)))
	{
		long *p;
		switch(vregp_or_lbl->type){
			case V_LBL:
				p = &vregp_or_lbl->bits.lbl.offset;
				break;

			case V_REG:
				p = &vregp_or_lbl->bits.regoff.offset;
				break;

			default:
				assert(0);
		}

		*p += (binop == op_minus ? -1 : 1) *
			vconst->bits.val_i *
			calc_ptr_step(vregp_or_lbl->t);

		return 1;
	}

	return 0;
}

static int const_is_noop(enum op_type binop, out_val *vconst, int is_lhs)
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
		out_val *lhs, out_val *rhs)
{
	const char *err = NULL;
	const integral_t eval = const_op_exec(
			lhs->bits.val_i, &rhs->bits.val_i,
			binop, type_is_signed(lhs->t),
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
		out_val **lhs, out_val **rhs,
		int *div_out)
{
	int l_ptr = !!type_is((*lhs)->t, type_ptr);
	int r_ptr = !!type_is((*rhs)->t, type_ptr);
	int ptr_step;

	if(!l_ptr && !r_ptr)
		return;

	ptr_step = calc_ptr_step((l_ptr ? *lhs : *rhs)->t);

	if(l_ptr ^ r_ptr){
		/* ptr +/- int, adjust the non-ptr by sizeof *ptr */
		out_val **incdec = (l_ptr ? rhs : lhs);
		*incdec = v_dup_or_reuse(octx, *incdec, (*incdec)->t);

		switch((*incdec)->type){
			case V_CONST_I:
				(*incdec)->bits.val_i *= ptr_step;
				break;

			case V_CONST_F:
				assert(0 && "float pointer inc?");

			case V_LBL:
			case V_FLAG:
			case V_REG_SPILT:
				*incdec = v_to_reg(octx, *incdec);

			case V_REG:
			{
				out_val *n = out_new_l(
						octx,
						type_nav_btype(cc1_type_nav, type_intptr_t),
						ptr_step);
				out_val *mult;

				mult = out_op(octx, op_multiply, *incdec, n);
				*incdec = mult;
				break;
			}
		}

	}else if(l_ptr && r_ptr){
		/* difference - divide afterwards */
		*div_out = ptr_step;
	}
}

static void try_shift_conv(
		out_ctx *octx,
		enum op_type *binop,
		out_val **lhs, out_val **rhs)
{
	if(*binop == op_divide && (*rhs)->type == V_CONST_I){
		integral_t k = (*rhs)->bits.val_i;
		if((k & (k - 1)) == 0){
			/* power of two, can shift */
			*binop = op_shiftr;

			*rhs = v_dup_or_reuse(octx, *rhs, (*rhs)->t);
			(*rhs)->bits.val_i = log2(k);
		}
	}else if(*binop == op_multiply){
		out_val **vconst = (*lhs)->type == V_CONST_I ? lhs : rhs;
		integral_t k = (*vconst)->bits.val_i;

		if((k & (k - 1)) == 0){
			*binop = op_shiftl;

			*vconst = v_dup_or_reuse(octx, *vconst, (*vconst)->t);
			(*vconst)->bits.val_i = log2(k);
		}
	}
}

static out_val *consume_one(
		out_ctx *octx,
		out_val *const ret,
		out_val *const a,
		out_val *const b)
{
	out_val_consume(octx, ret == a ? b : a);
	return ret;
}

out_val *out_op(out_ctx *octx, enum op_type binop, out_val *lhs, out_val *rhs)
{
	int div = 0;
	out_val *vconst = NULL, *vregp_or_lbl = NULL;
	out_val *result;

	fill_if_type(lhs, &vconst, &vregp_or_lbl);
	fill_if_type(rhs, &vconst, &vregp_or_lbl);

	/* check for adding or subtracting to stack */
	if(vconst && vregp_or_lbl
	&& try_mem_offset(binop, vconst, vregp_or_lbl, rhs))
	{
		return consume_one(octx, vregp_or_lbl, lhs, rhs);
	}

	if(vconst && const_is_noop(binop, vconst, vconst == lhs))
		return consume_one(octx, vconst == lhs ? rhs : lhs, lhs, rhs);

	/* constant folding */
	if(vconst){
		out_val *oconst = (vconst == lhs ? rhs : lhs);

		if(oconst->type == V_CONST_I){
			int step_l = calc_ptr_step(lhs->t);
			int step_r = calc_ptr_step(rhs->t);
			out_val *consted;

			/* TODO: ptr step as a separate stage in this function
			 * current we bail if something like (short *)0 + 2
			 * is attempted
			 */
			assert(step_l == 1 && step_r == 1);

			consted = try_const_fold(octx, binop, lhs, rhs);
			if(consted)
				return consume_one(octx, consted, lhs, rhs);
		}
	}

	switch(binop){
		case op_plus:
		case op_minus:
			apply_ptr_step(octx, &lhs, &rhs, &div);
			break;
		case op_multiply:
		case op_divide:
			if(vconst)
				try_shift_conv(octx, &binop, &lhs, &rhs);
			break;
		default:
			break;
	}

	result = impl_op(octx, binop, lhs, rhs);

	if(div){
		result = out_op(
				octx, op_divide,
				result,
				out_new_l(
					octx,
					type_ptr_to(type_nav_btype(cc1_type_nav, type_void)),
					div));
	}

	return result;
}

out_val *out_op_unary(out_ctx *octx, enum op_type uop, out_val *val)
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
