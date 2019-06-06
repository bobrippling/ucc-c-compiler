#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "../util/util.h"
#include "../util/util.h"

#include "const.h"
#include "sym.h"
#include "expr.h"
#include "cc1.h"
#include "type_is.h"

const char *constyness_strs[] = {
	"CONST_NO",
	"CONST_NUM",
	"CONST_ADDR",
	"CONST_STRK",
	"CONST_NEED_ADDR"
};

void const_fold(expr *e, consty *k)
{
	UCC_ASSERT(e->tree_type,
			"const_fold on %s before fold",
			e->f_str());

	memset(k, 0, sizeof *k);
	k->type = CONST_NO;

	if(e->f_const_fold){
		if(!e->const_eval.const_folded){
			int should_have_lbl;

			e->const_eval.const_folded = 1;
			e->f_const_fold(e, &e->const_eval.k);
			e->const_eval.const_folded = 2;

			should_have_lbl = (e->const_eval.k.type == CONST_ADDR || e->const_eval.k.type == CONST_NEED_ADDR)
				&& e->const_eval.k.bits.addr.lbl_type != CONST_LBL_MEMADDR;

			if(should_have_lbl){
				assert(e->const_eval.k.bits.addr.bits.lbl);
			}
		}else if(e->const_eval.const_folded == 1){
			ICW("const-folding a value during its own const-fold");
		}

		memcpy_safe(k, &e->const_eval.k);
	}

	if(k->type == CONST_NO && !k->nonconst)
		k->nonconst = e;
}

void const_fold_no(
		consty *k,
		consty *klhs, expr *lhs,
		consty *krhs, expr *rhs)
{
	k->type = CONST_NO;

	k->nonconst = CONST_AT_COMPILE_TIME(klhs->type)
		? (krhs->nonconst ? krhs->nonconst : rhs)
		: (klhs->nonconst ? klhs->nonconst : lhs);
}

#if 0
int const_expr_is_const(expr *e)
{
	/* TODO: move this to the expr ops */
	if(expr_kind(e, cast))
		return const_expr_is_const(e->expr);

	if(expr_kind(e, val) || expr_kind(e, sizeof) || expr_kind(e, addr))
		return 1;

	/* function names are */
	if(expr_kind(e, identifier) && decl_is_func(e->tree_type))
		return 1;

	/*
	 * const int i = 5; is const
	 * but
	 * const int i = *p; is not const
	 */
	return 0;
	if(e->sym)
		return decl_is_const(e->sym->decl);

	return decl_is_const(e->tree_type);
}
#endif

static int const_expr_zero(expr *e, int zero)
{
	consty k;

	const_fold(e, &k);

	switch(k.type){
		case CONST_NUM:
			if(K_FLOATING(k.bits.num))
				return !k.bits.num.val.f == zero;
			return !k.bits.num.val.i == zero;

		case CONST_ADDR:
			switch(k.bits.addr.lbl_type){
				case CONST_LBL_MEMADDR:
					return k.bits.addr.bits.memaddr == 0;
				case CONST_LBL_TRUE:
				case CONST_LBL_WEAK: /* might be zero, don't know. error caught elsewhere */
					return 0;
			}

		case CONST_NEED_ADDR:
		case CONST_STRK:
		case CONST_NO:
			break;
	}

	return 0;
}

int const_fold_integral_try(expr *e, numeric *piv)
{
	consty k;
	const_fold(e, &k);

	if(k.type != CONST_NUM)
		return 0;
	if(k.offset != 0)
		return 0;
	if(!K_INTEGRAL(k.bits.num))
		return 0;

	memcpy_safe(piv, &k.bits.num);
	return 1;
}

void const_fold_integral(expr *e, numeric *piv)
{
	UCC_ASSERT(const_fold_integral_try(e, piv), "not an integer constant");
}

integral_t const_fold_val_i(expr *e)
{
	numeric num;
	const_fold_integral(e, &num);
	return num.val.i;
}

int const_expr_and_non_zero(expr *e)
{
	return const_expr_zero(e, 0);
}

int const_expr_and_zero(expr *e)
{
	return const_expr_zero(e, 1);
}

/*
long const_expr_value(expr *e)
{
	enum constyness k;
	numeric val;

	const_fold(e, &val, &k);
	UCC_ASSERT(k == CONST_WITH_VAL, "not a constant");

	return val.val;
}
*/

integral_t const_op_exec(
		integral_t lval, const integral_t *rval, /* rval is optional */
		enum op_type op, type *arithty,
		const char **error)
{
	typedef sintegral_t S;
	typedef  integral_t U;
	const int is_signed = type_is_signed(arithty);
	U result;

#define S_OP(o) ((S)lval o (S)*rval)
#define U_OP(o) ((U)lval o (U)*rval)

#define OP(  a, b) case a: result = S_OP(b); break
#define OP_U(a, b) case a: result = is_signed ? (U)S_OP(b) : (U)U_OP(b); break

	switch(op){
		OP(op_multiply,   *);
		OP(op_eq,         ==);
		OP(op_ne,         !=);

		OP_U(op_le,       <=);
		OP_U(op_lt,       <);
		OP_U(op_ge,       >=);
		OP_U(op_gt,       >);

		OP_U(op_shiftl,   <<);
		OP_U(op_shiftr,   >>);

		OP(op_xor,        ^);
		OP(op_or,         |);
		OP(op_and,        &);
		OP(op_orsc,       ||);
		OP(op_andsc,      &&);

		case op_modulus:
		case op_divide:
			if(*rval){
				if(is_signed){
					/* need sign-extended division */
					result = (op == op_divide
							? (sintegral_t)lval / (sintegral_t)*rval
							: (sintegral_t)lval % (sintegral_t)*rval);
				}else{
					result = op == op_divide
						? lval / *rval
						: lval % *rval;
				}
			}else{
				*error = "division by zero";
				result = 0;
			}
			break;

		case op_plus:
			result = lval + (rval ? *rval : 0);
			break;

		case op_minus:
			result = rval ? lval - *rval : -lval;
			break;

		case op_not:  result = !lval; break;
		case op_bnot: result = ~lval; break;

		case op_unknown:
			ICE("unhandled type");
			break;
	}

	if(!is_signed){
		/* need to apply proper wrap-around */
		integral_t max = type_max_assert(arithty);

		if(max == NUMERIC_T_MAX){
			/* handling integral_t:s, already done */
		}else if(result > max){
			if(op_increases(op)){
				/* result = 256 -> 256 - (255 + 1) -> 0 */

				result = result - (max + 1);
			}else{
				/* (assuming integral_t max = 65536)
				 * result = 65536 -> 65536 - 65536 + 256 -> 256 */

				result = result - NUMERIC_T_MAX + max;
			}
		}
	}

	return result;
}

floating_t const_op_exec_fp(
		floating_t lv, const floating_t *rv,
		enum op_type op)
{
	switch(op){
		case op_orsc:
		case op_andsc:
		case op_unknown:
			/* should've been elsewhere */
		case op_modulus:
		case op_xor:
		case op_or:
		case op_and:
		case op_shiftl:
		case op_shiftr:
		case op_bnot:
			/* explicitly bad */
			ICE("floating point %s", op_to_str(op));

		case op_divide:
			if(lv == 0 && *rv == 0){
				/* Special case 0/0, as this is a common way of defining NaN in libcs
				 * when GNU __builtin_nan support isn't detected.
				 *
				 * We explicitly return NAN here as this isn't affected by the rounding
				 * mode, whereas performing the calculation could return -nan.
				 */
				return NAN;
			}
			return lv / *rv; /* safe - / 0.0f is inf */

		case op_multiply: return lv * *rv;
		case op_plus:     return rv ? lv + *rv : +lv;
		case op_minus:    return rv ? lv - *rv : -lv;
		case op_eq:       return lv == *rv;
		case op_ne:       return lv != *rv;
		case op_le:       return lv <= *rv;
		case op_lt:       return lv <  *rv;
		case op_ge:       return lv >= *rv;
		case op_gt:       return lv >  *rv;

		case op_not:
			UCC_ASSERT(!rv, "binary not?");
			return !lv;
	}

	ucc_unreach(-1);
}

static void const_intify(consty *k, expr *owner)
{
	switch(k->type){
		case CONST_NO:
			CONST_FOLD_NO(k, owner);
		case CONST_NUM:
			break;

		case CONST_STRK:
			break;

		case CONST_NEED_ADDR:
		case CONST_ADDR:
		{
			integral_t memaddr;

			switch(k->bits.addr.lbl_type){
				case CONST_LBL_TRUE:
				case CONST_LBL_WEAK:
					/* can't do (int)&x */
					return;
				case CONST_LBL_MEMADDR:
					break;
			}

			memaddr = k->bits.addr.bits.memaddr + k->offset;

			CONST_FOLD_LEAF(k);

			k->type = CONST_NUM;
			k->bits.num.val.i = memaddr;
			break;
		}
	}
}

static void const_memify(consty *k)
{
	switch(k->type){
		case CONST_STRK:
		case CONST_NEED_ADDR:
		case CONST_ADDR:
			break;

		case CONST_NO:
			break;

		case CONST_NUM:
		{
			integral_t memaddr = k->bits.num.val.i + k->offset;

			CONST_FOLD_LEAF(k);

			k->type = CONST_ADDR;
			k->bits.addr.bits.memaddr = memaddr;
			break;
		}
	}
}

static int is_lvalue_pointerish(type *t)
{
	return type_is_ptr(t) || type_is_array(t) || type_is_func_or_block(t);
}

void const_ensure_num_or_memaddr(
		consty *k, type *from, type *to,
		expr *owner, int set_nonstandard_const)
{
	const int from_ptr = is_lvalue_pointerish(from);
	const int to_ptr = is_lvalue_pointerish(to);

	if(from_ptr == to_ptr)
		return;

	if(from_ptr && !to_ptr){
		/* casting from pointer to int */
		const_intify(k, owner);

	}else{
		assert(to_ptr && !from_ptr);

		const_memify(k);
	}

	if(from_ptr
	&& !to_ptr
	&& type_size_assert(from) > type_size_assert(to))
	{
		/* not a constant but we treat it as such, as an extension */
		if(set_nonstandard_const && !k->nonstandard_const)
			k->nonstandard_const = owner;
	}
}
