#include <assert.h>

#include "pack.h"
#include "../util/platform.h"

typedef unsigned long pack_t;

pack_t pack_to_align(pack_t sz, unsigned align)
{
	assert(align > 0);

#ifdef SLOW
	if(sz % align)
		sz += align - sz % align;

	return sz;
#else
	return (sz + align - 1) & -align;
#endif
}

pack_t pack_to_word(pack_t o)
{
	static unsigned pws;
	if(!pws)
		pws = platform_word_size();
	return pack_to_align(o, pws);
}

void pack_next(
		unsigned long *poffset, unsigned long *after_space,
		unsigned sz, unsigned align)
{
	/* insert space as necessary */
	unsigned long offset = pack_to_align(*poffset, align);

	if(after_space)
		*after_space = offset;

	/* gap for the actual decl */
	offset += sz;

	/* return */
	*poffset = offset;
}
