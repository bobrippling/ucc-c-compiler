#include <strings.h>

#include "math.h"

int log2i(unsigned v)
{
	int r = -1;
	for(; v; v >>= 1, r++);
	return r;
}

int is_pow2(unsigned u)
{
	return (u & (u - 1)) == 0;
}

static unsigned pow2i(unsigned to)
{
	return 1 << to;
}

unsigned round_down_pow2(unsigned u)
{
	if(u == 0)
		return 0;

	if(is_pow2(u))
		return u;

	return pow2i(flsl(u) - 1);
}
