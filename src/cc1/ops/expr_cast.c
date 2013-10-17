#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "ops.h"
#include "../../util/alloc.h"
#include "../../util/platform.h"
#include "expr_cast.h"
#include "../sue.h"
#include "../defs.h"

#define IMPLICIT_STR(e) ((e)->expr_cast_implicit ? "implicit " : "")
#define IS_RVAL_CAST(e)  (!(e)->bits.cast.tref)
#define IS_DECAY_CAST(e) ((e)->bits.cast.tref && e->bits.cast.is_decay)

const char *str_expr_cast()
{
	return "cast";
}

static void fold_cast_num(expr *const e, numeric *const num)
{
	int to_fp, from_fp;

	to_fp = type_ref_is_floating(e->tree_type);
	from_fp = type_ref_is_floating(expr_cast_child(e)->tree_type);

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
		switch(type_ref_primitive(e->tree_type)){
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
		/* float -> int */
		UCC_ASSERT(K_FLOATING(*num), "i/f mismatch types");
		num->val.i = num->val.f;
		num->suffix = 0;

		/* fall through to int logic */
	}

	UCC_ASSERT(K_INTEGRAL(*num), "fp const?");

#define pv (&num->val.i)
	/* need to cast the val.i down as appropriate */
	if(type_ref_is_type(e->tree_type, type__Bool)){
		*pv = !!*pv; /* analagous to out/out.c::out_normalise()'s constant case */

	}else if(e->expr_cast_implicit && !from_fp){
		const unsigned sz = type_ref_size(e->tree_type, &e->where);
		const integral_t old = *pv;
		const int to_sig   = type_ref_is_signed(e->tree_type);
		const int from_sig = type_ref_is_signed(expr_cast_child(e)->tree_type);
		integral_t to_iv;
		sintegral_t to_iv_sign_ext;

		/* we don't save the truncated value - we keep the original
		 * so negative numbers, for example, are preserved */
		to_iv = integral_truncate(*pv, sz, &to_iv_sign_ext);

		if(e->expr_cast_implicit
		&& (to_sig && from_sig ? (sintegral_t)old != to_iv_sign_ext : old != to_iv))
		{
#define CAST_WARN(pre_fmt, pre_val, post_fmt, post_val)  \
			warn_at(&e->where,                           \
					"implicit cast changes value from %"     \
					pre_fmt " to %" post_fmt,                \
					pre_val, post_val)

			/* nice... */
			if(from_sig){
				if(to_sig)
					CAST_WARN(
							NUMERIC_FMT_D, (long long signed)old,
							NUMERIC_FMT_D, (long long signed)to_iv_sign_ext);
				else
					CAST_WARN(
							NUMERIC_FMT_D, (long long signed)old,
							NUMERIC_FMT_U, (long long unsigned)to_iv);
			}else{
				if(to_sig)
					CAST_WARN(
							NUMERIC_FMT_U, (long long unsigned)old,
							NUMERIC_FMT_D, (long long signed)to_iv_sign_ext);
				else
					CAST_WARN(
							NUMERIC_FMT_U, (long long unsigned)old,
							NUMERIC_FMT_U, (long long unsigned)to_iv);
			}
		}

		/* need to sign extend if signed */
		*pv = from_sig && to_sig ? (integral_t)to_iv_sign_ext : to_iv;
	}
#undef pv
}

static void fold_const_expr_cast(expr *e, consty *k)
{
	int to_fp;

	if(type_ref_is_void(e->tree_type)){
		k->type = CONST_NO;
		return;
	}

	to_fp = type_ref_is_floating(e->tree_type);

	const_fold(expr_cast_child(e), k);

	if(IS_RVAL_CAST(e))
		return;

	switch(k->type){
		case CONST_NUM:
			fold_cast_num(e, &k->bits.num);
			break;

		case CONST_NO:
			break;

		case CONST_NEED_ADDR:
			if(to_fp){
				k->type = CONST_NO;
				break;
			}
			/* fall */

		case CONST_ADDR:
		case CONST_STRK:
		{
			int l, r;

			UCC_ASSERT(!to_fp, "cast to float from address");

			/* allow if we're casting to a same-size type */
			l = type_ref_size(e->tree_type, &e->where);

			if(type_ref_decayable(expr_cast_child(e)->tree_type))
				r = platform_word_size(); /* func-ptr or array->ptr */
			else
				r = type_ref_size(expr_cast_child(e)->tree_type, &expr_cast_child(e)->where);

			if(l < r){
				/* shouldn't fit, check if it will */
				switch(k->type){
					default:
						ICE("bad switch");

					case CONST_STRK:
						/* no idea where it will be in memory,
						 * can't fit into a smaller type */
						k->type = CONST_NO; /* e.g. (int)&a */
						break;

					case CONST_NEED_ADDR:
					case CONST_ADDR:
						if(k->bits.addr.is_lbl){
							k->type = CONST_NO; /* similar to strk case */
						}else{
							integral_t new = k->bits.addr.bits.memaddr;
							const int pws = platform_word_size();

							/* mask out bits so we have it truncated to `l' */
							if(l < pws){
								new = integral_truncate(new, l, NULL);

								if(k->bits.addr.bits.memaddr != new)
									/* can't cast without losing value - not const */
									k->type = CONST_NO;

							}else{
								/* what are you doing... */
								k->type = CONST_NO;
							}
						}
				}
			}
			break;
		}
	}
}

void fold_expr_cast_descend(expr *e, symtable *stab, int descend)
{
	int flag;
	type_ref *tlhs, *trhs;

	if(descend)
		FOLD_EXPR(expr_cast_child(e), stab);

	if(IS_RVAL_CAST(e)){
		/* remove cv-qualifiers */
		e->tree_type = type_ref_new_cast(expr_cast_child(e)->tree_type, qual_none);

	}else{
		/* casts remove restrict qualifiers */
		enum type_qualifier q = type_ref_qual(e->bits.cast.tref);

		e->tree_type = type_ref_new_cast(e->bits.cast.tref, q & ~qual_restrict);

		fold_type_ref(e->tree_type, NULL, stab); /* struct lookup, etc */

		tlhs = e->tree_type;
		trhs = expr_cast_child(e)->tree_type;

		if(!IS_DECAY_CAST(e)){
			int size_lhs, size_rhs;

			fold_check_expr(expr_cast_child(e), FOLD_CHK_NO_ST_UN, "cast-expr");
			if(type_ref_is_void(tlhs))
				return; /* fine */
			fold_check_expr(e, FOLD_CHK_NO_ST_UN, "cast-target");

			if(!type_ref_is_complete(tlhs)){
				die_at(&e->where, "%scast to incomplete type %s",
						IMPLICIT_STR(e),
						type_ref_to_str(tlhs));
			}

			if((flag = !!type_ref_is(tlhs, type_ref_func))
			|| type_ref_is(tlhs, type_ref_array))
			{
				die_at(&e->where, "%scast to %s type '%s'",
						IMPLICIT_STR(e),
						flag ? "function" : "array",
						type_ref_to_str(tlhs));
			}

			if(((flag = !!type_ref_is_ptr(tlhs)) && type_ref_is_floating(trhs))
			||           (type_ref_is_ptr(trhs)  && type_ref_is_floating(tlhs)))
			{
				/* TODO: factor to a error-continuing function */
				fold_had_error = 1;
				warn_at_print_error(&e->where,
						"%scast %s pointer %s floating type",
						IMPLICIT_STR(e),
						flag ? "to" : "from",
						flag ? "from" : "to");
				return;
			}

			size_lhs = type_ref_size(tlhs, &e->where);
			size_rhs = type_ref_size(trhs, &expr_cast_child(e)->where);
			if(size_lhs < size_rhs){
				char buf[DECL_STATIC_BUFSIZ];

				strcpy(buf, type_ref_to_str(trhs));

				cc1_warn_at(&e->where, 0, WARN_LOSS_PRECISION,
						"possible loss of precision %s, size %d <-- %s, size %d",
						type_ref_to_str(tlhs), size_lhs,
						buf, size_rhs);
			}

			if((flag = (type_ref_is_fptr(tlhs) && type_ref_is_nonfptr(trhs)))
			||         (type_ref_is_fptr(trhs) && type_ref_is_nonfptr(tlhs)))
			{
				/* allow cast from NULL to func ptr */
				if(!expr_is_null_ptr(expr_cast_child(e), 0)){
					char buf[TYPE_REF_STATIC_BUFSIZ];

					warn_at(&e->where, "%scast from %spointer to %spointer\n"
							"%s <- %s",
							IMPLICIT_STR(e),
							flag ? "" : "function-", flag ? "function-" : "",
							type_ref_to_str(tlhs), type_ref_to_str_r(buf, trhs));
				}
			}

#ifdef W_QUAL
			if(decl_is_ptr(tlhs) && decl_is_ptr(trhs) && (tlhs->type->qual | trhs->type->qual) != tlhs->type->qual){
				const enum type_qualifier away = trhs->type->qual & ~tlhs->type->qual;
				char *buf = type_qual_to_str(away);
				char *p;

				p = &buf[strlen(buf)-1];
				if(p >= buf && *p == ' ')
					*p = '\0';

				warn_at(&e->where, "%scast removes qualifiers (%s)",
						IMPLICIT_STR(e), buf);
			}
#endif
		}
	}
}

void fold_expr_cast(expr *e, symtable *stab)
{
	fold_expr_cast_descend(e, stab, 1);
}

void gen_expr_cast(expr *e)
{
	gen_expr(expr_cast_child(e));

	if(IS_RVAL_CAST(e)){
		/*out_to_rvalue();*/
	}else{
		type_ref *tto, *tfrom;

		tto = e->tree_type;
		tfrom = expr_cast_child(e)->tree_type;

		/* return if cast-to-void */
		if(type_ref_is_void(tto)){
			out_change_type(tto);
			out_comment("cast to void");
			return;
		}

		if(fopt_mode & FOPT_PLAN9_EXTENSIONS){
			/* allow b to be an anonymous member of a */
			struct_union_enum_st *a_sue = type_ref_is_s_or_u(type_ref_is_ptr(tto)),
													 *b_sue = type_ref_is_s_or_u(type_ref_is_ptr(tfrom));

			if(a_sue && b_sue && a_sue != b_sue){
				decl *mem = struct_union_member_find_sue(b_sue, a_sue);

				if(mem){
					/*char buf[TYPE_REF_STATIC_BUFSIZ];
						fprintf(stderr, "CAST %s -> %s, adj by %d\n",
						type_ref_to_str(tfrom),
						type_ref_to_str_r(buf, tto),
						mem->struct_offset);*/

					out_change_type(type_ref_cached_VOID_PTR());
					out_push_l(type_ref_cached_INTPTR_T(), mem->struct_offset);
					out_op(op_plus);
				}
			}
		}

		/* normalise before the cast, otherwise we do things like
		 * 5.3 -> 5, then normalise 5, instead of 5.3 != 0.0
		 */
		if(type_ref_is_type(tto, type__Bool)) /* 1 or 0 */
			out_normalise();

		out_cast(tto);
	}
}

void gen_expr_str_cast(expr *e)
{
	idt_printf("%scast expr:\n", IS_RVAL_CAST(e) ? "rvalue-" : "");
	gen_str_indent++;
	print_expr(expr_cast_child(e));
	gen_str_indent--;
}

void mutate_expr_cast(expr *e)
{
	e->f_const_fold = fold_const_expr_cast;
}

expr *expr_new_cast(expr *what, type_ref *to, int implicit)
{
	expr *e = expr_new_wrapper(cast);
	e->bits.cast.tref = to;
	e->expr_cast_implicit = implicit;
	expr_cast_child(e) = what;
	return e;
}

expr *expr_new_cast_rval(expr *sub)
{
	expr *e = expr_new_wrapper(cast);
	/* mark as rvalue cast */
	e->bits.cast.tref = NULL;
	e->bits.cast.is_decay = 0;
	expr_cast_child(e) = sub;
	return e;
}

expr *expr_new_cast_decay(expr *sub, type_ref *to)
{
	expr *e = expr_new_wrapper(cast);
	/* mark as decay */
	e->bits.cast.tref = to;
	e->bits.cast.is_decay = 1;
	expr_cast_child(e) = sub;
	return e;
}

void gen_expr_style_cast(expr *e)
{
	stylef("(%s)", type_ref_to_str(e->bits.cast.tref));
	gen_expr(expr_cast_child(e));
}
