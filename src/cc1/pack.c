#include "pack.h"
#include "../util/platform.h"

void pack_next(int *poffset, int *pthis, int sz, int align)
{
	static int word_size;
	int offset = *poffset, this;

	if(!word_size)
		word_size = platform_word_size();

	if(offset % align)
		offset += align - offset % align;

	this = offset;

	offset += sz;

	*poffset = offset, *pthis = this;
}
