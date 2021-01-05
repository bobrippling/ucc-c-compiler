#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "../defs.h"
#include "ops.h"
#include "expr_op.h"
#include "../out/lbl.h"
#include "../out/asm.h"
#include "../type_is.h"
#include "../type_nav.h"
#include "../fopt.h"
#include "../sanitize.h"
#include "../sanitize_opt.h"

#include "expr_cast.h"
#include "expr_val.h"
#include "expr_sizeof.h"
#include "expr_identifier.h"

/*
 * usual arithmetic conversions:
 *   mul, div, mod,
 *   plus, minus,
 *   lt, gt le, ge, eq, ne,
 *   and, xor, or
 *
 * integer promotions:
 *   + - ~ << >>
 */

#define BOOLEAN_TYPE type_int
#define SHOW_CONST_OP 0

typedef struct
{
	const char *lbl; /* null if not label */
	integral_t offset;
	int is_weak;
} collapsed_consty;

enum eval_truth
{
	EVAL_FALSE,
	EVAL_TRUE,
	EVAL_UNKNOWN
};

const char *str_expr_op(void)
{
	return "operator";
}

static void const_op_num_fp(
		expr *e, consty *k,
		const consty *lhs, const consty *rhs)
{
	/* float const-op */
	floating_t fp_r;

	assert(lhs->type == CONST_NUM && (!rhs || rhs->type == CONST_NUM));

	if(cc1_fopt.rounding_math && rhs){ /* aka #pragma STDC FENV ACCESS ON (TODO) */
		k->type = CONST_NO;
		return;
	}

	fp_r = const_op_exec_fp(
			lhs->bits.num.val.f,
			rhs ? &rhs->bits.num.val.f : NULL,
			e->bits.op.op);

	k->type = CONST_NUM;

	/* both relational and normal ops between floats are not constant */
	/* except -, which is taken as part of the floating point constant */
	if(e->bits.op.op != op_minus && !k->nonstandard_const)
		k->nonstandard_const = e;

	if(op_returns_bool(e->bits.op.op)){
		k->bits.num.val.i = fp_r; /* convert to bool */

	}else{
		const btype *ty = type_get_type(e->tree_type);

		UCC_ASSERT(ty, "no float type for float op?");

		k->bits.num.val.f = fp_r;

		switch(ty->primitive){
			case type_float:   k->bits.num.suffix = VAL_FLOAT;   break;
			case type_double:  k->bits.num.suffix = VAL_DOUBLE;  break;
			case type_ldouble: k->bits.num.suffix = VAL_LDOUBLE; break;
			default: ICE("bad float");
		}
	}
}

static void collapse_const(collapsed_consty *out, const consty *in)
{
	out->is_weak = 0;

	switch(in->type){
		case CONST_NO:
			assert(0);

		case CONST_NUM:
			out->lbl = NULL;
			out->offset = in->bits.num.val.i + in->offset;
			break;

		case CONST_STRK:
			out->lbl = in->bits.str->lit->lbl;
			out->offset = in->offset;
			break;

		case CONST_NEED_ADDR:
		case CONST_ADDR:
			switch(in->bits.addr.lbl_type){
				case CONST_LBL_TRUE:
				case CONST_LBL_WEAK:
					out->lbl = in->bits.addr.bits.lbl;
					out->offset = in->offset;
					out->is_weak = (in->bits.addr.lbl_type == CONST_LBL_WEAK);
					break;
				case CONST_LBL_MEMADDR:
					out->lbl = NULL;
					out->offset = in->bits.addr.bits.memaddr + in->offset;
					break;
			}
			break;
	}
}

static enum eval_truth collapsed_consty_to_truth(const collapsed_consty *k)
{
	return k->is_weak ? EVAL_UNKNOWN : k->lbl || k->offset ? EVAL_TRUE : EVAL_FALSE;
}

static void eval_truth_to_consty(enum eval_truth t, consty *k, expr *e)
{
	switch(t){
		case EVAL_TRUE:
		case EVAL_FALSE:
			k->type = CONST_NUM;
			k->bits.num.val.i = t == EVAL_TRUE;
			break;

		case EVAL_UNKNOWN:
			CONST_FOLD_NO(k, e);
			break;
	}
}

static enum eval_truth eval_shortcircuit(
		enum op_type op,
		enum eval_truth lhs,
		enum eval_truth rhs)
{
	const enum { AND, OR } fixed_op = op == op_andsc ? AND : OR;
	enum { SIDEEFFECT = 1 << 2 }; /* must be bitwise-or-able with EVAL_* */
	static const enum eval_truth results[3][2][3] = {
		[EVAL_FALSE]   [AND] [EVAL_FALSE]   = EVAL_FALSE,
		[EVAL_FALSE]   [AND] [EVAL_TRUE]    = EVAL_FALSE,
		[EVAL_FALSE]   [AND] [EVAL_UNKNOWN] = EVAL_FALSE,
		[EVAL_FALSE]   [OR]  [EVAL_FALSE]   = EVAL_FALSE,
		[EVAL_FALSE]   [OR]  [EVAL_TRUE]    = EVAL_TRUE,
		[EVAL_FALSE]   [OR]  [EVAL_UNKNOWN] = EVAL_UNKNOWN,
		[EVAL_TRUE]    [AND] [EVAL_FALSE]   = EVAL_FALSE,
		[EVAL_TRUE]    [AND] [EVAL_TRUE]    = EVAL_TRUE,
		[EVAL_TRUE]    [AND] [EVAL_UNKNOWN] = EVAL_UNKNOWN,
		[EVAL_TRUE]    [OR]  [EVAL_FALSE]   = EVAL_TRUE,
		[EVAL_TRUE]    [OR]  [EVAL_TRUE]    = EVAL_TRUE,
		[EVAL_TRUE]    [OR]  [EVAL_UNKNOWN] = EVAL_TRUE,
		[EVAL_UNKNOWN] [AND] [EVAL_FALSE]   = EVAL_UNKNOWN, /* sideeffect */
		[EVAL_UNKNOWN] [AND] [EVAL_TRUE]    = EVAL_UNKNOWN,
		[EVAL_UNKNOWN] [AND] [EVAL_UNKNOWN] = EVAL_UNKNOWN,
		[EVAL_UNKNOWN] [OR]  [EVAL_FALSE]   = EVAL_UNKNOWN,
		[EVAL_UNKNOWN] [OR]  [EVAL_TRUE]    = EVAL_UNKNOWN, /* sideeffect */
		[EVAL_UNKNOWN] [OR]  [EVAL_UNKNOWN] = EVAL_UNKNOWN,
		/* sideeffect entries could be a constant value, but other compilers
		 * don't treat this as such, so we fail the const-eval */
	};
	return results[lhs][fixed_op][rhs];
}

static void const_op_num_int(
		expr *e, consty *k,
		const consty *lhs, const consty *rhs)
{
	const char *err = NULL;
	int is_signed;
	collapsed_consty l, r;

	/* the op is signed if an operand is, not the result,
	 * e.g. u_a < u_b produces a bool (signed) */
	is_signed = type_is_signed(e->lhs->tree_type) ||
		(e->rhs ? type_is_signed(e->rhs->tree_type) : 0);

	collapse_const(&l, lhs);
	if(rhs){
		type *ptr;
		int ptr_r = 0;

		collapse_const(&r, rhs);

		/* need to apply pointer arithmetic */
		if(!!l.lbl + !!r.lbl < 2 /* at least one side is a number */
		&& (e->bits.op.op == op_plus || e->bits.op.op == op_minus) /* +/- */
		&& ((ptr = type_is_ptr(e->lhs->tree_type))
			|| (ptr_r = 1, ptr = type_is_ptr(e->rhs->tree_type))))
		{
			unsigned step = type_size(ptr, &e->where);

			assert(!(ptr_r ? &l : &r)->lbl);

			*(ptr_r ? &l.offset : &r.offset) *= step;
		}
	}else{
		memset(&r, 0, sizeof r);
	}

	CONST_FOLD_LEAF(k);

	switch(e->bits.op.op){
		case op_andsc:
		case op_orsc:
		{
			enum eval_truth result = eval_shortcircuit(
					e->bits.op.op,
					collapsed_consty_to_truth(&l),
					collapsed_consty_to_truth(&r));

			eval_truth_to_consty(result, k, e);

			/* early finish */
			return;
		}
		default:
			break;
	}

	switch(!!l.lbl + !!r.lbl){
		default:
			assert(0);

		case 1:
		{
			collapsed_consty *num_side = NULL, *lbl_side = NULL;
			if(!l.lbl)
				num_side = &l;
			else if(!r.lbl)
				num_side = &r;
			else
				assert(0 && "unreachable");

			if(l.lbl)
				lbl_side = &l;
			else if(r.lbl)
				lbl_side = &r;
			else
				assert(0 && "unreachable");

			/* label and num - only + and -, shortcircuit or comparison */
			switch(e->bits.op.op){
				case op_not:
					/* !&lbl */
					assert(!rhs);
					assert(lbl_side);
					if(lbl_side->is_weak){
						CONST_FOLD_NO(k, e);
					}else{
						k->type = CONST_NUM;
						k->bits.num.val.i = 0;
					}
					break;

				case op_eq:
				case op_ne:
					assert(num_side && "binary two labels shouldn't be here");
					if(num_side->offset == 0 && !lbl_side->is_weak){
						/* &x == 0, etc */
						k->type = CONST_NUM;
						k->bits.num.val.i = (e->bits.op.op != op_eq);
						break;
					}
					/* fall */

				default:
					/* ~&lbl, 5 == &lbl, 6 > &lbl etc */
					CONST_FOLD_NO(k, e);
					break;

				case op_plus:
				case op_minus:
					if(!rhs){
						/* unary +/- on label */
						CONST_FOLD_NO(k, e);
						break;
					}

					/* this is fine, we're simply adding to a maybe-weak label */
					memcpy_safe(k, num_side == &l ? rhs : lhs);
					if(e->bits.op.op == op_plus)
						k->offset += num_side->offset;
					else if(e->bits.op.op == op_minus)
						k->offset -= num_side->offset;
					break;
			}
			break;
		}

		case 0:
		{
			integral_t int_r;

			int_r = const_op_exec(
					l.offset, rhs ? &r.offset : NULL,
					e->bits.op.op, e->tree_type, &err);

			if(SHOW_CONST_OP){
				char tbufs[2][TYPE_STATIC_BUFSIZ];

				if(rhs){
					fprintf(stderr,
							"const op: (%s)%lld %s (%s)%lld   -->   (%s)%lld, is_signed=%d\n",
							type_to_str_r(tbufs[0], e->lhs->tree_type),
							l.offset,
							op_to_str(e->bits.op.op),
							type_to_str_r(tbufs[1], e->rhs->tree_type),
							r.offset,
							type_to_str(e->tree_type),
							int_r,
							is_signed);
				}else{
					fprintf(stderr,
							"const op: (%s)%s%lld --> (%s)%lld, is_signed=%d\n",
							type_to_str_r(tbufs[0], e->lhs->tree_type),
							op_to_str(e->bits.op.op),
							l.offset,
							type_to_str(e->tree_type),
							int_r,
							is_signed);
				}
			}

			if(err){
				cc1_warn_at(&e->where, constop_bad, "%s", err);
				CONST_FOLD_NO(k, e);
			}else{
				const btype *bt;

				k->type = CONST_NUM;
				k->bits.num.val.i = int_r;

				if(!is_signed)
					k->bits.num.suffix = VAL_UNSIGNED;

				/* if no btype, we may be something like:
				 * (int *)0 + 3 */
				bt = type_get_type(e->tree_type);
				switch(bt ? bt->primitive : type_unknown){
					case type_long:
						k->bits.num.suffix |= VAL_LONG;
						break;
					case type_llong:
						k->bits.num.suffix |= VAL_LLONG;
					default:
						break;
				}
			}
			break;
		}

		case 2:
			switch(e->bits.op.op){
				case op_not:
					assert(0 && "binary not?");
				case op_unknown:
					assert(0);
				default:
					CONST_FOLD_NO(k, e);
					break;

				case op_orsc:
				case op_andsc:
					assert(0 && "unreachable");

				case op_eq:
				case op_ne:
				{
					/* if one is weak, it may, or may not be the other */
					if(l.is_weak || r.is_weak){
						CONST_FOLD_NO(k, e);
					}else{
						int same = !strcmp(l.lbl, r.lbl);
						k->type = CONST_NUM;

						k->bits.num.val.i = ((e->bits.op.op == op_eq) == same);
					}
					break;
				}

				case op_minus:
					if(!l.is_weak && !r.is_weak && !strcmp(l.lbl, r.lbl)){
						type *tnext = type_is_ptr(e->lhs->tree_type);
						assert(tnext);

						k->type = CONST_NUM;
						k->bits.num.val.i = (l.offset - r.offset) / type_size(tnext, NULL);
						k->nonstandard_const = e;
					}
					break;
			}
			break;
	}
}

static void const_op_num(
		expr *e, consty *k,
		const consty *lhs, const consty *rhs)
{
	int fp[2] = {
		type_is_floating(e->lhs->tree_type)
	};

	if(rhs){
		int valid;

		fp[1] = type_is_floating(e->rhs->tree_type);

		valid = fp[0] == fp[1];

		if(!valid){
			if(fold_had_error){
				/* okay, ignore */
				return;
			}
			assert(valid && "one float and one non-float?");
		}
	}

	if(fp[0])
		const_op_num_fp(e, k, lhs, rhs);
	else
		const_op_num_int(e, k, lhs, rhs);
}

static void const_shortcircuit(
		expr *e, consty *k,
		const consty *lhs,
		const consty *rhs)
{
	collapsed_consty clhs;
	enum eval_truth truth;

	if(!CONST_AT_COMPILE_TIME(lhs->type))
		return;

	/* if we've got here, previous attempts at const-ing have failed,
	 * which means lhs or rhs is not constant.
	 *
	 * if lhs is not constant, we can't decide, hence the above return
	 *
	 * otherwise, lhs is constant, rhs is not */
  assert(!CONST_AT_COMPILE_TIME(rhs->type));

	collapse_const(&clhs, lhs);

	truth = eval_shortcircuit(
			e->bits.op.op,
			collapsed_consty_to_truth(&clhs),
			EVAL_UNKNOWN);

	eval_truth_to_consty(truth, k, e);
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

	if(!CONST_AT_COMPILE_TIME(lhs.type) /* catch need_addr */
	|| !CONST_AT_COMPILE_TIME(rhs.type))
	{
		const_fold_no(k, &lhs, e->lhs, &rhs, e->rhs);

		/* allow shortcircuit */
		if(e->bits.op.op == op_andsc || e->bits.op.op == op_orsc){
			const_shortcircuit(e, k, &lhs, &rhs);

			/* undo CONST_FOLD_NO() above */
			if(k->type != CONST_NO)
				k->nonconst = NULL;
		}

		return;
	}

	const_op_num(e, k, &lhs, e->rhs ? &rhs : NULL);

	if(!k->nonstandard_const
	&& lhs.type != CONST_NO /* otherwise it's uninitialised */
	&& rhs.type != CONST_NO)
	{
		k->nonstandard_const = lhs.nonstandard_const
			? lhs.nonstandard_const : rhs.nonstandard_const;
	}
}

static void expr_promote_if_smaller(expr **pe, symtable *stab, int do_float)
{
	expr *e = *pe;
	type *to;

	if(type_is_floating(e->tree_type) && !do_float)
		return;

	if(type_is_promotable(e->tree_type, &to)){
		expr *cast;

		UCC_ASSERT(!type_is(e->tree_type, type_ptr),
				"invalid promotion for pointer");

		/* if(type_primitive_size(e->tree_type->type->primitive) >= type_primitive_size(to))
		 *   return;
		 *
		 * insert down-casts too - the tree_type of the expression is still important
		 */

		cast = expr_new_cast(e, to, 1);

		fold_expr_cast_descend(cast, stab, 0);

		*pe = cast;
	}
}

void expr_promote_int_if_smaller(expr **pe, symtable *stab)
{
	expr_promote_if_smaller(pe, stab, 0);
}

void expr_promote_default(expr **pe, symtable *stab)
{
	expr_promote_if_smaller(pe, stab, 1);
}

type *op_required_promotion(
		enum op_type op,
		expr *lhs, expr *rhs,
		where *w, const char *desc,
		type **plhs, type **prhs)
{
	type *resolved = NULL;
	type *const tlhs = lhs->tree_type, *const trhs = rhs->tree_type;
	int floating_lhs;

	if(!desc)
		desc = op_to_str(op);

	*plhs = *prhs = NULL;

	if((floating_lhs = type_is_floating(tlhs))
			!= type_is_floating(trhs))
	{
		/* cast _to_ the floating type */
		type *res = floating_lhs ? (*prhs = tlhs) : (*plhs = trhs);

		resolved = op_returns_bool(op)
			? type_nav_btype(cc1_type_nav, BOOLEAN_TYPE)
			: res;

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

	if(type_is_void(tlhs) || type_is_void(trhs))
		warn_at_print_error(w, "use of void expression");

	{
		type *l_pointee = type_is_ptr(tlhs);
		const int l_ptr = !!l_pointee;
		const int r_ptr = !!type_is(trhs, type_ptr);

		if(l_ptr && r_ptr){
			char buf[TYPE_STATIC_BUFSIZ];

			if(op == op_minus){
				switch(type_cmp(tlhs, trhs, 0)){
					case TYPE_CONVERTIBLE_IMPLICIT:
					case TYPE_CONVERTIBLE_EXPLICIT:
					case TYPE_NOT_EQUAL:
						warn_at_print_error(w, "subtraction of distinct pointer types %s and %s",
								type_to_str(tlhs), type_to_str_r(buf, trhs));
						fold_had_error = 1;
						break;

					case TYPE_QUAL_ADD:
					case TYPE_QUAL_SUB:
					case TYPE_QUAL_POINTED_ADD:
					case TYPE_QUAL_POINTED_SUB:
					case TYPE_QUAL_NESTED_CHANGE:
					case TYPE_EQUAL:
					case TYPE_EQUAL_TYPEDEF:
						if(type_is_void(l_pointee))
							cc1_warn_at(w, arith_voidp, "arithmetic on void pointer");
						break;
				}

				resolved = type_nav_btype(cc1_type_nav, type_intptr_t);

			}else if(op_returns_bool(op)){
ptr_relation:
				if(op_is_comparison(op)){
					if(fold_type_chk_warn(lhs, NULL, rhs, /*is_comparison*/1, w,
							l_ptr && r_ptr
							? "comparison lacks a cast"
							: "comparison between pointer and integer"))
					{
						int void_lhs;
						/* not equal - ptr-A vs ptr-B */

						if((void_lhs = type_is_void_ptr(tlhs)) || type_is_void_ptr(trhs)){
							/* special case - if comparing against void*,
							 * cast the void* to the target type */
							*(void_lhs ? plhs : prhs) = (void_lhs ? trhs : tlhs);
						}else{
							/* At least one is a pointer, cast the other to it.
							 * This matches gcc & clang, with clang giving priority
							 * to the lhs, in the case that both are pointers */
							if(type_is_ptr(tlhs))
								*prhs = tlhs;
							else
								*plhs = trhs;
						}
					}
				}

				resolved = type_nav_btype(cc1_type_nav, BOOLEAN_TYPE);

			}else{
				fold_had_error = 1;
				warn_at_print_error(w, "operation between two pointers must be relational or subtraction");
				resolved = type_nav_btype(cc1_type_nav, BOOLEAN_TYPE);
			}

			goto fin;

		}else if(l_ptr || r_ptr){
			/* + or - */

			/* cmp between pointer and integer - missing cast */
			if(op_returns_bool(op))
				goto ptr_relation;

			switch(op){
				default:
					warn_at_print_error(w, "operation between pointer and integer must be + or -");
					fold_had_error = 1;
					break;

				case op_minus:
					if(!l_ptr){
						warn_at_print_error(w, "subtraction of pointer from integer");
						fold_had_error = 1;
					}
					break;

				case op_plus:
					break;
			}

			resolved = l_ptr ? tlhs : trhs;

			/* FIXME: promote to unsigned */
			*(l_ptr ? prhs : plhs) = type_nav_btype(cc1_type_nav, type_intptr_t);

			/* + or -, check if we can */
			{
				type *const next = type_next(resolved);

				if(!type_is_complete(next)){
					if(type_is_void(next)){
						cc1_warn_at(w, arith_voidp, "arithmetic on void pointer");
					}else{
						warn_at_print_error(w,
								"arithmetic on pointer to incomplete type %s",
								type_to_str(next));
						fold_had_error = 1;
					}
					/* TODO: note: type declared at resolved->where */
				}else if(type_is(next, type_func)){
					cc1_warn_at(w, arith_fnptr, "arithmetic on function pointer '%s'",
							type_to_str(resolved));
				}
			}

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
		type *tlarger = NULL;

		if(op == op_shiftl || op == op_shiftr){
			/* fine with any parameter sizes
			 * don't need to match. resolves to lhs,
			 * or int if lhs is smaller
			 */

			if(type_intrank(type_get_primitive(tlhs)) < type_intrank(type_int))
				resolved = *plhs = type_nav_btype(cc1_type_nav, type_int);
			else
				resolved = tlhs;

		}else if(op == op_andsc || op == op_orsc){
			/* no promotion */
			resolved = type_nav_btype(cc1_type_nav, BOOLEAN_TYPE);

		}else{
			const int l_unsigned = !type_is_signed(tlhs),
			          r_unsigned = !type_is_signed(trhs);

			const int l_rank = type_intrank(type_get_primitive(tlhs)),
			          r_rank = type_intrank(type_get_primitive(trhs));

			/* want to warn regardless of checks - for enums */
			fold_type_chk_warn(lhs, NULL, rhs, /*is_comparison*/1, w, desc);

			if(l_unsigned == r_unsigned){
				enum { SAME, LEFT, RIGHT } larger = SAME;

				if(l_rank > r_rank)
					larger = LEFT;
				else if(l_rank < r_rank)
					larger = RIGHT;

				if(l_rank == r_rank && l_rank == -1){
					/* floating types come in here - default to larger */
					const int l_sz = type_size(tlhs, &lhs->where),
					          r_sz = type_size(trhs, &rhs->where);

					if(l_sz > r_sz)
						larger = LEFT;
					else if(l_sz < r_sz)
						larger = RIGHT;
				}

				if(larger != SAME)
					*(larger == LEFT ? prhs : plhs) = (larger == LEFT ? tlhs : trhs);

				tlarger = (larger == LEFT ? tlhs : trhs);

			}else if(l_unsigned ? l_rank >= r_rank : r_rank >= l_rank){
				if(l_unsigned)
					tlarger = *prhs = tlhs;
				else
					tlarger = *plhs = trhs;

			}else{
				/* can the signed type represent all of the unsigned type's values?
				 * this is true if signed_type_size > unsigned_type_size
				 * (for 2's complement, which we assume)
				 * if so - convert unsigned to signed type */
				const int l_sz = type_size(tlhs, &lhs->where),
				          r_sz = type_size(trhs, &rhs->where);

				if(l_unsigned ? r_sz > l_sz : l_sz > r_sz){
					if(l_unsigned)
						tlarger = *plhs = trhs;
					else
						tlarger = *prhs = tlhs;

				}else{
					/* else convert both to (unsigned)signed_type */
					type *signed_t = l_unsigned ? trhs : tlhs;

					tlarger = *plhs = *prhs = type_sign(cc1_type_nav, signed_t, 0);
				}
			}

			/* if we have a _comparison_, convert to bool */
			resolved = op_returns_bool(op)
				? type_nav_btype(cc1_type_nav, BOOLEAN_TYPE)
				: tlarger;
		}
	}

fin:
	UCC_ASSERT(resolved, "no decl from type promotion");

	return resolved; /* XXX: memleak in some cases */
}

type *op_promote_types(
		enum op_type op,
		expr **plhs, expr **prhs,
		symtable *stab,
		where *w, const char *desc)
{
	type *tlhs, *trhs;
	type *resolved;

	resolved = op_required_promotion(
			op, *plhs, *prhs, w, desc,
			&tlhs, &trhs);

	if(tlhs)
		fold_insert_casts(tlhs, plhs, stab);

	if(trhs)
		fold_insert_casts(trhs, prhs, stab);

	return resolved;
}

static expr *expr_is_nonvla_casted_array(expr *e)
{
	expr *array = e;
	type *test;

	if(expr_kind(array, cast))
		array = array->expr;

	if((test = type_is(array->tree_type, type_array))
	&& !test->bits.array.vla_kind)
	{
		return array;
	}

	return NULL;
}

int fold_check_bounds(expr *e, int chk_one_past_end)
{
	/* this could be in expr_deref, but it catches more in expr_op */
	expr *array;
	int lhs = 0;

	/* check bounds */
	if(e->bits.op.op != op_plus && e->bits.op.op != op_minus)
		return 0;

	array = expr_is_nonvla_casted_array(e->lhs);
	if(array)
		lhs = 1;
	else
		array = expr_is_nonvla_casted_array(e->rhs);

	if(array && !type_is_incomplete_array(array->tree_type)){
		consty k;

		const_fold(lhs ? e->rhs : e->lhs, &k);

		if(k.type == CONST_NUM){
			const size_t sz = type_array_len(array->tree_type);

			UCC_ASSERT(K_INTEGRAL(k.bits.num),
					"fp index?");

#define idx k.bits.num
			if(e->bits.op.op == op_minus)
				idx.val.i = -idx.val.i;

			/* index is allowed to be one past the end, i.e. idx.val == sz */
			if((sintegral_t)idx.val.i < 0
			|| (chk_one_past_end ? idx.val.i > sz : idx.val.i == sz))
			{
				int warned = cc1_warn_at(&e->where,
						array_oob,
						"index %" NUMERIC_FMT_D " out of bounds of array, size %ld",
						idx.val.i, (long)sz);

				if(warned)
					note_at(type_loc(array->tree_type), "array declared here");

				return warned;
			}
#undef idx
		}
	}

	return 0;
}

static int op_unsigned_cmp_check(expr *e)
{
	consty k_lhs, k_rhs;
	consty *k_side;
	int unsigned_lhs, unsigned_rhs;
	sintegral_t v;
	int warn;
	int expect;
	const char *lhs_s, *rhs_s;

	switch(e->bits.op.op){
		case op_ge:
		case op_lt:
		case op_gt:
		case op_le:
			break;
		default:
			return 0;
	}

	unsigned_lhs = !type_is_signed(e->lhs->tree_type);
	unsigned_rhs = !type_is_signed(e->rhs->tree_type);
	if(!unsigned_lhs && !unsigned_rhs)
		return 0;

	const_fold(e->lhs, &k_lhs);
	const_fold(e->rhs, &k_rhs);

	k_side = k_lhs.type == CONST_NUM && K_INTEGRAL(k_lhs.bits.num)
		? &k_lhs
		: k_rhs.type == CONST_NUM && K_INTEGRAL(k_rhs.bits.num)
		? &k_rhs
		: NULL;

	if(!k_side)
		return 0;

	v = k_side->bits.num.val.i;
	if(v)
		return 0;

	warn = 0;
	switch(e->bits.op.op){
		case op_ge:
			warn = k_side == &k_rhs; /* u >= 0 */
			expect = 1;
			break;

		case op_le:
			warn = k_side == &k_lhs; /* 0 <= u */
			expect = 1;
			break;

		case op_gt:
			warn = k_side == &k_lhs; /* 0 > u */;
			expect = 0;
			break;

		case op_lt:
			warn = k_side == &k_rhs; /* u < 0 */;
			expect = 0;
			break;

		default:
			assert(0 && "unreachable");
	}

	if(!warn)
		return 0;

	if(k_side == &k_lhs){
		lhs_s = "0";
		rhs_s = "unsigned expression";
	}else{
		lhs_s = "unsigned expression";
		rhs_s = "0";
	}

	return cc1_warn_at(&e->where,
			tautologic_unsigned,
			"comparison of %s %s %s is always %s",
			lhs_s, op_to_str(e->bits.op.op), rhs_s,
			expect ? "true" : "false");
}

static int msg_if_precedence(expr *sub, where *w,
		enum op_type binary, int (*test)(enum op_type))
{
	sub = expr_skip_all_casts(sub);

	if(expr_kind(sub, op)
	&& sub->rhs /* don't warn for (1 << -5) : (-5) is a unary op */
	&& !sub->in_parens
	&& sub->bits.op.op != binary
	&& (test ? (*test)(sub->bits.op.op) : 1))
	{
		/* ==, !=, <, ... */
		return cc1_warn_at(w, parse_precedence,
				"%s has higher precedence than %s",
				op_to_str(sub->bits.op.op), op_to_str(binary));
	}
	return 0;
}

static int op_check_precedence(expr *e)
{
	switch(e->bits.op.op){
		case op_or:
		case op_and:
		case op_xor:
			return msg_if_precedence(e->lhs, &e->where, e->bits.op.op, op_is_comparison)
				||   msg_if_precedence(e->rhs, &e->where, e->bits.op.op, op_is_comparison);

		case op_andsc:
		case op_orsc:
			return msg_if_precedence(e->lhs, &e->where, e->bits.op.op, op_is_shortcircuit)
				||   msg_if_precedence(e->rhs, &e->where, e->bits.op.op, op_is_shortcircuit);

		case op_shiftl:
		case op_shiftr:
			return msg_if_precedence(e->lhs, &e->where, e->bits.op.op, NULL)
				|| msg_if_precedence(e->rhs, &e->where, e->bits.op.op, NULL);

		default:
			return 0;
	}
}

static int str_cmp_check(expr *e)
{
	if(op_is_comparison(e->bits.op.op)){
		consty kl, kr;

		const_fold(e->lhs, &kl);
		const_fold(e->rhs, &kr);

		if(kl.type == CONST_STRK || kr.type == CONST_STRK){
			return cc1_warn_at(&e->rhs->where,
					undef_strlitcmp,
					"comparison with string literal is unspecified");
		}
	}
	return 0;
}

static int op_shift_check(expr *e)
{
	switch(e->bits.op.op){
		case op_shiftl:
		case op_shiftr:
		{
			const unsigned ty_sz = CHAR_BIT * type_size(e->lhs->tree_type, &e->lhs->where);
			int undefined = 0;
			consty lhs, rhs;
			int emitted = 0;

			const_fold(e->lhs, &lhs);
			const_fold(e->rhs, &rhs);

			if(type_is_signed(e->rhs->tree_type)
			&& (sintegral_t)rhs.bits.num.val.i < 0)
			{
				emitted = cc1_warn_at(&e->rhs->where,
						op_shift_bad,
						"shift count is negative (%"
						NUMERIC_FMT_D ")", (sintegral_t)rhs.bits.num.val.i);

				undefined = 1;
			}else if(rhs.bits.num.val.i >= ty_sz){
				emitted = cc1_warn_at(&e->rhs->where,
						op_shift_bad,
						"shift count >= width of %s (%u)",
						type_to_str(e->lhs->tree_type), ty_sz);

				undefined = 1;
			}

			if(undefined){
				consty k;

				memset(&k, 0, sizeof k);

				if(lhs.type == CONST_NUM){
					k.type = CONST_NUM;
					k.bits.num.val.i = 0;
				}else{
					CONST_FOLD_NO(&k, e);
				}

				expr_set_const(e, &k);
			}

			return emitted; /* aka, warned */
		}
		default:
			return 0;
	}
}

static int op_float_check(expr *e)
{
	type *tl = e->lhs->tree_type,
	         *tr = e->rhs->tree_type;

	if((type_is_floating(tl) || type_is_floating(tr))
	&& !op_can_float(e->bits.op.op))
	{
		char buf[TYPE_STATIC_BUFSIZ];

		/* TODO: factor to a error-continuing function */
		fold_had_error = 1;
		warn_at_print_error(&e->where,
				"binary %s between '%s' and '%s'",
				op_to_str(e->bits.op.op),
				type_to_str_r(buf, tl),
				type_to_str(       tr));

		return 1;
	}

	return 0;
}

static int is_unsuffixed_positive_int_literal(expr *e)
{
	if(!expr_kind(e, val))
		return 0;

	if(e->bits.num.suffix & VAL_FLOATING)
		return 0;

	if(e->bits.num.suffix & (VAL_SUFFIXED_MASK))
		return 0; /* suffixed */

	if((sintegral_t)e->bits.num.val.i < 0)
		return 0; /* negative */

	return 1;
}

void expr_check_sign(const char *desc,
		expr *lhs, expr *rhs, where *w)
{
	/* don't warn for unsuffixed +ve integer literals */
	if(is_unsuffixed_positive_int_literal(lhs))
		return;
	if(is_unsuffixed_positive_int_literal(rhs))
		return;

	if(type_is_scalar(lhs->tree_type)
	&& type_is_scalar(rhs->tree_type)
	&& type_is_signed(lhs->tree_type) != type_is_signed(rhs->tree_type))
	{
		cc1_warn_at(w,
				sign_compare,
				"signed and unsigned types in '%s'", desc);
	}
}

static int op_sizeof_div_check(expr *e)
{
	expr *lhs;

	if(e->bits.op.op != op_divide)
		return 0;

	lhs = expr_skip_all_casts(e->lhs);

	if(!expr_kind(lhs, sizeof))
		return 0;

	if(lhs->expr && type_is_ptr(lhs->expr->tree_type)){
		return cc1_warn_at(&e->where,
				sizeof_ptr_div,
				"division of sizeof(%s) - did you mean sizeof(array)?",
				type_to_str(lhs->expr->tree_type));
	}

	return 0;
}

static int array_subscript_tycheck(expr *e)
{
	type *ty_maybe_int;

	if(!e->bits.op.array_notation)
		return 0;

	if(type_is_ptr(e->lhs->tree_type))
		ty_maybe_int = expr_skip_implicit_casts(e->rhs)->tree_type;
	else if(type_is_ptr(e->rhs->tree_type))
		ty_maybe_int = expr_skip_implicit_casts(e->lhs)->tree_type;
	else
		return 0;

	if(!type_is_primitive(ty_maybe_int, type_nchar))
		return 0;

	return cc1_warn_at(&e->where, char_subscript,
			"array subscript is of type 'char'");
}

static int is_lval_decay_followed_by_ext(expr *e)
{
	expr *child;

	if(!expr_kind(e, cast))
		return 0;
	child = expr_cast_child(e);

	if(!expr_kind(child, cast) || !expr_cast_is_lval2rval(child))
		return 0;

	return 1;
}

static int op_int_promotion_check(expr *e)
{
	type *tlhs, *trhs;

	if(op_is_shortcircuit(e->bits.op.op))
		return 0;

	if(!is_lval_decay_followed_by_ext(e->lhs))
		return 0;
	if(!is_lval_decay_followed_by_ext(e->rhs))
		return 0;

	tlhs = expr_cast_child(e->lhs)->tree_type;
	trhs = expr_cast_child(e->rhs)->tree_type;

	/* guard against non-integer types (e.g. vlas) */
	if(!type_is_integral(tlhs) || !type_is_integral(trhs))
		return 0;

	if(type_size(tlhs, NULL) < type_primitive_size(type_int)
	&& type_size(trhs, NULL) < type_primitive_size(type_int))
	{
		return cc1_warn_at(&e->where,
				int_op_promotion,
				"operands promoted to int for '%s'",
				op_to_str(e->bits.op.op));
	}

	return 0;
}

static int tautological_pointer_check(expr *e)
{
	expr *other = NULL;
	type *ty;
	int is_array;
	consty k;

	if(!op_is_comparison(e->bits.op.op) && !op_is_shortcircuit(e->bits.op.op))
		return 0;

	if(expr_is_null_ptr(e->lhs, NULL_STRICT_INT | NULL_STRICT_ANY_PTR))
		other = e->rhs;
	else if(expr_is_null_ptr(e->rhs, NULL_STRICT_INT | NULL_STRICT_ANY_PTR))
		other = e->lhs;

	if(!other)
		return 0;

	ty = expr_skip_generated_casts(other)->tree_type;

	if(!(is_array = !!type_is_array(ty)) && !type_is(ty, type_func))
		return 0;

	const_fold(other, &k);
	switch(k.type){
		case CONST_ADDR:
		case CONST_NEED_ADDR:
			if(k.bits.addr.lbl_type == CONST_LBL_WEAK)
				return 0;
		default:
			break;
	}

	return cc1_warn_at(&e->where, tautologic_pointer_cmp,
			"comparison of %s with null is always false",
			is_array ? "array" : "function");
}

void fold_expr_op(expr *e, symtable *stab)
{
	const char *op_desc = e->bits.op.array_notation
		? "subscript"
		: op_to_str(e->bits.op.op);

	UCC_ASSERT(e->bits.op.op != op_unknown, "unknown op in expression at %s",
			where_str(&e->where));

	FOLD_EXPR(e->lhs, stab);
	/* ensure we fold the rhs before returning in case of errors,
	 * so it has a tree_type for future things like const_fold */
	if(e->rhs)
		FOLD_EXPR(e->rhs, stab);

	if(fold_check_expr(e->lhs, FOLD_CHK_NO_ST_UN, op_desc)){
		e->tree_type = type_nav_btype(cc1_type_nav, type_int);
		return;
	}

	if(e->rhs){
		if(fold_check_expr(e->rhs, FOLD_CHK_NO_ST_UN, op_desc)){
			e->tree_type = type_nav_btype(cc1_type_nav, type_int);
			return;
		}

		if(op_float_check(e)){
			e->tree_type = type_nav_btype(cc1_type_nav, type_int);
			return;
		}

		/* no-op if float */
		expr_promote_int_if_smaller(&e->lhs, stab);
		expr_promote_int_if_smaller(&e->rhs, stab);

		/* must check signs before casting */
		if(op_is_comparison(e->bits.op.op)){
			expr_check_sign(
					op_desc,
					e->lhs,
					e->rhs,
					&e->where);
		}

		e->tree_type = op_promote_types(e->bits.op.op,
				&e->lhs, &e->rhs, stab, &e->where, op_desc);

		(void)(
				fold_check_bounds(e, 1) ||
				op_check_precedence(e) ||
				op_unsigned_cmp_check(e) ||
				op_shift_check(e) ||
				str_cmp_check(e) ||
				op_sizeof_div_check(e) ||
				array_subscript_tycheck(e) ||
				op_int_promotion_check(e) ||
				tautological_pointer_check(e));

	}else{
		/* (except unary-not) can only have operations on integers,
		 * promote to signed int
		 */
		expr_promote_int_if_smaller(&e->lhs, stab);

		switch(e->bits.op.op){
			default:
				ICE("bad unary op %s", op_desc);

			case op_not:
				e->tree_type = type_nav_btype(cc1_type_nav, type_int);
				if(fold_check_expr(e->lhs,
						FOLD_CHK_NO_ST_UN,
						op_desc))
				{
					return;
				}
				break;

			case op_plus:
			case op_minus:
			case op_bnot:
			{
				enum fold_chk chk = FOLD_CHK_NO_ST_UN;

				if(e->bits.op.op == op_bnot)
					chk |= FOLD_CHK_INTEGRAL;
				else
					chk |= FOLD_CHK_ARITHMETIC;

				e->tree_type = type_unqualify(e->lhs->tree_type);

				if(fold_check_expr(e->lhs, chk, op_to_str(e->bits.op.op))){
					return;
				}
				break;
			}
		}
	}
}

void dump_expr_op(const expr *e, dump *ctx)
{
	dump_desc_expr_newline(ctx, "operator", e, 0);

	dump_printf_indent(ctx, 0, " %s\n", op_to_str(e->bits.op.op));

	dump_inc(ctx);

	dump_expr(e->lhs, ctx);
	if(e->rhs)
		dump_expr(e->rhs, ctx);

	dump_dec(ctx);
}

static const out_val *op_shortcircuit(const expr *e, out_ctx *octx)
{
	out_blk *blk_rhs, *blk_empty, *landing;
	const out_val *lhs;

	blk_rhs = out_blk_new(octx, "shortcircuit_a");
	blk_empty = out_blk_new(octx, "shortcircuit_b");
	landing = out_blk_new(octx, "shortcircuit_landing");

	lhs = gen_expr(e->lhs, octx);
	lhs = out_normalise(octx, lhs);

	out_ctrl_branch(
			octx,
			lhs,
			e->bits.op.op == op_andsc ? blk_rhs : blk_empty,
			e->bits.op.op == op_andsc ? blk_empty : blk_rhs);

	out_current_blk(octx, blk_rhs);
	{
		const out_val *rhs = gen_expr(e->rhs, octx);
		rhs = out_normalise(octx, rhs);

		out_ctrl_transfer(octx, landing, rhs, &blk_rhs, 1);
	}

	out_current_blk(octx, blk_empty);
	{
		out_ctrl_transfer(
				octx,
				landing,
				out_new_l(
					octx,
					type_nav_btype(cc1_type_nav, BOOLEAN_TYPE),
					e->bits.op.op == op_orsc ? 1 : 0),
				&blk_empty,
				1);

	}

	out_current_blk(octx, landing);
	{
		const out_val *merged = out_ctrl_merge(octx, blk_empty, blk_rhs);

		return merged;
	}
}

void gen_op_trapv(
		type *evaltt,
		const out_val **eval,
		out_ctx *octx,
		enum op_type op)
{
	if(type_is_integral(evaltt) && type_is_signed(evaltt)){
		if(!out_sanitize_enabled(octx, SAN_SIGNED_INTEGER_OVERFLOW))
			return;
	}else if(type_is_ptr(evaltt)){
		if(!out_sanitize_enabled(octx, SAN_POINTER_OVERFLOW))
			return;
	}else{
		return;
	}

	if(op_is_comparison(op))
		return;

	{
		out_blk *land = out_blk_new(octx, "trapv_end");
		out_blk *blk_undef = out_blk_new(octx, "trapv_bad");

		out_ctrl_branch(octx,
				out_annotate_likely(octx, out_new_overflow(octx, eval), 1),
				blk_undef,
				land);

		out_current_blk(octx, blk_undef);
		sanitize_fail(octx, op_to_str(op));

		out_current_blk(octx, land);
	}
}

const out_val *gen_expr_op(const expr *e, out_ctx *octx)
{
	const out_val *lhs, *eval;

	switch(e->bits.op.op){
		case op_orsc:
		case op_andsc:
			return op_shortcircuit(e, octx);

		case op_unknown:
			ICE("asm_operate: unknown operator got through");

		default:
			break;
	}

	lhs = gen_expr(e->lhs, octx);

	if(!e->rhs){
		eval = out_op_unary(octx, e->bits.op.op, lhs);

		/* ensure flags, etc get extended up to our type */
		eval = out_change_type(octx, eval, e->tree_type);

		/* this doesn't do the extension here, but tags it with the type.
		 * then when/if we come to do things like decaying a flag to a register,
		 * we'll spot that the type isn't a 1-byte type, and extend then.
		 *
		 * this allows flags to propagate through and be optimised with subsequent
		 * flag operations, without instantly promoting to int
		 */

	}else{
		const out_val *rhs = gen_expr(e->rhs, octx);

		switch(e->bits.op.op){
			case op_plus:
				sanitize_boundscheck(e->lhs, e->rhs, octx, lhs, rhs);
				break;
			case op_shiftl:
			case op_shiftr:
				sanitize_shift(e->lhs, e->rhs, e->bits.op.op, octx, &lhs, &rhs);
				break;
			case op_divide:
				sanitize_divide(lhs, rhs, e->tree_type, octx);
				break;
			default:
				break;
		}

		eval = out_op(octx, e->bits.op.op, lhs, rhs);

		/* make sure we get the pointer, for example 2+(int *)p
		 * or the int, e.g. (int *)a && (int *)b -> int */
		eval = out_change_type(octx, eval, e->tree_type);
	}

	gen_op_trapv(e->tree_type, &eval, octx, e->bits.op.op);

	return eval;
}

static int expr_op_has_sideeffects(const expr *e)
{
	return expr_has_sideeffects(e->lhs) || (e->rhs && expr_has_sideeffects(e->rhs));
}

static int expr_op_requires_relocation(const expr *e)
{
	return expr_requires_relocation(e->lhs) || (e->rhs && expr_requires_relocation(e->rhs));
}

void mutate_expr_op(expr *e)
{
	e->f_const_fold = fold_const_expr_op;
	e->f_has_sideeffects = expr_op_has_sideeffects;
	e->f_requires_relocation = expr_op_requires_relocation;
}

expr *expr_new_op(enum op_type op)
{
	expr *e = expr_new_wrapper(op);
	e->bits.op.op = op;
	return e;
}

expr *expr_new_op2(enum op_type o, expr *l, expr *r)
{
	expr *e = expr_new_op(o);
	e->lhs = l, e->rhs = r;
	return e;
}

const out_val *gen_expr_style_op(const expr *e, out_ctx *octx)
{
	if(e->rhs){
		const char *post;
		char middle[16];

		if(e->bits.op.array_notation){
			strcpy(middle, "[");
			post = "]";
		}else{
			snprintf(middle, sizeof middle, " %s ", op_to_str(e->bits.op.op));
			post = "";
		}

		IGNORE_PRINTGEN(gen_expr(e->lhs, octx));
		stylef("%s", middle);
		IGNORE_PRINTGEN(gen_expr(e->rhs, octx));
		stylef("%s", post);

	}else{
		stylef("%s ", op_to_str(e->bits.op.op));
		IGNORE_PRINTGEN(gen_expr(e->lhs, octx));
	}
	return NULL;
}
