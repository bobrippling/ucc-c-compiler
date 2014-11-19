typedef unsigned long ull;

ull rdtsc(void)
{
	unsigned hi, lo;
	__asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
	return (ull)lo  | (ull)hi << 32;
}
