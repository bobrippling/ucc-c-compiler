#include "type.h"

static enum type_cmp type_ref_cmp_r(
		type_ref *const orig_a,
		type_ref *const orig_b)
{
	type_ref *a, *b;

	if(!orig_a || !orig_b)
		return orig_a == orig_b ? TYPE_EQUAL : TYPE_NOT_EQUAL;

	/* FIXME: check qualifiers */
#if 0
	if(!type_qual_equal(a->qual, b->qual)){
		if(mode & TYPE_CMP_EXACT)
			return ?;

		/* if b is const, a must be */
		if((mode & TYPE_CMP_QUAL)
		&&  (b->qual & qual_const)
		&& !(a->qual & qual_const))
		{
			return ?;
		}
	}
#endif

	a = type_ref_skip_tdefs_casts(orig_a);
	b = type_ref_skip_tdefs_casts(orig_b);

	/* array/func decay takes care of any array->ptr checks */
	if(a->type != b->type)
		return TYPE_NOT_EQUAL;

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
				if((mode & DECL_CMP_ALLOW_TENATIVE_ARRAY) == 0)
					return TYPE_NOT_EQUAL;
			}

			/* next */
			break;
		}

		case type_ref_block:
			if(!type_qual_equal(a->bits.block.qual, b->bits.block.qual))
				return TYPE_NOT_EQUAL;
			break;

		case type_ref_ptr:
			TODO_HERE();
			if(fopt_mode & FOPT_PLAN9_EXTENSIONS){
				/* allow b to be an anonymous member of a, if pointers */
				struct_union_enum_st *a_sue = type_ref_is_s_or_u(type_ref_is_ptr(a)),
				                     *b_sue = type_ref_is_s_or_u(type_ref_is_ptr(b));

				if(a_sue && b_sue /* already know they aren't equal */){
					/* b_sue has an a_sue,
					 * the implicit cast adjusts to return said a_sue */
					if(struct_union_member_find_sue(b_sue, a_sue))
						return TYPE_CONVERTIBLE;
				}
			if(!type_qual_equal(a->bits.ptr.qual, b->bits.ptr.qual))
				return TYPE_NOT_EQUAL;
			break;

		case type_ref_cast:
		case type_ref_tdef:
			ICE("should've been skipped");

		case type_ref_func:
			if(FUNCARGS_ARE_EQUAL != funcargs_equal(a->bits.func, b->bits.func, 1 /* exact match */, NULL))
				return TYPE_NOT_EQUAL;
			break;
	}

	return type_ref_cmp_r(a->ref, b->ref, mode);
}

enum type_cmp type_ref_cmp(type_ref *a, type_ref *b)
{
	if(!(mode & DECL_CMP_EXACT_MATCH) && mode & DECL_CMP_ALLOW_VOID_PTR){ /* one side is void * */
		if(type_ref_is_void_ptr(a) && type_ref_is_ptr(b))
			return 1;
		if(type_ref_is_void_ptr(b) && type_ref_is_ptr(a))
			return 1;
	}

	return type_ref_cmp_r(a, b);
}
