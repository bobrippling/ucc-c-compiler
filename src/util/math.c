#include "math.h"

int log2i(unsigned v)
{
	int r = -1;
	for(; v; v >>= 1, r++);
	return r;
}
