#include "pack.h"
#include "../util/platform.h"

int pack_next(int offset, int sz, int align)
{
	static int word_size;
	int next;

	if(!word_size)
		word_size = platform_word_size();

	/* TODO */
	next = offset + sz + (offset + sz) % align;

	/*printf("pack(%d, %d) -> %d\n", offset, sz, next);*/

	return next;
}
