#include "pack.h"
#include "../util/platform.h"

int pack_next(int offset, int sz, int align)
{
	static int word_size;

	if(!word_size)
		word_size = platform_word_size();

	offset += sz;
	if(offset % align)
		offset += align - offset % align;

	/*printf("pack(%d, %d) -> %d\n", offset, sz, next);*/

	return offset;
}
