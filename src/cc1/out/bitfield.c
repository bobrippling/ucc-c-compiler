#include <stddef.h>

#include "../type.h"
#include "../type_is.h"
#include "../type_nav.h"
#include "../defs.h"

#include "val.h"
#include "out.h"

#include "bitfield.h"

const out_val *out_bitfield_to_scalar(
		out_ctx *octx,
		const struct vbitfield *const bf,
		const out_val *scalar)
{
	type *const ty = scalar->t;

	/* shift right, then mask */
	scalar = out_op(
			octx, op_shiftr,
			scalar,
			out_new_l(octx, ty, bf->off));

	scalar = out_op(
			octx, op_and,
			scalar,
			out_new_l(octx, ty, ~(-1UL << bf->nbits)));

	/* if it's signed we need to sign extend
	 * using a signed right shift to copy its MSB
	 */
	if(type_is_signed(ty)){
		const unsigned ty_sz = type_size(ty, NULL);
		const unsigned nshift = CHAR_BIT * ty_sz - bf->nbits;

		scalar = out_op(
				octx, op_shiftl,
				scalar,
				out_new_l(octx, ty, nshift));

		scalar = out_op(
				octx, op_shiftr,
				scalar,
				out_new_l(octx, ty, nshift));
	}

	return scalar;
}

const out_val *out_bitfield_scalar_merge(out_ctx *octx,
		const struct vbitfield *const in_bf,
		const out_val *scalar, const out_val *pword)
{
	/* load in,
	 * &-out where we want our value (to 0),
	 * &-out our value (so we don't affect other bits),
	 * |-in our new value,
	 * store
	 */

	struct vbitfield bf = *in_bf;
	/* we get the lvalue type - change to pointer */
	type *const ty = pword->t, *ty_ptr = type_ptr_to(ty);
	unsigned long mask_leading_1s, mask_back_0s, mask_rm;
	out_val *mut_pword;
	const out_val *scratch;

	scalar = out_cast(octx, scalar, ty, /*normalise:*/0);

	/* load the pointer to the store, forgetting the bitfield */
	pword = out_change_type(octx, pword, ty_ptr);
	pword = mut_pword = v_dup_or_reuse(octx, pword, pword->t);
	mut_pword->bitfield.nbits = 0;

	/* load the bitfield without using bitfield semantics */
	scratch = out_deref(octx, mut_pword);

	/* e.g. width of 3, offset of 2:
	 * 111100000
	 *     ^~~^~
	 *     |  |- offset
	 *     +- width
	 */

	mask_leading_1s = -1UL << (bf.off + bf.nbits);

	/* e.g. again:
	 *
	 * -1 = 11111111
	 * << = 11111100
	 * ~  = 00000011
	 */
	mask_back_0s = ~(-1UL << bf.off);

	/* | = 111100011 */
	mask_rm = mask_leading_1s | mask_back_0s;

	/* &-out our value */
	out_comment(octx, "bitmask/rm = %#lx", mask_rm);
	scratch = out_op(
			octx, op_and,
			scratch,
			out_new_l(octx, ty, mask_rm));

	/* mask and shift our value up */
	scalar = out_op(
			octx, op_and,
			scalar,
			out_new_l(octx, ty, ~(-1UL << bf.nbits)));

	scalar = out_op(
			octx, op_shiftl,
			scalar,
			out_new_l(octx, ty, bf.off));

	/* | our value in with the dereferenced store value */
	scratch = out_op(
			octx, op_or,
			scalar, scratch);

	return scratch;
}
