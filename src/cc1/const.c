#include <stdio.h>
#include <stdarg.h>

#include "../util/util.h"
#include "tree.h"
#include "const.h"
#include "sym.h"
#include "../util/util.h"
#include "cc1.h"

int operate(expr *lhs, expr *rhs, enum op_type op, int *bad)
{
#define OP(a, b) case a: return lhs->val.i.val b rhs->val.i.val
	if(op != op_deref && lhs->type != expr_val){
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
			if(lhs->ptr_safe && lhs->type == expr_addr){
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
			if(e->lhs->type == expr_val || e->rhs->type == expr_val)
				POSSIBLE_OPT(e, "short circuit const");
			break;

#define VAL(e, x) (e->type == expr_val && e->val.i.val == x)

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

	switch(e->type){
		case expr_val:
		case expr_sizeof:
		case expr_addr:
			return 0;

		case expr_cast:
			return const_fold(e->rhs);

		case expr_comma:
			return !const_fold(e->lhs) && !const_fold(e->rhs);

		case expr_assign: /* could check if the assignment subtree is const */
		case expr_funcall: /* could extend to have int x() const; */
			return 1;

		case expr_identifier:
			if(e->sym && e->sym->decl->type->spec & spec_const){
				/*
				 * TODO
				 * fold. need to hunt for assignment tree
				 */
				//fprintf(stderr, "TODO: fold expression with const identifier %s\n", e->spel);
			}
			return 1;

		case expr_if:
			if(!const_fold(e->expr) && (e->lhs ? !const_fold(e->lhs) : 1) && !const_fold(e->rhs)){
				e->type = expr_val;
				e->val.i.val = e->expr->val.i.val ? (e->lhs ? e->lhs->val.i.val : e->expr->val.i.val) : e->rhs->val.i.val;
				return 0;
			}
			break;

		case expr_op:
		{
			int l, r;
			l = const_fold(e->lhs);
			r = e->rhs ? const_fold(e->rhs) : 0;

#define VAL(x) x->type == expr_val

			if(!l && !r && VAL(e->lhs) && (e->rhs ? VAL(e->rhs) : 1)){
				int bad = 0;

				e->val.i.val = operate(e->lhs, e->rhs, e->op, &bad);

				if(!bad)
					e->type = expr_val;
				/*
				 * TODO: else free e->[lr]hs
				 * currently not a leak, just ignored
				 */

				return bad;
			}else{
				operate_optimise(e);
			}
#undef VAL
		}
	}
	return 1;
}

int const_expr_is_const(expr *e)
{
	if(e->type == expr_val || e->type == expr_sizeof || e->type == expr_addr)
		return 1;

	/* can't have a const+extern in a constant expression */
	return 0; /*e->sym ? (e->sym->decl->type->spec & spec_const && (e->sym->decl->type->spec & spec_extern) == 0) : 0;
							the above needs to be added in when *const parsing is done */
}

int const_expr_val(expr *e)
{
	switch(e->type){
		case expr_val:
			return e->val.i.val; /* FIXME: doesn't account for longs */

		case expr_sizeof:
			ICE("TODO: const_expr_val with sizeof");

		case expr_addr:
			die_at(&e->where, "address of expression can't be resolved at compile-time");

		default:
			break;
	}

	ICE("const_expr_val on non-const expr");
	return 0;
}

int const_expr_is_zero(expr *e)
{
	if(e->type == expr_cast)
		return const_expr_is_zero(e->rhs);

	return const_expr_is_const(e) && (e->type == expr_val ? e->val.i.val == 0 : 0);
}
