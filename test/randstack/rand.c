#include "asm.h"

__attribute__((constructor))
static void randomise_stack(void)
{
	typedef unsigned long T;
	T *p = get_rsp();

	for(long i = 0; i < 2048; i++)
		p[-i] = -1 ^ i ^ i << 16 ^ i << 32;
}
