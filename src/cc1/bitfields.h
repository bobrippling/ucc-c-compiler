#ifndef BITFIELD_H
#define BITFIELD_H

struct bitfield_val
{
	integral_t val;
	unsigned offset;
	unsigned width;
};

void bitfields_val_set(
		struct bitfield_val *,
		expr *kval,
		expr *field_w);

struct bitfield_val *bitfields_add(
		struct bitfield_val *,
		unsigned *nbitfields,
		decl *d_mem,
		expr *e_val);

integral_t bitfields_merge(
		const struct bitfield_val *bfs,
		unsigned n,
		unsigned *const out_width);

#endif
