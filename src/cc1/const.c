#include <stdio.h>
#include <stdarg.h>

#include "../util/util.h"
#include "tree.h"
#include "const.h"
#include "sym.h"
#include "../util/util.h"

int operate(expr *lhs, expr *rhs, enum op_type op, int *bad)
{
#define OP(a, b) case a: return lhs->val.i b rhs->val.i
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
			if(rhs->val.i)
				return lhs->val.i / rhs->val.i;
			fprintf(stderr, "%s: division by zero\n", where_str(&rhs->where));
			*bad = 1;
			return 0;

		case op_plus:
			if(rhs)
				return lhs->val.i + rhs->val.i;
			return lhs->val.i;

		case op_minus:
			if(rhs)
				return lhs->val.i - rhs->val.i;
			return -lhs->val.i;

		case op_not:  return !lhs->val.i;
		case op_bnot: return ~lhs->val.i;

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

		case op_unknown:
			break;
	}

	ICE("unhandled asm operate type");
	return 0;
}

/* returns 0 if successfully folded */
int const_fold(expr *e)
{
	switch(e->type){
		case expr_val:
		case expr_sizeof:
		case expr_cast:
		case expr_addr:
			return 0;

		case expr_comma:
			return !const_fold(e->lhs) && !const_fold(e->rhs);

		case expr_assign:
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
				e->val.i = e->expr->val.i ? (e->lhs ? e->lhs->val.i : e->expr->val.i) : e->rhs->val.i;
				return 0;
			}
			break;

		case expr_op:
		{
			int l, r;
			l = const_fold(e->lhs);
			r = e->rhs ? const_fold(e->rhs) : 0;

			if(!l && !r){
				int bad = 0;

				e->val.i = operate(e->lhs, e->rhs, e->op, &bad);

				if(!bad)
					e->type = expr_val;
				/*
				 * TODO: else free e->[lr]hs
				 * currently not a leak, just ignored
				 */

				return bad;
			}else if(e->op == op_orsc || e->op == op_andsc){
				/* check if one side is (&& ? false : true) and short circuit it without needing to check the other side */
			}
		}
	}
	return 1;
}

int const_expr_is_const(expr *e)
{
	if(e->type == expr_val || e->type == expr_sizeof || e->type == expr_addr)
		return 1;

	/* can't have a const+extern in a constant expression */
	return e->sym ? (e->sym->decl->type->spec & spec_const && (e->sym->decl->type->spec & spec_extern) == 0) : 0;
}

int const_expr_is_zero(expr *e)
{
	if(e->type == expr_cast)
		return const_expr_is_zero(e->rhs);

	return const_expr_is_const(e) && (e->type == expr_val ? e->val.i == 0 : 0);
}
