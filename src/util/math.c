#include <strings.h>
#include "math.h"

static unsigned long long pow2i(unsigned v)
{
	unsigned long long out = 1;

	while(v--)
		out *= 2;

	return out;
}

int ispow2(unsigned long long v)
{
	return v && (v & (v - 1)) == 0;
}

static unsigned last_bit_index(unsigned long long v)
{
	int i;
	int last = -1;

	for(i = 0; v; i++){
		if(v & (1ull << i)){
			last = i;
			v &= ~(1ull << i);
		}
	}

	return last;
}

int log2i(unsigned v)
{
	return log2ll(v);
}

int log2ll(unsigned long long v)
{
	int r = -1;
	int adj = 0;
	if(!ispow2(v))
		adj = 0;
	for(; v; v >>= 1, r++);
	return r + adj;
}

unsigned long long round2(unsigned long long v)
{
	if(ispow2(v))
		return v;
	return pow2i(last_bit_index(v) + 1);
}
