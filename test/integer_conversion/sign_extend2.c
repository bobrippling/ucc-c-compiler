// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

typedef unsigned long long uint64_t;

main()
{
	// sign extend - negative so it becomes 0xffffffff80000000
	uint64_t x = (signed)0x80000000;

	if(x != 0xffffffff80000000)
		abort();

	if(x != -2147483648)
		abort();

	return 0;
}
