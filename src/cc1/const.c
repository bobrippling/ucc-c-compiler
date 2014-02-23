#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "../util/util.h"
#include "../util/util.h"

#include "const.h"
#include "sym.h"
#include "expr.h"
#include "cc1.h"

void const_fold(expr *e, consty *k)
{
	memset(k, 0, sizeof *k);
	k->type = CONST_NO;

	if(e->f_const_fold){
		if(!e->const_eval.const_folded){
			e->const_eval.const_folded = 1;
			e->f_const_fold(e, &e->const_eval.k);
		}

		memcpy_safe(k, &e->const_eval.k);
	}
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

	if(k.type != CONST_NUM)
		return 0;

	if(K_FLOATING(k.bits.num))
		return !k.bits.num.val.f == zero;

	return !k.bits.num.val.i == zero;
}

void const_fold_integral(expr *e, numeric *piv)
{
	consty k;
	const_fold(e, &k);

	UCC_ASSERT(k.type == CONST_NUM, "not const");
	UCC_ASSERT(k.offset == 0, "got offset for val?");
	UCC_ASSERT(K_INTEGRAL(k.bits.num), "fp?");

	memcpy_safe(piv, &k.bits.num);
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
		enum op_type op, int is_signed,
		const char **error)
{
	typedef sintegral_t S;
	typedef  integral_t U;

	/* FIXME: casts based on lval.type */
#define S_OP(o) (S)lval o (S)*rval
#define U_OP(o) (U)lval o (U)*rval

#define OP(  a, b) case a: return S_OP(b)
#define OP_U(a, b) case a: return is_signed ? S_OP(b) : U_OP(b)

	switch(op){
		OP(op_multiply,   *);
		OP(op_eq,         ==);
		OP(op_ne,         !=);

		OP_U(op_le,       <=);
		OP_U(op_lt,       <);
		OP_U(op_ge,       >=);
		OP_U(op_gt,       >);

		OP(op_xor,        ^);
		OP(op_or,         |);
		OP(op_and,        &);
		OP(op_orsc,       ||);
		OP(op_andsc,      &&);
		OP(op_shiftl,     <<);
		OP(op_shiftr,     >>);

		case op_modulus:
		case op_divide:
			if(*rval)
				return op == op_divide ? lval / *rval : lval % *rval;

			*error = "division by zero";
			return 0;

		case op_plus:
			return lval + (rval ? *rval : 0);

		case op_minus:
			return rval ? lval - *rval : -lval;

		case op_not:  return !lval;
		case op_bnot: return ~lval;

		case op_unknown:
			break;
	}

	ICE("unhandled type");
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

		case op_multiply: return lv * *rv;
		case op_divide:   return lv / *rv; /* safe - / 0.0f is inf */
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
