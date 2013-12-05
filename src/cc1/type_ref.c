#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "../util/where.h"
#include "../util/util.h"

#include "data_structs.h"
#include "expr.h"
#include "sue.h"
#include "type_ref.h"
#include "decl.h"
#include "const.h"
#include "funcargs.h"
#include "cc1.h" /* fopt_mode */

#include "type_ref_is.h"

static enum type_cmp type_ref_cmp_r(
		type_ref *const orig_a,
		type_ref *const orig_b,
		enum type_cmp_opts opts)
{
	enum type_cmp ret;
	type_ref *a, *b;

	if(!orig_a || !orig_b)
		return orig_a == orig_b ? TYPE_EQUAL : TYPE_NOT_EQUAL;

	a = type_ref_skip_tdefs_casts(orig_a);
	b = type_ref_skip_tdefs_casts(orig_b);

	/* array/func decay takes care of any array->ptr checks */
	if(a->type != b->type){
		/* allow int <-> ptr */
		if((type_ref_is_ptr(a) && type_ref_is_integral(b))
		|| (type_ref_is_ptr(b) && type_ref_is_integral(a)))
		{
			return TYPE_CONVERTIBLE_EXPLICIT;
		}

		return TYPE_NOT_EQUAL;
	}

	switch(a->type){
		case type_ref_type:
			return type_cmp(a->bits.type, b->bits.type);

		case type_ref_array:
		{
			const int a_complete = !!a->bits.array.size,
			          b_complete = !!b->bits.array.size;

			if(a_complete && b_complete){
				const integral_t av = const_fold_val_i(a->bits.array.size),
				                 bv = const_fold_val_i(b->bits.array.size);

				if(av != bv)
					return TYPE_NOT_EQUAL;
			}else if(a_complete != b_complete){
				if((opts & TYPE_CMP_ALLOW_TENATIVE_ARRAY) == 0)
					return TYPE_NOT_EQUAL;
			}

			/* next */
			break;
		}

		case type_ref_block:
		case type_ref_ptr:
			break;

		case type_ref_cast:
		case type_ref_tdef:
			ICE("should've been skipped");

		case type_ref_func:
			if(funcargs_cmp(a->bits.func.args, b->bits.func.args)
					!= FUNCARGS_ARE_EQUAL)
			{
				/* "void (int)" and "void (int, int)" aren't equal,
				 * but a cast can soon fix it */
				return TYPE_CONVERTIBLE_EXPLICIT;
			}
			break;
	}

	ret = type_ref_cmp_r(a->ref, b->ref, opts);

	if(ret == TYPE_NOT_EQUAL
	&& a->type == type_ref_func)
	{
		/* "int (int)" and "void (int)" aren't equal - but castable */
		ret = TYPE_CONVERTIBLE_EXPLICIT;
	}

	if(ret == TYPE_NOT_EQUAL
	&& a->type == type_ref_ptr
	&& fopt_mode & FOPT_PLAN9_EXTENSIONS)
	{
		/* allow b to be an anonymous member of a, if pointers */
		struct_union_enum_st *a_sue = type_ref_is_s_or_u(a),
		                     *b_sue = type_ref_is_s_or_u(b);

		if(a_sue && b_sue /* already know they aren't equal */){
			/* b_sue has an a_sue,
			 * the implicit cast adjusts to return said a_sue */
			if(struct_union_member_find_sue(b_sue, a_sue))
				return TYPE_CONVERTIBLE_IMPLICIT;
		}
	}

	if(ret == TYPE_EQUAL
	&& (a->type == type_ref_ptr || a->type == type_ref_block))
	{
		/* check qualifiers of what we point to */
		const enum type_qualifier qa = type_ref_qual(a->ref),
		                          qb = type_ref_qual(b->ref);

		if(type_qual_loss(qa, qb))
			/* warns are done, but conversion allowed */
			ret = TYPE_QUAL_LOSS;
	}

	return ret;
}

enum type_cmp type_ref_cmp(type_ref *a, type_ref *b, enum type_cmp_opts opts)
{
	const enum type_cmp cmp = type_ref_cmp_r(a, b, opts);

	if(cmp == TYPE_NOT_EQUAL){
		/* try for convertible */
		if(type_ref_is_void_ptr(a) && type_ref_is_ptr(b))
			return TYPE_CONVERTIBLE_IMPLICIT;

		if(type_ref_is_void_ptr(b) && type_ref_is_ptr(a))
			return TYPE_CONVERTIBLE_IMPLICIT;
	}

	return cmp;
}
