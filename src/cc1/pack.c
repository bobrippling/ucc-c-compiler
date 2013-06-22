#include <assert.h>

#include "pack.h"
#include "../util/platform.h"

int pack_to_align(int o, int align)
{
	assert(align > 0);

#ifdef SLOW
	if(o % align)
		o += align - o % align;

	return o;
#else
	return (o + align - 1) & -align;
#endif
}

int pack_to_word(int o)
{
	static int pws;
	if(!pws)
		pws = platform_word_size();
	return pack_to_align(o, pws);
}

void pack_next(
		unsigned *poffset, unsigned *after_space,
		unsigned sz, unsigned align)
{
	/* insert space as necessary */
	int offset = pack_to_align(*poffset, align);

	if(after_space)
		*after_space = offset;

	/* gap for the actual decl */
	offset += sz;

	/* return */
	*poffset = offset;
}
