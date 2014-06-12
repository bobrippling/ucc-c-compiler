#ifndef OUT_BITFIELD_H
#define OUT_BITFIELD_H

const out_val *out_bitfield_to_scalar(out_ctx *,
		const struct vbitfield *, const out_val *);

const out_val *out_bitfield_scalar_merge(out_ctx *,
		const struct vbitfield *,
		const out_val *scalar, const out_val *pword);

#endif
