// RUN: %ocheck 0 %s

#define LARGE_NEGATIVE 4294967595

typedef unsigned long long uint64_t;

f(uint64_t x)
{
	if(x != -(uint64_t)LARGE_NEGATIVE)
		abort();
}

main()
{
	f(-(uint64_t)LARGE_NEGATIVE);
	return 0;
}
