#include <string.h>
#include <stdlib.h>
#include "ops.h"
#include "expr_op.h"
#include "../out/lbl.h"

const char *str_expr_op()
{
	return "op";
}

static void operate(
		intval *lval, intval *rval,
		enum op_type op,
		intval *piv, enum constyness *pconst_type,
		where *where)
{
	/* FIXME: casts based on lval.type */
#define OP(a, b) case a: piv->val = lval->val b rval->val; return
	switch(op){
		OP(op_multiply,   *);
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

		case op_modulus:
		case op_divide:
		{
			long l = lval->val, r = rval->val;

			if(r){
				piv->val = op == op_divide ? l / r : l % r;
				return;
			}
			warn_at(where, 1, "division by zero");
			*pconst_type = CONST_NO;
			return;
		}

		case op_plus:
			piv->val = lval->val + (rval ? rval->val : 0);
			return;

		case op_minus:
			piv->val = rval ? lval->val - rval->val : -lval->val;
			return;

		case op_not:  piv->val = !lval->val; return;
		case op_bnot: piv->val = ~lval->val; return;

		case op_unknown:
			break;
	}

	ICE("unhandled type");
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

#define VAL(e, x) (expr_kind(e, val) && e->val.iv.val == x)

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

void fold_const_expr_op(expr *e, intval *piv, enum constyness *pconst_type)
{
	intval lhs, rhs;
	enum constyness l_const, r_const;

	const_fold(e->lhs, &lhs, &l_const);
	if(e->rhs){
		const_fold(e->rhs, &rhs, &r_const);
	}else{
		r_const = CONST_WITH_VAL;
	}

	if(l_const == CONST_WITH_VAL && r_const == CONST_WITH_VAL){
		*pconst_type = CONST_WITH_VAL;
		operate(&lhs, e->rhs ? &rhs : NULL, e->op, piv, pconst_type, &e->where);
	}else{
		*pconst_type = CONST_WITHOUT_VAL;
		operate_optimise(e);
	}
}

void expr_promote_int(expr **pe, enum type_primitive to, symtable *stab)
{
	expr *const e = *pe;
	expr *cast;

	if(decl_is_ptr(e->tree_type)){
		UCC_ASSERT(to == type_intptr_t, "invalid promotion for pointer");
		return;
	}

	UCC_ASSERT(!e->tree_type->desc, "invalid type to promote");

	/* if(type_primitive_size(e->tree_type->type->primitive) >= type_primitive_size(to))
	 *   return;
	 *
	 * insert down-casts too - the tree_type of the expression is still important
	 */

  cast = expr_new_cast(decl_new_type(to), 1);

	cast->expr = e;

	fold_expr_cast_descend(cast, stab, 0);

	*pe = cast;
}

decl *op_required_promotion(
		enum op_type op,
		expr *lhs, expr *rhs,
		where *w,
		decl **plhs, decl **prhs)
{
	decl *resolved = NULL;
	decl *const dlhs = lhs->tree_type, *const drhs = rhs->tree_type;

	*plhs = *prhs = NULL;

#if 0
	If either operand is a pointer:
		relation: both must be pointers

	if both operands are pointers:
		must be '-'

	exactly one pointer:
		must be + or -
#endif

	if(decl_is_void(dlhs) || decl_is_void(drhs))
		DIE_AT(w, "use of void expression");

	{
		const int l_ptr = decl_is_ptr(dlhs);
		const int r_ptr = decl_is_ptr(drhs);

		if(l_ptr && r_ptr){
			if(op == op_minus){
				resolved = decl_new_type(type_ptrdiff_t);
			}else if(op_is_relational(op)){
				resolved = decl_new_type(type_int);
			}else{
				DIE_AT(w, "operation between two pointers must be relational or subtraction");
			}

			goto fin;

		}else if(l_ptr){
			/* + or - */
			if(op != op_plus && op != op_minus)
				DIE_AT(w, "operation between pointer and integer must be + or -");

			resolved = decl_copy(l_ptr ? dlhs : drhs);

			/* FIXME: promote to unsigned */
			*(l_ptr ? prhs : plhs) = decl_new_type(op == op_plus ? type_intptr_t : type_ptrdiff_t);

			goto fin;

		}else if(r_ptr){
			DIE_AT(w, "invalid %s between integer and pointer", op_to_str(op));

		}
	}

#if 0
	If both operands have the same type, then no further conversion is needed.

	Otherwise, if both operands have signed integer types or both have unsigned
	integer types, the operand with the type of lesser integer conversion rank
	is converted to the type of the operand with greater rank.

	Otherwise, if the operand that has unsigned integer type has rank greater
	or equal to the rank of the type of the other operand, then the operand
	with signed integer type is converted to the type of the operand with
	unsigned integer type.

	Otherwise, if the type of the operand with signed integer type can
	represent all of the values of the type of the operand with unsigned
	integer type, then the operand with unsigned integer type is converted to
	the type of the operand with signed integer type.

	Otherwise, both operands are converted to the unsigned integer type
	corresponding to the type of the operand with signed integer type.
#endif

	{
		decl *dlarger = NULL;

		if(op == op_shiftl || op == op_shiftr){
			/* fine with any parameter sizes - don't need to match. resolves to lhs */
			dlarger = dlhs;

		}else if(dlhs->type->primitive != drhs->type->primitive){
			const int l_larger = decl_size(dlhs) > decl_size(drhs);
			char bufa[DECL_STATIC_BUFSIZ], bufb[DECL_STATIC_BUFSIZ];

			/* TODO: needed? */
			fold_decl_equal(dlhs, drhs,
					w, WARN_COMPARE_MISMATCH,
					"mismatching types in %s (%s and %s)",
					op_to_str(op),
					decl_to_str_r(bufa, dlhs),
					decl_to_str_r(bufb, drhs));

			*(l_larger ? prhs : plhs) = (l_larger ? dlhs : drhs);

			dlarger = l_larger ? dlhs : drhs;

		}else{
			/* default to either */
			dlarger = dlhs;
		}

		/* if we have a _comparison_ (e.g. between enums), convert to int */
		resolved = op_is_relational(op)
			? decl_new_type(type_int)
			: decl_copy(dlarger);
	}

fin:
	UCC_ASSERT(resolved, "no decl from type promotion");

	UCC_ASSERT(!!*plhs + !!*prhs < 2, "can't cast both expressions");

	return resolved;
}

decl *op_promote_types(
		enum op_type op,
		const char *desc,
		expr **plhs, expr **prhs,
		where *w, symtable *stab)
{
	decl *dlhs, *drhs;
	decl *resolved;

	resolved = op_required_promotion(op, *plhs, *prhs, w, &dlhs, &drhs);

	if(dlhs)
		fold_insert_casts(dlhs, plhs, stab, w, desc);
	else if(drhs)
		fold_insert_casts(drhs, prhs, stab, w, desc);

	return resolved;
}

void fold_expr_op(expr *e, symtable *stab)
{
	UCC_ASSERT(e->op != op_unknown, "unknown op in expression at %s",
			where_str(&e->where));

	FOLD_EXPR(e->lhs, stab);
	fold_disallow_st_un(e->lhs, "op-lhs");

	if(e->rhs){
		FOLD_EXPR(e->rhs, stab);
		fold_disallow_st_un(e->rhs, "op-rhs");

		e->tree_type = op_promote_types(e->op, op_to_str(e->op),
				&e->lhs, &e->rhs, &e->where, stab);
	}else{
		/* (except unary-not) can only have operations on integers,
		 * promote to signed int
		 */

		if(e->op == op_not){
			e->tree_type = decl_new_int();

		}else{
			decl *d_unary = e->lhs->tree_type;

			if(!decl_is_integral(d_unary) && !decl_is_floating(d_unary))
				DIE_AT(&e->where, "type '%s' to unary %s",
						decl_to_str(d_unary), op_to_str(e->op));

			/* extend to int if smaller */
			if(decl_size(d_unary) < type_primitive_size(type_int)){
				expr_promote_int(&e->lhs, type_int, stab);
				d_unary = e->lhs->tree_type;
			}

			e->tree_type = decl_copy(d_unary);
		}
	}
}

void gen_expr_str_op(expr *e, symtable *stab)
{
	(void)stab;
	idt_printf("op: %s\n", op_to_str(e->op));
	gen_str_indent++;

#define PRINT_IF(hs) if(e->hs) print_expr(e->hs)
	PRINT_IF(lhs);
	PRINT_IF(rhs);
#undef PRINT_IF

	gen_str_indent--;
}

static void op_shortcircuit(expr *e, symtable *tab)
{
	char *bail = out_label_code("shortcircuit_bail");

	gen_expr(e->lhs, tab);

	out_dup();
	(e->op == op_andsc ? out_jfalse : out_jtrue)(bail);
	out_pop();

	gen_expr(e->rhs, tab);

	out_label(bail);
	free(bail);

	out_normalise();
}

void gen_expr_op(expr *e, symtable *tab)
{
	switch(e->op){
		case op_orsc:
		case op_andsc:
			op_shortcircuit(e, tab);
			break;

		case op_unknown:
			ICE("asm_operate: unknown operator got through");

		default:
			gen_expr(e->lhs, tab);

			if(e->rhs){
				gen_expr(e->rhs, tab);

				out_op(e->op);
			}else{
				out_op_unary(e->op);
			}
	}
}

void mutate_expr_op(expr *e)
{
	e->f_const_fold = fold_const_expr_op;
}

expr *expr_new_op(enum op_type op)
{
	expr *e = expr_new_wrapper(op);
	e->op = op;
	return e;
}

void gen_expr_style_op(expr *e, symtable *stab)
{ (void)e; (void)stab; /* TODO */ }
