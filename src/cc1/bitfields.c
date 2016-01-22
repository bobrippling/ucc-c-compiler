#include <stdio.h>

#include "../util/alloc.h"

#include "num.h"
#include "type.h"
#include "expr.h"
#include "decl_init.h"

#include "bitfields.h"


#define BITFIELD_DBG(...) /*fprintf(stderr, __VA_ARGS__)*/

integral_t bitfields_merge(
		const struct bitfield_val *vals,
		unsigned n,
		unsigned *const out_width)
{
	integral_t v = 0;
	unsigned width = 0;
	unsigned i;

	BITFIELD_DBG("bitfield out -- new\n");
	for(i = 0; i < n; i++){
		integral_t this = integral_truncate_bits(
				vals[i].val, vals[i].width, NULL);

		width += vals[i].width;

		BITFIELD_DBG("bitfield out: 0x%llx << %u gives ",
				this, vals[i].offset);

		v |= this << vals[i].offset;

		BITFIELD_DBG("0x%llx\n", v);
	}

	BITFIELD_DBG("bitfield done with 0x%llx\n", v);

	/* if width == 0, should be empty */
	*out_width = width;
	return v;
}

void bitfields_val_set(struct bitfield_val *bfv, expr *kval, expr *field_w)
{
	bfv->val = kval ? const_fold_val_i(kval) : 0;
	bfv->offset = 0;
	bfv->width = const_fold_val_i(field_w);
}

struct bitfield_val *bitfields_add(
		struct bitfield_val *bfs,
		unsigned *const pn,
		decl *d_mem,
		expr *e_val)
{
	const unsigned i = *pn;

	bfs = urealloc1(bfs, (*pn += 1) * sizeof *bfs);

	bitfields_val_set(&bfs[i],
			e_val,
			d_mem->bits.var.field_width);

	bfs[i].offset = d_mem->bits.var.struct_offset_bitfield;

	return bfs;
}

