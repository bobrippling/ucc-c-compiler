// RUN: %ocheck 2 %s

__attribute((always_inline))
static inline int f(void)
{
	return 2;
}

main()
{
	volatile int stash = 0;
	return stash + f() + stash; // force register spill before jumps for inlining
}
