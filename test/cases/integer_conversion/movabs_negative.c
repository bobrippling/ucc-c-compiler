// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

#define LARGE_NEGATIVE 4294967595

typedef unsigned long long uint64_t;

f(uint64_t x)
{
	if(x != -(uint64_t)LARGE_NEGATIVE)
		abort();
}

main()
{
#include "../ocheck-init.c"
	f(-(uint64_t)LARGE_NEGATIVE);
	return 0;
}
