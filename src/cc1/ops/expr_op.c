#include <string.h>
#include <stdlib.h>
#include "ops.h"
#include "expr_op.h"
#include "../out/lbl.h"
#include "../out/asm.h"

const char *str_expr_op()
{
	return "op";
}

static void operate(
		intval *lval, intval *rval,
		enum op_type op,
		consty *konst,
		where *where)
{
	/* FIXME: casts based on lval.type */
#define piv (&konst->bits.iv)
#define OP(a, b) case a: konst->bits.iv.val = lval->val b rval->val; return
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
			konst->type = CONST_NO;
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
#undef piv
}

static void const_offset(consty *r, consty *val, consty *addr,
		type_ref *addr_type, enum op_type op)
{
	unsigned step = type_ref_size(type_ref_next(addr_type), NULL);
	int change;

	memcpy_safe(r, addr);

	change = val->bits.iv.val * step;

	if(op == op_minus)
		change = -change;

	/* may already have an offset, hence += */
	r->offset += change;
}

void fold_const_expr_op(expr *e, consty *k)
{
	consty lhs, rhs;

	const_fold(e->lhs, &lhs);
	if(e->rhs){
		const_fold(e->rhs, &rhs);
	}else{
		memset(&rhs, 0, sizeof rhs);
		rhs.type = CONST_VAL;
	}

	k->type = CONST_NO;

	if(lhs.type == CONST_VAL && rhs.type == CONST_VAL){
		k->type = CONST_VAL;
		operate(&lhs.bits.iv, e->rhs ? &rhs.bits.iv : NULL, e->op, k, &e->where);

	}else if((e->op == op_andsc || e->op == op_orsc)
	&& (CONST_AT_COMPILE_TIME(lhs.type) || CONST_AT_COMPILE_TIME(rhs.type))){

		/* allow 1 || f() */
		consty *kside = CONST_AT_COMPILE_TIME(lhs.type) ? &lhs : &rhs;
		int is_true = !!kside->bits.iv.val;

		if(e->op == (is_true ? op_orsc : op_andsc))
			memcpy(k, kside, sizeof *k);

	}else if(e->op == op_plus || e->op == op_minus){
		/* allow one CONST_{ADDR,STRK} and one CONST_VAL for an offset const */
		int lhs_addr = lhs.type == CONST_ADDR || lhs.type == CONST_STRK;
		int rhs_addr = rhs.type == CONST_ADDR || rhs.type == CONST_STRK;

		/**/if(lhs_addr && rhs.type == CONST_VAL)
			const_offset(k, &rhs, &lhs, e->lhs->tree_type, e->op);
		else if(rhs_addr && lhs.type == CONST_VAL)
			const_offset(k, &lhs, &rhs, e->rhs->tree_type, e->op);
	}
}

void expr_promote_int_if_smaller(expr **pe, symtable *stab)
{
	static unsigned sz_int;
	expr *e = *pe;

	if(!sz_int)
		sz_int = type_primitive_size(type_int);

	if(type_ref_size(e->tree_type, &e->where) < sz_int){
		expr *cast;

		UCC_ASSERT(!type_ref_is(e->tree_type, type_ref_ptr),
				"invalid promotion for pointer");

		/* if(type_primitive_size(e->tree_type->type->primitive) >= type_primitive_size(to))
		 *   return;
		 *
		 * insert down-casts too - the tree_type of the expression is still important
		 */

		cast = expr_new_cast(type_ref_new_type(type_new_primitive(type_int)), 1);

		cast->expr = e;

		fold_expr_cast_descend(cast, stab, 0);

		*pe = cast;
	}
}

type_ref *op_required_promotion(
		enum op_type op,
		expr *lhs, expr *rhs,
		where *w,
		type_ref **plhs, type_ref **prhs)
{
	type_ref *resolved = NULL;
	type_ref *const tlhs = lhs->tree_type, *const trhs = rhs->tree_type;

	*plhs = *prhs = NULL;

#if 0
	If either operand is a pointer:
		relation: both must be pointers

	if both operands are pointers:
		must be '-'

	exactly one pointer:
		must be + or -
#endif

	if(type_ref_is_void(tlhs) || type_ref_is_void(trhs))
		DIE_AT(w, "use of void expression");

	{
		const int l_ptr = !!type_ref_is(tlhs, type_ref_ptr);
		const int r_ptr = !!type_ref_is(trhs, type_ref_ptr);

		if(l_ptr && r_ptr){
			if(op == op_minus){
				resolved = type_ref_cached_INTPTR_T();
			}else if(op_is_relational(op)){
ptr_relation:
				if(op_is_comparison(op)){
					char buf[TYPE_REF_STATIC_BUFSIZ];

					if(!fold_type_ref_equal(tlhs, trhs, w,
							WARN_COMPARE_MISMATCH, 0,
							l_ptr && r_ptr
							? "comparison of distinct pointer types lacks a cast (%s vs %s)"
							: "comparison between pointer and integer (%s vs %s)",
							type_ref_to_str(tlhs), type_ref_to_str_r(buf, trhs)))
					{
						/* not equal - ptr vs int */
						*(l_ptr ? prhs : plhs) = type_ref_cached_INTPTR_T();
					}
				}

				resolved = type_ref_cached_INT();

			}else{
				DIE_AT(w, "operation between two pointers must be relational or subtraction");
			}

			goto fin;

		}else if(l_ptr || r_ptr){
			/* + or - */

			/* cmp between pointer and integer - missing cast */
			if(op_is_relational(op))
				goto ptr_relation;

			switch(op){
				default:
					DIE_AT(w, "operation between pointer and integer must be + or -");

				case op_minus:
					if(!l_ptr)
						DIE_AT(w, "subtraction of pointer from integer");

				case op_plus:
					break;
			}

			resolved = l_ptr ? tlhs : trhs;

			/* FIXME: promote to unsigned */
			*(l_ptr ? prhs : plhs) = type_ref_cached_INTPTR_T();

			goto fin;
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
		type_ref *tlarger = NULL;

		if(op == op_shiftl || op == op_shiftr){
			/* fine with any parameter sizes - don't need to match. resolves to lhs */
			tlarger = tlhs;

		}else{
			const int l_unsigned = !type_ref_is_signed(tlhs),
			          r_unsigned = !type_ref_is_signed(trhs);

			const int l_sz = type_ref_size(tlhs, &lhs->where),
			          r_sz = type_ref_size(trhs, &rhs->where);

			if(l_unsigned == r_unsigned){
				if(l_sz != r_sz){
					const int l_larger = l_sz > r_sz;
					char bufa[TYPE_REF_STATIC_BUFSIZ], bufb[TYPE_REF_STATIC_BUFSIZ];

					/* TODO: needed? */
					fold_type_ref_equal(tlhs, trhs,
							w, WARN_COMPARE_MISMATCH, 0,
							"mismatching types in %s (%s and %s)",
							op_to_str(op),
							type_ref_to_str_r(bufa, tlhs),
							type_ref_to_str_r(bufb, trhs));

					*(l_larger ? prhs : plhs) = (l_larger ? tlhs : trhs);

					tlarger = l_larger ? tlhs : trhs;

				}else{
					/* default to either */
					tlarger = tlhs;
				}

			}else if(l_unsigned ? l_sz >= r_sz : r_sz >= l_sz){
				if(l_unsigned)
					tlarger = *prhs = tlhs;
				else
					tlarger = *plhs = trhs;

			}else if(l_unsigned ? r_sz > l_sz : l_sz > r_sz){
				/* can the signed type represent all of the unsigned type's values?
				 * this is true if signed_type > unsigned_type
				 * - convert unsigned to signed type */

				if(l_unsigned)
					tlarger = *plhs = trhs;
				else
					tlarger = *prhs = tlhs;

			}else{
				/* else convert both to (unsigned)signed_type */
				type_ref *signed_t = l_unsigned ? trhs : tlhs;

				tlarger = *plhs = *prhs = type_ref_new_cast_signed(signed_t, 0);
			}
		}

		/* if we have a _comparison_ (e.g. between enums), convert to int */
		resolved = op_is_relational(op)
			? type_ref_cached_INT()
			: tlarger;
	}

fin:
	UCC_ASSERT(resolved, "no decl from type promotion");

	return resolved; /* XXX: memleak in some cases */
}

type_ref *op_promote_types(
		enum op_type op,
		const char *desc,
		expr **plhs, expr **prhs,
		where *w, symtable *stab)
{
	type_ref *tlhs, *trhs;
	type_ref *resolved;

	resolved = op_required_promotion(op, *plhs, *prhs, w, &tlhs, &trhs);

	if(tlhs)
		fold_insert_casts(tlhs, plhs, stab, w, desc);

	if(trhs)
		fold_insert_casts(trhs, prhs, stab, w, desc);

	return resolved;
}

static expr *expr_is_array_cast(expr *e)
{
	expr *array = e;

	if(expr_kind(array, cast))
		array = array->expr;

	if(type_ref_is(array->tree_type, type_ref_array))
		return array;

	return NULL;
}

static void op_bound(expr *e)
{
	/* this could be in expr_deref, but it catches more in expr_op */
	expr *array;
	int lhs = 0;

	/* check bounds */
	if(e->op != op_plus && e->op != op_minus)
		return;

	array = expr_is_array_cast(e->lhs);
	if(array)
		lhs = 1;
	else
		array = expr_is_array_cast(e->rhs);

	if(array){
		consty k;

		const_fold(lhs ? e->rhs : e->lhs, &k);

		if(k.type == CONST_VAL){
			const long sz = type_ref_array_len(array->tree_type);

			if(sz == 0) /* FIXME: sentinel */
				return;

#define idx k.bits.iv
			if(e->op == op_minus)
				idx.val = -idx.val;

			/* index is allowed to be one past the end, i.e. idx.val == sz */
			if(idx.val < 0 || idx.val > sz)
				WARN_AT(&e->where,
						"index %ld out of bounds of array, size %ld",
						idx.val, sz);
			/* TODO: "note: array here" */
#undef idx
		}
	}
}

static void op_unsigned_cmp_check(expr *e)
{
	switch(e->op){
			int lhs;
		/*case op_gt:*/
		case op_ge:
		case op_lt:
		case op_le:
			if((lhs = !type_ref_is_signed(e->lhs->tree_type))
			||        !type_ref_is_signed(e->rhs->tree_type))
			{
				consty k;

				const_fold(lhs ? e->rhs : e->lhs, &k);

				if(k.type == CONST_VAL){
					const int v = k.bits.iv.val;

					if(v <= 0){
						WARN_AT(&e->where,
								"comparison of unsigned expression %s %d is always %s",
								op_to_str(e->op), v,
								e->op == op_lt || e->op == op_le ? "false" : "true");
					}
				}
			}

		default:
			break;
	}
}

static void msg_if_precedence(expr *sub, where *w,
		enum op_type binary, int (*test)(enum op_type))
{
	if(expr_kind(sub, op) && !sub->in_parens && (*test)(sub->op)){
		/* ==, !=, <, ... */
		WARN_AT(w, "%s has higher precedence than %s",
				op_to_str(sub->op), op_to_str(binary));
	}
}

static void op_check_precedence(expr *e)
{
	switch(e->op){
		case op_or:
		case op_and:
			msg_if_precedence(e->lhs, &e->where, e->op, op_is_comparison);
			msg_if_precedence(e->rhs, &e->where, e->op, op_is_comparison);
			break;

		case op_andsc:
		case op_orsc:
			msg_if_precedence(e->lhs, &e->where, e->op, op_is_shortcircuit);
			msg_if_precedence(e->rhs, &e->where, e->op, op_is_shortcircuit);
			break;

		default:
			break;
	}
}

void fold_expr_op(expr *e, symtable *stab)
{
	UCC_ASSERT(e->op != op_unknown, "unknown op in expression at %s",
			where_str(&e->where));

	FOLD_EXPR(e->lhs, stab);
	fold_disallow_st_un(e->lhs, op_to_str(e->op));

	if(e->rhs){
		FOLD_EXPR(e->rhs, stab);
		fold_disallow_st_un(e->rhs, op_to_str(e->op));

		expr_promote_int_if_smaller(&e->lhs, stab);
		expr_promote_int_if_smaller(&e->rhs, stab);

		e->tree_type = op_promote_types(e->op, op_to_str(e->op),
				&e->lhs, &e->rhs, &e->where, stab);

		op_bound(e);
		op_check_precedence(e);
		op_unsigned_cmp_check(e);

	}else{
		/* (except unary-not) can only have operations on integers,
		 * promote to signed int
		 */

		if(e->op == op_not){
			e->tree_type = type_ref_cached_INT();

		}else{
			/* op_bnot */
			type_ref *t_unary = e->lhs->tree_type;

			if(!type_ref_is_integral(t_unary))
				DIE_AT(&e->where, "unary %s applied to type '%s'",
						op_to_str(e->op), type_ref_to_str(t_unary));

			expr_promote_int_if_smaller(&e->lhs, stab);

			e->tree_type = e->lhs->tree_type;
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
				out_change_type(e->tree_type);
				/* make sure we get the pointer, for example 2+(int *)p
				 * or the int, e.g. (int *)a && (int *)b -> int */
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
