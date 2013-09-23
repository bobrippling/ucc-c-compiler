#include <string.h>
#include <stdlib.h>

#include "../defs.h"
#include "ops.h"
#include "expr_op.h"
#include "../out/lbl.h"
#include "../out/asm.h"
#include "../out/basic_block.h"
#include "../type_ref_is.h"

const char *str_expr_op()
{
	return "op";
}

static void const_offset(consty *r, consty *val, consty *addr,
		type_ref *addr_type, enum op_type op)
{
	unsigned step = type_ref_size(type_ref_next(addr_type), NULL);
	int change;

	UCC_ASSERT(K_INTEGRAL(val->bits.num),
			"fp + address?");

	memcpy_safe(r, addr);

	change = val->bits.num.val.i * step;

	if(op == op_minus)
		change = -change;

	/* may already have an offset, hence += */
	r->offset += change;
}

static void fold_const_expr_op(expr *e, consty *k)
{
	consty lhs, rhs;

	memset(k, 0, sizeof *k);

	const_fold(e->lhs, &lhs);
	if(e->rhs){
		const_fold(e->rhs, &rhs);
	}else{
		memset(&rhs, 0, sizeof rhs);
		rhs.type = CONST_NUM;
	}

	if(lhs.type == CONST_NUM && rhs.type == CONST_NUM){
		int fp[2] = {
			type_ref_is_floating(e->lhs->tree_type)
		};

		if(e->rhs){
			fp[1] = type_ref_is_floating(e->rhs->tree_type);

			UCC_ASSERT(!(fp[0] ^ fp[1]),
					"one float and one non-float?");
		}

		if(fp[0]){
			/* float const-op */
			floating_t r = const_op_exec_fp(
					lhs.bits.num.val.f,
					e->rhs ? &rhs.bits.num.val.f : 0,
					e->op);

			k->type = CONST_NUM;

			if(op_returns_bool(e->op)){
				k->bits.num.val.i = r; /* convert to bool */

			}else{
				const type *ty = type_ref_get_type(e->tree_type);

				UCC_ASSERT(ty, "no float type for float op?");

				k->bits.num.val.f = r;

				switch(ty->primitive){
					case type_float:   k->bits.num.suffix = VAL_FLOAT;   break;
					case type_double:  k->bits.num.suffix = VAL_DOUBLE;  break;
					case type_ldouble: k->bits.num.suffix = VAL_LDOUBLE; break;
					default: ICE("bad float");
				}
			}

		}else{
			const char *err = NULL;
			integral_t r;
			/* the op is signed if an operand is, not the result,
			 * e.g. u_a < u_b produces a bool (signed) */
			int is_signed = type_ref_is_signed(e->lhs->tree_type) ||
				(e->rhs ? type_ref_is_signed(e->rhs->tree_type) : 0);

			r = const_op_exec(
					lhs.bits.num.val.i,
					e->rhs ? &rhs.bits.num.val.i : NULL,
					e->op, is_signed, &err);

			if(err){
				warn_at(&e->where, "%s", err);
			}else{
				k->type = CONST_NUM;
				k->bits.num.val.i = r;
			}
		}

	}else if((e->op == op_andsc || e->op == op_orsc)
	&& (CONST_AT_COMPILE_TIME(lhs.type) || CONST_AT_COMPILE_TIME(rhs.type))){

		/* allow 1 || f() */
		consty *kside = CONST_AT_COMPILE_TIME(lhs.type) ? &lhs : &rhs;
		int is_true = !!kside->bits.num.val.i;

		/* TODO: to be more conformant we should disallow: a() && 0
		 * i.e. ordering
		 */

		if(e->op == (is_true ? op_orsc : op_andsc))
			memcpy(k, kside, sizeof *k);

	}else if(e->op == op_plus || e->op == op_minus){
		/* allow one CONST_{ADDR,STRK} and one CONST_VAL for an offset const */
		int lhs_addr = lhs.type == CONST_ADDR || lhs.type == CONST_STRK;
		int rhs_addr = rhs.type == CONST_ADDR || rhs.type == CONST_STRK;

		/* this is safe - fold() checks that we can't add floats to addresses */

		/**/if(lhs_addr && rhs.type == CONST_NUM)
			const_offset(k, &rhs, &lhs, e->lhs->tree_type, e->op);
		else if(rhs_addr && lhs.type == CONST_NUM)
			const_offset(k, &lhs, &rhs, e->rhs->tree_type, e->op);
	}
}

static void expr_promote_if_smaller(expr **pe, symtable *stab, int do_float)
{
	expr *e = *pe;
	type_ref *to;

	if(type_ref_is_floating(e->tree_type) && !do_float)
		return;

	if(type_ref_is_promotable(e->tree_type, &to)){
		expr *cast;

		UCC_ASSERT(!type_ref_is(e->tree_type, type_ref_ptr),
				"invalid promotion for pointer");

		/* if(type_primitive_size(e->tree_type->type->primitive) >= type_primitive_size(to))
		 *   return;
		 *
		 * insert down-casts too - the tree_type of the expression is still important
		 */

		cast = expr_new_cast(to, 1);

		cast->expr = e;

		fold_expr_cast_descend(cast, stab, 0);

		*pe = cast;
	}
}

static void expr_promote_int_if_smaller(expr **pe, symtable *stab)
{
	expr_promote_if_smaller(pe, stab, 0);
}

void expr_promote_default(expr **pe, symtable *stab)
{
	expr_promote_if_smaller(pe, stab, 1);
}

type_ref *op_required_promotion(
		enum op_type op,
		expr *lhs, expr *rhs,
		where *w,
		type_ref **plhs, type_ref **prhs)
{
	type_ref *resolved = NULL;
	type_ref *const tlhs = lhs->tree_type, *const trhs = rhs->tree_type;
	int floating_lhs;

	*plhs = *prhs = NULL;

	if((floating_lhs = type_ref_is_floating(tlhs))
			!= type_ref_is_floating(trhs))
	{
		/* cast _to_ the floating type */
		type_ref *res = floating_lhs ? (*prhs = tlhs) : (*plhs = trhs);

		resolved = op_is_comparison(op) ? type_ref_cached_BOOL() : res;

		goto fin;
		/* else we pick the largest floating or integral type */
	}

#if 0
	If either operand is a pointer:
		relation: both must be pointers

	if both operands are pointers:
		must be '-'

	exactly one pointer:
		must be + or -
#endif

	if(type_ref_is_void(tlhs) || type_ref_is_void(trhs))
		die_at(w, "use of void expression");

	{
		const int l_ptr = !!type_ref_is(tlhs, type_ref_ptr);
		const int r_ptr = !!type_ref_is(trhs, type_ref_ptr);

		if(l_ptr && r_ptr){
			char buf[TYPE_REF_STATIC_BUFSIZ];

			if(op == op_minus){
				/* don't allow void * */
				switch(type_ref_cmp(tlhs, trhs, 0)){
					case TYPE_CONVERTIBLE_IMPLICIT:
					case TYPE_CONVERTIBLE_EXPLICIT:
					case TYPE_NOT_EQUAL:
						die_at(w, "subtraction of distinct pointer types %s and %s",
								type_ref_to_str(tlhs), type_ref_to_str_r(buf, trhs));
					case TYPE_QUAL_LOSS:
					case TYPE_EQUAL:
						break;
				}

				resolved = type_ref_cached_INTPTR_T();

			}else if(op_returns_bool(op)){
ptr_relation:
				if(op_is_comparison(op)){
					if(fold_type_chk_warn(tlhs, trhs, w,
							l_ptr && r_ptr
							? "comparison of distinct pointer types lacks a cast"
							: "comparison between pointer and integer"))
					{
						/* not equal - ptr vs int */
						*(l_ptr ? prhs : plhs) = type_ref_cached_INTPTR_T();
					}
				}

				resolved = type_ref_cached_BOOL();

			}else{
				die_at(w, "operation between two pointers must be relational or subtraction");
			}

			goto fin;

		}else if(l_ptr || r_ptr){
			/* + or - */

			/* cmp between pointer and integer - missing cast */
			if(op_returns_bool(op))
				goto ptr_relation;

			switch(op){
				default:
					die_at(w, "operation between pointer and integer must be + or -");

				case op_minus:
					if(!l_ptr)
						die_at(w, "subtraction of pointer from integer");

				case op_plus:
					break;
			}

			resolved = l_ptr ? tlhs : trhs;

			/* FIXME: promote to unsigned */
			*(l_ptr ? prhs : plhs) = type_ref_cached_INTPTR_T();

			/* + or -, check if we can */
			{
				type_ref *const next = type_ref_next(resolved);

				if(!type_ref_is_complete(next)){
					if(type_ref_is_void(next)){
						warn_at(w, "arithmetic on void pointer");
					}else{
						die_at(w, "arithmetic on pointer to incomplete type %s",
								type_ref_to_str(next));
					}
					/* TODO: note: type declared at resolved->where */
				}
			}


			if(type_ref_is_void_ptr(resolved))
				warn_at(w, "arithmetic on void pointer");

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
			/* fine with any parameter sizes
			 * don't need to match. resolves to lhs,
			 * or int if lhs is smaller (done before this function)
			 */

			UCC_ASSERT(
					type_ref_size(tlhs, &lhs->where)
						>= type_primitive_size(type_int),
					"shift operand should have been promoted");

			resolved = tlhs;

		}else if(op == op_andsc || op == op_orsc){
			/* no promotion */
			resolved = type_ref_cached_BOOL();

		}else{
			const int l_unsigned = !type_ref_is_signed(tlhs),
			          r_unsigned = !type_ref_is_signed(trhs);

			const int l_sz = type_ref_size(tlhs, &lhs->where),
			          r_sz = type_ref_size(trhs, &rhs->where);

			if(l_unsigned == r_unsigned){
				if(l_sz != r_sz){
					const int l_larger = l_sz > r_sz;
					char buf[64];

					snprintf(buf, sizeof buf,
							"mismatching types in %s",
							op_to_str(op));

					fold_type_chk_warn(
							tlhs, trhs,
							w, buf);

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

			/* if we have a _comparison_ (e.g. between enums), convert to int */
			resolved = op_returns_bool(op)
				? type_ref_cached_BOOL()
				: tlarger;
		}
	}

fin:
	UCC_ASSERT(resolved, "no decl from type promotion");

	return resolved; /* XXX: memleak in some cases */
}

type_ref *op_promote_types(
		enum op_type op,
		expr **plhs, expr **prhs,
		where *w, symtable *stab)
{
	type_ref *tlhs, *trhs;
	type_ref *resolved;

	resolved = op_required_promotion(op, *plhs, *prhs, w, &tlhs, &trhs);

	if(tlhs)
		fold_insert_casts(tlhs, plhs, stab);

	if(trhs)
		fold_insert_casts(trhs, prhs, stab);

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

void fold_check_bounds(expr *e, int chk_one_past_end)
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

	if(array && !type_ref_is_incomplete_array(array->tree_type)){
		consty k;

		const_fold(lhs ? e->rhs : e->lhs, &k);

		if(k.type == CONST_NUM){
			const size_t sz = type_ref_array_len(array->tree_type);

			UCC_ASSERT(K_INTEGRAL(k.bits.num),
					"fp index?");

#define idx k.bits.num
			if(e->op == op_minus)
				idx.val.i = -idx.val.i;

			/* index is allowed to be one past the end, i.e. idx.val == sz */
			if((sintegral_t)idx.val.i < 0
			|| (chk_one_past_end ? idx.val.i > sz : idx.val.i == sz))
			{
				/* XXX: note */
				char buf[WHERE_BUF_SIZ];

				warn_at(&e->where,
						"index %" NUMERIC_FMT_D " out of bounds of array, size %ld\n"
						"%s: note: array declared here",
						idx.val.i, (long)sz, where_str_r(buf, &array->tree_type->where));
			}
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

				if(k.type == CONST_NUM && K_INTEGRAL(k.bits.num)){
					const int v = k.bits.num.val.i;

					if(v <= 0){
						warn_at(&e->where,
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
	if(expr_kind(sub, op)
	&& !sub->in_parens
	&& sub->op != binary
	&& (test ? (*test)(sub->op) : 1))
	{
		/* ==, !=, <, ... */
		warn_at(w, "%s has higher precedence than %s",
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

		case op_shiftl:
		case op_shiftr:
			msg_if_precedence(e->lhs, &e->where, e->op, NULL);
			msg_if_precedence(e->rhs, &e->where, e->op, NULL);
			break;

		default:
			break;
	}
}

static void op_shift_check(expr *e)
{
	switch(e->op){
		case op_shiftl:
		case op_shiftr:
		{
			const unsigned ty_sz = CHAR_BIT * type_ref_size(e->lhs->tree_type, &e->lhs->where);
			int undefined = 0;
			consty lhs, rhs;

			const_fold(e->lhs, &lhs);
			const_fold(e->rhs, &rhs);

			if(type_ref_is_signed(e->rhs->tree_type)
			&& (sintegral_t)rhs.bits.num.val.i < 0)
			{
				warn_at(&e->rhs->where, "shift count is negative (%"
						NUMERIC_FMT_D ")", (sintegral_t)rhs.bits.num.val.i);

				undefined = 1;
			}else if(rhs.bits.num.val.i >= ty_sz){
				warn_at(&e->rhs->where, "shift count >= width of %s (%u)",
						type_ref_to_str(e->lhs->tree_type), ty_sz);

				undefined = 1;
			}

			if(undefined){
				consty k;

				if(lhs.type == CONST_NUM){
					k.type = CONST_NUM;
					k.bits.num.val.i = 0;
				}else{
					k.type = CONST_NO;
				}

				expr_set_const(e, &k);
			}
		}
		default:
			break;
	}
}

static int op_float_check(expr *e)
{
	type_ref *tl = e->lhs->tree_type,
	         *tr = e->rhs->tree_type;

	if((type_ref_is_floating(tl) || type_ref_is_floating(tr))
	&& !op_can_float(e->op))
	{
		char buf[TYPE_REF_STATIC_BUFSIZ];

		/* TODO: factor to a error-continuing function */
		/*fold_had_error = 1; die for now */
		die_at(&e->where,
				"binary %s between '%s' and '%s'",
				op_to_str(e->op),
				type_ref_to_str_r(buf, tl),
				type_ref_to_str(       tr));

		return 1;
	}

	return 0;
}

void fold_expr_op(expr *e, symtable *stab)
{
	UCC_ASSERT(e->op != op_unknown, "unknown op in expression at %s",
			where_str(&e->where));

	FOLD_EXPR(e->lhs, stab);
	fold_check_expr(e->lhs, FOLD_CHK_NO_ST_UN, op_to_str(e->op));

	if(e->rhs){
		FOLD_EXPR(e->rhs, stab);
		fold_check_expr(e->rhs, FOLD_CHK_NO_ST_UN, op_to_str(e->op));

		if(op_float_check(e)){
			/* short circuit - TODO: error expr */
			e->tree_type = type_ref_cached_INT();
			return;
		}

		/* no-op if float */
		expr_promote_int_if_smaller(&e->lhs, stab);
		expr_promote_int_if_smaller(&e->rhs, stab);

		e->tree_type = op_promote_types(e->op,
				&e->lhs, &e->rhs, &e->where, stab);

		fold_check_bounds(e, 1);
		op_check_precedence(e);
		op_unsigned_cmp_check(e);
		op_shift_check(e);

	}else{
		/* (except unary-not) can only have operations on integers,
		 * promote to signed int
		 */

		expr_promote_int_if_smaller(&e->lhs, stab);

		switch(e->op){
			default:
				ICE("bad unary op %s", op_to_str(e->op));

			case op_not:
				fold_check_expr(e->lhs,
						FOLD_CHK_NO_ST_UN,
						op_to_str(e->op));

				e->tree_type = type_ref_cached_INT();
				break;

			case op_minus:
			case op_bnot:
				fold_check_expr(
						e->lhs,
						(e->op == op_bnot ? FOLD_CHK_INTEGRAL : 0)
							| FOLD_CHK_NO_ST_UN,
						op_to_str(e->op));

				e->tree_type = e->lhs->tree_type;
				break;
		}
	}
}

basic_blk *gen_expr_str_op(expr *e, basic_blk *bb)
{
	idt_printf("op: %s\n", op_to_str(e->op));
	gen_str_indent++;

#define PRINT_IF(hs) if(e->hs) print_expr(e->hs)
	PRINT_IF(lhs);
	PRINT_IF(rhs);
#undef PRINT_IF

	gen_str_indent--;

	return bb;
}

static basic_blk *op_shortcircuit(expr *e, basic_blk *bb)
{
	const int flip = e->op == op_andsc;

	basic_blk *b_t, *b_f;
	basic_blk_phi *b_end;

	bb = gen_expr(e->lhs, bb);
	out_normalise(bb);

	b_t = bb_new("sc_true"), b_f = bb_new("sc_false");

	bb_split(bb,
			flip ? b_t : b_f,
			flip ? b_f : b_t,
			&b_end);

	b_f = gen_expr(e->rhs, b_f);
	out_normalise(b_f);


	bb_phi_incoming(b_end, b_t);
	bb_phi_incoming(b_end, b_f);

	return bb_phi_next(b_end);
}

basic_blk *gen_expr_op(expr *e, basic_blk *bb)
{
	switch(e->op){
		case op_orsc:
		case op_andsc:
			bb = op_shortcircuit(e, bb);
			break;

		case op_unknown:
			ICE("asm_operate: unknown operator got through");

		default:
			bb = gen_expr(e->lhs, bb);

			if(e->rhs){
				bb = gen_expr(e->rhs, bb);

				out_op(bb, e->op);

				/* make sure we get the pointer, for example 2+(int *)p
				 * or the int, e.g. (int *)a && (int *)b -> int */
				out_change_type(bb, e->tree_type);

				if(fopt_mode & FOPT_TRAPV
				&& type_ref_is_integral(e->tree_type)
				&& type_ref_is_signed(e->tree_type))
				{
					basic_blk *b_of, *b_cont;
					basic_blk_phi *b_phi;

					out_push_overflow(bb);
					bb_split_new(bb, &b_of, &b_cont, &b_phi, "trapv");

					out_undefined(b_of);
					bb_terminates(b_of);

					bb_phi_incoming(b_phi, b_cont);
					bb = bb_phi_next(b_phi);
				}
			}else{
				out_op_unary(bb, e->op);
			}
	}

	return bb;
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

expr *expr_new_op2(enum op_type o, expr *l, expr *r)
{
	expr *e = expr_new_op(o);
	e->lhs = l, e->rhs = r;
	return e;
}

basic_blk *gen_expr_style_op(expr *e, basic_blk *bb)
{
	if(e->rhs){
		bb = gen_expr(e->lhs, bb);
		stylef(" %s ", op_to_str(e->op));
		bb = gen_expr(e->rhs, bb);
	}else{
		stylef("%s ", op_to_str(e->op));
		bb = gen_expr(e->lhs, bb);
	}

	return bb;
}
