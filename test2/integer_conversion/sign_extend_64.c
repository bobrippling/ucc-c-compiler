// RUN: %ocheck 0 %s

typedef unsigned long long uint64_t;

int main()
{
	uint64_t x = 1 << 31;
	uint64_t y = (uint64_t)1 << 31;

	if(x != 0xffffffff80000000)
		abort();
	if(y != 0x80000000)
		abort();

	return 0;
}
