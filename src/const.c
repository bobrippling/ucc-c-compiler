#include <stdio.h>

#include "tree.h"
#include "const.h"
#include "sym.h"

int operate(expr *lhs, expr *rhs, enum op_type op, int *bad)
{
#define OP(a, b) case a: return lhs->val b rhs->val
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

		case op_divide:
			if(!rhs->val)
				fprintf(stderr, "%s: division by zero\n", where_str(&rhs->where));
			else
				return lhs->val / rhs->val;
			*bad = 1;
			return 0;

		case op_plus:
			if(rhs)
				return lhs->val + rhs->val;
			return lhs->val;

		case op_minus:
			if(rhs)
				return lhs->val - rhs->val;
			return -lhs->val;

		case op_not:  return !lhs->val;
		case op_bnot: return !lhs->val;

		case op_deref:
			/* check if we're directly derefing a string */
			if(lhs->type == expr_str)
				return *lhs->spel;
			*bad = 1;
			return 0;

		case op_unknown:
			break;
	}

	fprintf(stderr, "fold error: unknown op\n");
	*bad = 1;
	return 0;
}

/* returns 0 if successfully folded */
int const_fold(expr *e)
{
	switch(e->type){
		case expr_val:
		case expr_sizeof:
		case expr_str:
			return 0;

		case expr_addr:
		case expr_assign:
		case expr_funcall: /* could extend to have int x() const; */
			return 1;

		case expr_identifier:
			if(e->sym && e->sym->decl->spec & spec_const){
				/*
				 * TODO
				 * (needs "const int x = 5;" parsing)
				 * can fold, do so
				 */
				fprintf(stderr, "TODO: fold expression with const identifier %s\n", e->spel);
			}
			return 1;


		case expr_op:
		{
			int l, r;
			l = const_fold(e->lhs);
			r = e->rhs ? const_fold(e->rhs) : 0;

			if(!l && !r){
				int bad = 0;
				e->val = operate(e->lhs, e->rhs, e->op, &bad);

				if(!bad)
					e->type = expr_val;
				/*
				 * TODO: else free e->[lr]hs
				 * currently not a leak, just ignored
				 */

				return bad;
			}
		}
	}
	return 1;
}

int fold_expr_is_const(expr *e)
{
	if(e->type == expr_val || e->type == expr_sizeof || e->type == expr_str)
		return 1;

	/* can't have a const+extern in a constant expression */
	return e->sym ? (e->sym->decl->spec & spec_const && (e->sym->decl->spec & spec_extern) == 0) : 0;
}
