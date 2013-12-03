// RUN: %ocheck 0 %s

typedef unsigned long long uint64_t;

int main()
{
	uint64_t x = 1 << 31; // (int)1 << 31 = -1, sign extend up
	uint64_t y = (uint64_t)1 << 31; // -1ULL << 31 = 0x8000.., zextend up

	if(x != 0xffffffff80000000)
		abort();
	if(y != 0x80000000)
		abort();

	return 0;
}
