#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "ops.h"
#include "../../util/alloc.h"
#include "../../util/platform.h"
#include "expr_cast.h"
#include "../sue.h"
#include "../defs.h"
#include "../type_is.h"
#include "../type_nav.h"
#include "../out/dbg.h"

#define IMPLICIT_STR(e) (expr_cast_is_implicit(e) ? "implicit " : "")

static integral_t convert_integral_to_integral_warn(
		const integral_t in, type *tin,
		type *tout,
		int do_warn, where *w);


const char *str_expr_cast()
{
	return "cast";
}

static void fold_cast_num(expr *const e, numeric *const num)
{
	int to_fp, from_fp;

	to_fp = type_is_floating(e->tree_type);
	from_fp = type_is_floating(expr_cast_child(e)->tree_type);

	if(to_fp){
		if(from_fp){
			UCC_ASSERT(K_FLOATING(*num), "i/f mismatch types");
			/* float -> float - nothing to see here */
		}else{
			UCC_ASSERT(K_INTEGRAL(*num), "i/f mismatch types");
			/* int -> float */
			if(num->suffix & VAL_UNSIGNED){
				num->val.f = num->val.i;
			}else{
				/* force a signed conversion, long long to long double */
				num->val.f = (sintegral_t)num->val.i;
			}
		}

		/* perform the trunc */
		switch(type_primitive(e->tree_type)){
			default:
				ICE("fp expected");

#define TRUNC(cse, ty, bmask) \
			case type_ ## cse: \
				num->val.f = (ty)num->val.f; \
				num->suffix = bmask; \
				break

			TRUNC(float, float, VAL_FLOAT);
			TRUNC(double, double, VAL_DOUBLE);
			TRUNC(ldouble, long double, VAL_LDOUBLE);
#undef TRUNC
		}
		return;
	}else if(from_fp){
		UCC_ASSERT(K_FLOATING(*num), "i/f mismatch types");

		/* special case _Bool */
		if(type_is_primitive(e->tree_type, type__Bool)){
			num->val.i = !!num->val.f;
		}else{
			/* float -> int */
			num->val.i = num->val.f;
		}

		num->suffix = 0;

		/* fall through to int logic */
	}

	UCC_ASSERT(K_INTEGRAL(*num), "fp const?");

#define pv (&num->val.i)
	/* need to cast the val.i down as appropriate */
	if(type_is_primitive(e->tree_type, type__Bool)){
		*pv = !!*pv; /* analagous to out/out.c::out_normalise()'s constant case */

	}else if(!from_fp){
		*pv = convert_integral_to_integral_warn(
				*pv, e->expr->tree_type,
				e->tree_type,
				e->expr_cast_implicit, &e->where);
	}
#undef pv
}

static void warn_value_changed_at(
		where *w,
		const char *infmt,
		int signed_in, int signed_out,
		integral_t a, integral_t b)
{
	char *fmt = ustrdup(infmt);
	char *p = fmt;

	for(;;){
		p = strchr(p, '%');
		if(!p)
			break;
		p += 3;
		if(*p == 'A' || *p == 'B'){
			*p = (*p == 'A' ? signed_in : signed_out) ? 'd' : 'u';
		}
	}

	cc1_warn_at(w, overflow, fmt, a, b);
	free(fmt);
}

static integral_t convert_integral_to_integral_warn(
		const integral_t in, type *tin,
		type *tout,
		int do_warn, where *w)
{
	/*
	 * C99
	 * 6.3.1.3 Signed and unsigned integers
	 *
	 * When a value with integer type is converted to another integer type
	 * other than _Bool, if the value can be represented by the new type, it
	 * is unchanged.
	 *
	 * Otherwise, if the new type is unsigned, the value is converted by
	 * repeatedly adding or subtracting one more than the maximum value that
	 * can be represented in the new type until the value is in the range of
	 * the new type.
	 *
	 * Otherwise, the new type is signed and the value cannot be represented
	 * in it; either the result is implementation-defined or an
	 * implementation-defined signal is raised.
	 */

	/* representable
	 * if size(out) > size(in)
	 * or if size(out) == size(in)
	 *    and conversion is not unsigned -> signed
	 * or conversion is unsigned -> signed and in < signed-max
	 */

	const unsigned sz_out = type_size(tout, w);
	const int signed_in = type_is_signed(tin);
	const int signed_out = type_is_signed(tout);
	sintegral_t to_iv_sign_ext;
	integral_t to_iv = integral_truncate(in, sz_out, &to_iv_sign_ext);
	integral_t ret;

	if(!signed_out && signed_in){
		const unsigned sz_in_bits = CHAR_BIT * type_size(tin, w);
		const unsigned sz_out_bits = CHAR_BIT * sz_out;

		/* e.g. "(unsigned)-1". Pick to_iv, i.e. the unsigned truncated repr
		 * this assumes that signed ints on the host machine we're run on
		 * are 2's complement, i.e. truncated a negative gives us all 1s,
		 * which we truncate */
		ret = to_iv;

		/* need to ensure sign extension */
		if(ret & (1ULL << (sz_in_bits - 1))
		&& sz_in_bits != sz_out_bits)
		{
			ret |= -1ULL << sz_in_bits;

			/* need to unmask any top bits, e.g. int instead of long long */
			if(sz_out_bits >= CHAR_BIT * sizeof(ret)){
				/* shift would be a no-op (technically UB) */
			}else{
				ret &= -1ULL >> sz_out_bits;
			}
		}

	}else if(signed_in){
		/* signed to signed */
		ret = (integral_t)to_iv_sign_ext;
	}else{
		/* unsigned to unsigned */
		ret = to_iv_sign_ext;
	}

	if(do_warn){
		if(ret != in){
			warn_value_changed_at(w,
					"implicit cast changes value from %llA to %llB",
					signed_in, signed_out,
					in, signed_out ? (integral_t)to_iv_sign_ext : ret);

		}else if(signed_out && !signed_in && (sintegral_t)ret < 0){
			warn_value_changed_at(w,
					"implicit cast negates value, %llA to %llB",
					signed_in, signed_out,
					in, (sintegral_t)to_iv_sign_ext);

		}else if(signed_out ? (sintegral_t)ret > 0 : 1){
			/* ret > 0 - don't warn for -1 <-- -1L */
			int in_high = integral_high_bit(in, tin);
			int out_high = integral_high_bit(type_max(tout, w), tout);

			if(in_high > out_high){
				warn_value_changed_at(w,
						"implicit cast truncates value from %llA to %llB",
						signed_in, signed_out,
						in, ret & ((1ULL << (out_high + 1)) - 1));
			}
		}
	}

	return ret;
}

static void check_qual_rm(type *ptr_lhs, type *ptr_rhs, expr *e)
{
	enum type_qualifier ql, qr, remain;

	if(!e->expr_cast_implicit)
		return;

	if(!ptr_lhs || !ptr_rhs)
		return;

	ql = type_qual(ptr_lhs);
	qr = type_qual(ptr_rhs);
	remain = qr & ~ql;

	if(remain == qual_none)
		return;

	cc1_warn_at(&e->where,
			cast_qual,
			"%scast removes qualifiers (%s)",
			IMPLICIT_STR(e),
			type_qual_to_str(remain, 0));
}

static void check_addr_int_cast(consty *k, int l, expr *owner)
{
	/* shouldn't fit, check if it will */
	switch(k->type){
		default:
			ICE("bad switch");

		case CONST_STRK:
			/* no idea where it will be in memory,
			 * can't fit into a smaller type */
			CONST_FOLD_NO(k, owner); /* e.g. (int)&a */
			break;

		case CONST_NEED_ADDR:
		case CONST_ADDR:
			if(k->bits.addr.is_lbl){
				CONST_FOLD_NO(k, owner); /* similar to strk case */
			}else{
				integral_t new = k->bits.addr.bits.memaddr;
				const int pws = platform_word_size();

				/* mask out bits so we have it truncated to `l' */
				if(l < pws){
					new = integral_truncate(new, l, NULL);

					if(k->bits.addr.bits.memaddr != new)
						/* can't cast without losing value - not const */
						CONST_FOLD_NO(k, owner);

				}else{
					/* what are you doing... */
					CONST_FOLD_NO(k, owner);
				}
			}
	}
}

static void cast_addr(expr *e, consty *k)
{
	int l, r;
	type *subtt = expr_cast_child(e)->tree_type;

	/* allow if we're casting to a same-size type */
	l = type_size(e->tree_type, &e->where);

	if(type_decayable(subtt))
		r = platform_word_size(); /* func-ptr or array->ptr */
	else
		r = type_size(subtt, &expr_cast_child(e)->where);

	if(l < r)
		check_addr_int_cast(k, l, e);
}

static void fold_const_expr_cast(expr *e, consty *k)
{
	int to_fp;

	if(type_is_void(e->tree_type)){
		CONST_FOLD_NO(k, e);
		return;
	}

	const_fold(expr_cast_child(e), k);

	if(expr_cast_is_lval2rval(e)){
		/* if we're going from int to pointer or vice-versa,
		 * change the const type */
		const_ensure_num_or_memaddr(k, e->expr->tree_type, e->tree_type, e);
		return;
	}

	to_fp = type_is_floating(e->tree_type);

	switch(k->type){
		case CONST_NO:
			break;

		case CONST_NUM:
			fold_cast_num(e, &k->bits.num);
			break;

		case CONST_NEED_ADDR:
			if(to_fp){
				CONST_FOLD_NO(k, e);
				break;
			}
			/* fall */

		case CONST_ADDR:
		case CONST_STRK:
			if(to_fp){
				/* had an error - reported in fold() */
				CONST_FOLD_NO(k, e);
				return;
			}

			cast_addr(e, k);
			break;
	}

	/* may be mutated above */
	if(k->type == CONST_NO)
		return;

	const_ensure_num_or_memaddr(k, e->expr->tree_type, e->tree_type, e);
}

void fold_expr_cast_descend(expr *e, symtable *stab, int descend)
{
	int flag;
	type *tlhs, *trhs;

	if(descend){
		if(expr_cast_is_lval2rval(e)){
			fold_expr_nodecay(expr_cast_child(e), stab);

			/* Only lval2rval casts are lvalue-internals - they
			 * need the proper dereference done to them.
			 * Normal casts, in particular rvalue-casts of lvalues,
			 *
			 * e.g. int p; (long)p;
			 *
			 * are not lvalues, and so the non-lval-decay case doesn't
			 * set lvalue-internal
			 */
			switch(expr_is_lval(expr_cast_child(e))){
				case LVALUE_NO:
				case LVALUE_STRUCT:
					break;
				case LVALUE_USER_ASSIGNABLE:
					e->f_islval = expr_is_lval_struct;
			}

		}else{
			expr_cast_child(e) = fold_expr_nonstructdecay(expr_cast_child(e), stab);
		}
	}

	if(expr_cast_is_lval2rval(e)){
		e->tree_type = type_unattribute(
				type_unqualify(
					type_decay(
						expr_cast_child(e)->tree_type)));

	}else{
		/* casts remove restrict qualifiers */
		const enum fold_chk check_flags
			= FOLD_CHK_NO_ST_UN | FOLD_CHK_ALLOW_VOID | FOLD_CHK_NOWARN_ASSIGN;
		enum type_qualifier q = type_qual(e->bits.cast_to);
		int size_lhs, size_rhs;
		int warned_about_size = 0;
		type *ptr_lhs, *ptr_rhs;

		e->tree_type = type_qualify(e->bits.cast_to, q & ~qual_restrict);

		fold_type(e->tree_type, stab); /* struct lookup, etc */

		tlhs = e->tree_type;
		trhs = expr_cast_child(e)->tree_type;

		if(type_is_void(tlhs))
			return; /* fine */

		fold_check_expr(expr_cast_child(e), check_flags, "cast");

		fold_check_expr(e, check_flags, "cast-target");

		if(!type_is_complete(tlhs)){
			die_at(&e->where, "%scast to incomplete type %s",
					IMPLICIT_STR(e),
					type_to_str(tlhs));
		}

		ptr_lhs = type_is_ptr(tlhs);
		ptr_rhs = type_is_ptr(trhs);


		if((flag = !!type_is(tlhs, type_func))
		|| type_is(tlhs, type_array))
		{
			warn_at_print_error(&e->where, "%scast to %s type '%s'",
					IMPLICIT_STR(e),
					flag ? "function" : "array",
					type_to_str(tlhs));

			fold_had_error = 1;
			return;
		}

		if((ptr_lhs && type_is_floating(trhs))
		|| (ptr_rhs && type_is_floating(tlhs)))
		{
			fold_had_error = 1;
			warn_at_print_error(&e->where,
					"%scast %s pointer %s floating type",
					IMPLICIT_STR(e),
					ptr_lhs ? "to" : "from",
					ptr_lhs ? "from" : "to");
			return;
		}

		if(e->expr_cast_implicit){
			struct_union_enum_st *ea, *eb;

			ea = type_is_enum(tlhs);
			eb = type_is_enum(trhs);

			if(ea && eb && ea != eb){
				cc1_warn_at(&e->where,
						enum_mismatch,
						"implicit conversion from 'enum %s' to 'enum %s'",
						eb->spel, ea->spel);
			}else if(ea && !eb){
				/* passing to enum from non-enum */
				consty k;

				/* warn if out of range. if in range, warn about int literal -> enum */
				const_fold(e->expr, &k);

				if(k.type == CONST_NUM
				&& K_INTEGRAL(k.bits.num)
				&& !enum_has_value(ea, k.bits.num.val.i))
				{
					cc1_warn_at(&e->where,
							enum_out_of_range,
							"value %" NUMERIC_FMT_U " is out of range for 'enum %s'",
							k.bits.num.val.i,
							ea->spel);
				}
				else
				{
					cc1_warn_at(&e->where,
							enum_mismatch_int,
							"implicit conversion from '%s' to 'enum %s'",
							type_to_str(trhs),
							ea->spel);
				}
			}
		}

		size_lhs = type_size(tlhs, &e->where);
		size_rhs = type_size(trhs, &expr_cast_child(e)->where);

		if(!!ptr_lhs ^ !!ptr_rhs){
			consty k;

			if(ptr_lhs && expr_is_null_ptr(expr_cast_child(e), NULL_STRICT_INT)){
				/* no warning if 0 --> ptr */
			}else if(ptr_rhs && type_is_primitive(e->tree_type, type__Bool)){
				/* no warning for ptr --> bool */
			}else{
				/* this checks any pointer <--> integer conversion */

				if(e->expr_cast_implicit){
					cc1_warn_at(&e->where,
							int_ptr_conv,
							"implicit conversion between pointer and integer");

					warned_about_size = 1;

				}else if(size_lhs != size_rhs
				/* don't warn for (void *)0, etc */
				&& (const_fold(expr_cast_child(e), &k), k.type != CONST_NUM))
				{
					/* check explicit pointer <--> int truncation */
					char buf[TYPE_STATIC_BUFSIZ];

					cc1_warn_at(&e->where,
							int_ptr_conv,
							"cast %s '%s' %s smaller integer type '%s'",
							ptr_lhs ? "to" : "from",
							type_to_str(ptr_lhs ? tlhs : trhs),
							ptr_lhs ? "from" : "to",
							type_to_str_r(buf, ptr_lhs ? trhs : tlhs));

					warned_about_size = 1;
				}
			}
		}

		if(!warned_about_size && size_lhs < size_rhs
		&& !expr_kind(expr_cast_child(e), val))
		{
			char buf[DECL_STATIC_BUFSIZ];

			cc1_warn_at(&e->where, truncation,
					"possible truncation converting %s to %s",
					type_to_str(trhs), type_to_str_r(buf, tlhs));

			warned_about_size = 1;
		}

		if((flag = (type_is_fptr(tlhs) && type_is_nonfptr(trhs)))
		||         (type_is_fptr(trhs) && type_is_nonfptr(tlhs)))
		{
			/* allow cast from NULL to func ptr */
			if(!expr_is_null_ptr(expr_cast_child(e), NULL_STRICT_VOID_PTR)){
				char buf[TYPE_STATIC_BUFSIZ];

				cc1_warn_at(&e->where,
						mismatch_ptr,
						"%scast from %spointer to %spointer\n"
						"%s <- %s",
						IMPLICIT_STR(e),
						flag ? "" : "function-", flag ? "function-" : "",
						type_to_str(tlhs), type_to_str_r(buf, trhs));
			}
		}

		check_qual_rm(ptr_lhs, ptr_rhs, e);

		/* removes cv-qualifiers:
		 * (const int)3 has type int, not const int */
		e->tree_type = type_unqualify(e->tree_type);
	}
}

void fold_expr_cast(expr *e, symtable *stab)
{
	fold_expr_cast_descend(e, stab, 1);
}

const out_val *gen_expr_cast(const expr *e, out_ctx *octx)
{
	type *tto = e->tree_type;
	type *tfrom = expr_cast_child(e)->tree_type;
	const int cast_to_void = type_is_void(tto);
	const int is_volatile = type_qual(tfrom) & qual_volatile;
	const out_val *casted;

	/* return if cast-to-void */
	if(cast_to_void && !is_volatile){
		out_comment(octx, "(non-volatile) cast to void");
		return out_change_type(octx, gen_expr(expr_cast_child(e), octx), tto);
	}

	if(expr_cast_is_lval2rval(e) && !is_volatile){
		/* we're an lval2rval cast
		 * if inlining, check if we can substitute the lvalue's rvalue here
		 */
		decl *d = expr_to_declref(GEN_CONST_CAST(expr *, e), NULL);
		if(d && d->sym)
			return out_new_sym_val(octx, d->sym);
	}

	casted = gen_expr(expr_cast_child(e), octx);

	if(expr_cast_is_lval2rval(e)){
		if(type_is_s_or_u(tfrom)){
			/* either pass through as an LVALUE_STRUCT,
			 * or dereference here for cast-to-void, if volatile */
			UCC_ASSERT(
					cast_to_void ||
					type_cmp(tfrom, tto, 0) & (TYPE_EQUAL_ANY | TYPE_QUAL_ADD | TYPE_QUAL_SUB),
					"struct cast? (non-lval2rval)");

			out_comment(octx,
					"struct lval decay, cast_to_void=%d is_volatile=%d",
					cast_to_void, is_volatile);

			if(cast_to_void){
				if(is_volatile){
					out_force_read(octx, tfrom, casted);
				}else{
					out_val_consume(octx, casted);
				}

				/* we've been generated but won't be used
				 * i.e. (void) *a; */
				casted = out_new_noop(octx);

			}else{
				/* LVALUE_STRUCT passthru */
			}

		}else{
			/* primitive lval2rval */
			casted = out_deref(octx, casted);
		}

	}else{
		if(fopt_mode & FOPT_PLAN9_EXTENSIONS){
			/* allow b to be an anonymous member of a */
			struct_union_enum_st *a_sue = type_is_s_or_u(type_is_ptr(tto)),
			                      *b_sue = type_is_s_or_u(type_is_ptr(tfrom));

			if(a_sue && b_sue && a_sue != b_sue){
				decl *mem = struct_union_member_find_sue(b_sue, a_sue);

				if(mem){
					/*char buf[TYPE_STATIC_BUFSIZ];
						fprintf(stderr, "CAST %s -> %s, adj by %d\n",
						type_to_str(tfrom),
						type_to_str_r(buf, tto),
						mem->struct_offset);*/

					casted = out_change_type(
							octx,
							casted,
							type_ptr_to(
								type_nav_btype(cc1_type_nav, type_void)));

					casted = out_op(
							octx, op_plus,
							casted,
							out_new_l(
								octx,
								type_nav_btype(cc1_type_nav, type_intptr_t),
								mem->bits.var.struct_offset));
				}
			}
		}

		/* if cast-to-void and our operand is a volatile lvalue,
		 * we need to read it */
		if(cast_to_void
		&& is_volatile
		&& expr_is_lval(expr_cast_child(e)) != LVALUE_NO)
		{
			/* can read - lvalue, so it's in memory */
			out_force_read(octx, tfrom, casted);
			casted = out_new_noop(octx); /* fine - cast_to_void */
		}else{
			casted = out_cast(octx, casted, tto, /*normalise_bool:*/1);
		}

		/* a cast can potentially introduce a usage of a new type.
		 * let debug info know about it */
		gen_asm_emit_type(octx, tto);
	}

	return casted;
}

void dump_expr_cast(const expr *e, dump *ctx)
{
	const char *desc = "cast";

	if(expr_cast_is_lval2rval(e)){
		desc = "lvalue-decay";
	}else if(e->expr_cast_implicit){
		desc = "implicit cast";
	}

	dump_desc_expr(ctx, desc, e);

	dump_inc(ctx);
	dump_expr(expr_cast_child(e), ctx);
	dump_dec(ctx);
}

void mutate_expr_cast(expr *e)
{
	e->f_const_fold = fold_const_expr_cast;
}

expr *expr_new_cast(expr *what, type *to, int implicit)
{
	expr *e = expr_new_wrapper(cast);
	e->bits.cast_to = to;
	e->expr_cast_implicit = implicit;
	expr_cast_child(e) = what;
	return e;
}

expr *expr_new_cast_lval_decay(expr *sub)
{
	expr *e = expr_new_wrapper(cast);

	e->bits.cast_to = NULL; /* indicate lval2rval */

	expr_cast_child(e) = sub;
	return e;
}

const out_val *gen_expr_style_cast(const expr *e, out_ctx *octx)
{
	if(e->bits.cast_to)
		stylef("(%s)", type_to_str(e->bits.cast_to));
	IGNORE_PRINTGEN(gen_expr(expr_cast_child(e), octx));
	return NULL;
}
