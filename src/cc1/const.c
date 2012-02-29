#include <stdio.h>
#include <stdarg.h>

#include "../util/util.h"
#include "data_structs.h"
#include "const.h"
#include "sym.h"
#include "../util/util.h"
#include "cc1.h"

int operate(expr *lhs, expr *rhs, enum op_type op, int *bad)
{
#define OP(a, b) case a: return lhs->val.i.val b rhs->val.i.val
	if(op != op_deref && !expr_kind(lhs, val)){
		*bad = 1;
		return 0;
	}

	switch(op){
		OP(op_multiply,   *);
		OP(op_modulus,    %);
		OP(op_eq,         ==);
		OP(op_ne,         !=);
		OP(op_le,         <=);
		OP(op_lt,         <);
		OP(op_ge,         >=);
		OP(op_gt,         >);
		OP(op_xor,        ^);
		OP(op_or,         |);
		OP(op_and,        &);
		OP(op_orsc,       ||);
		OP(op_andsc,      &&);
		OP(op_shiftl,     <<);
		OP(op_shiftr,     >>);

		case op_divide:
			if(rhs->val.i.val)
				return lhs->val.i.val / rhs->val.i.val;
			warn_at(&rhs->where, "division by zero");
			*bad = 1;
			return 0;

		case op_plus:
			if(rhs)
				return lhs->val.i.val + rhs->val.i.val;
			return lhs->val.i.val;

		case op_minus:
			if(rhs)
				return lhs->val.i.val - rhs->val.i.val;
			return -lhs->val.i.val;

		case op_not:  return !lhs->val.i.val;
		case op_bnot: return ~lhs->val.i.val;

		case op_deref:
			/*
			 * potential for major ICE here
			 * I mean, we might not even be dereferencing the right size pointer
			 */
			/*
			switch(lhs->vartype->primitive){
				case type_int:  return *(int *)lhs->val.s;
				case type_char: return *       lhs->val.s;
				default:
					break;
			}

			ignore for now, just deal with simple stuff
			*/
			if(lhs->ptr_safe && expr_kind(lhs, addr)){
				if(lhs->array_store->type == array_str)
					return *lhs->val.s;
				/*return lhs->val.exprs[0]->val.i;*/
			}
			*bad = 1;
			return 0;

		case op_struct_ptr:
		case op_struct_dot:
			*bad = 1;
			return 0;

		case op_unknown:
			break;
	}

	ICE("unhandled asm operate type");
	return 0;
}

void operate_optimise(expr *e)
{
	/* TODO */

	switch(e->op){
		case op_orsc:
		case op_andsc:
			/* check if one side is (&& ? false : true) and short circuit it without needing to check the other side */
			if(expr_kind(e->lhs, val) || expr_kind(e->rhs, val))
				POSSIBLE_OPT(e, "short circuit const");
			break;

#define VAL(e, x) (expr_kind(e, val) && e->val.i.val == x)

		case op_plus:
		case op_minus:
			if(VAL(e->lhs, 0) || (e->rhs ? VAL(e->rhs, 0) : 0))
				POSSIBLE_OPT(e, "zero being added or subtracted");
			break;

		case op_multiply:
			if(VAL(e->lhs, 1) || VAL(e->lhs, 0) || (e->rhs ? VAL(e->rhs, 1) || VAL(e->rhs, 0) : 0))
				POSSIBLE_OPT(e, "1 or 0 being multiplied");
			else
		case op_divide:
			if(VAL(e->rhs, 1))
				POSSIBLE_OPT(e, "divide by 1");
			break;

		default:
			break;
#undef VAL
	}
}

/* returns 0 if successfully folded */
int const_fold(expr *e)
{
	if((fopt_mode & FOPT_CONST_FOLD) == 0)
		return 1;

	if(e->f_const_fold)
		return e->f_const_fold(e);

	return 1;
}

int const_expr_is_const(expr *e)
{
	if(expr_kind(e, val) || expr_kind(e, sizeof) || expr_kind(e, addr))
		return 1;

	if(e->sym)
		return decl_is_const(e->sym->decl);

	return decl_is_const(e->tree_type);
}

int const_expr_val(expr *e)
{
	if(expr_kind(e, val))
		return e->val.i.val; /* FIXME: doesn't account for longs */

	if(expr_kind(e, sizeof))
		ICE("TODO: const_expr_val with sizeof");

	if(expr_kind(e, addr))
		die_at(&e->where, "address of expression can't be resolved at compile-time");

	ICE("const_expr_val on non-const expr");
	return 0;
}

int const_expr_is_zero(expr *e)
{
	if(expr_kind(e, cast))
		return const_expr_is_zero(e->rhs);

	return const_expr_is_const(e) && (expr_kind(e, val) ? e->val.i.val == 0 : 0);
}
